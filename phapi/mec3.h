/*
 * Mark's Third Echo Canceller
 *
 * Copyright (C) 2003, Digium, Inc.
 *
 * This program is free software and may be used
 * and distributed under the terms of the GNU General Public 
 * License, incorporated herein by reference.  
 *
 * Dedicated to the crew of the Columbia, STS-107 for their
 * bravery and courageous sacrifice for science.
 *
 */

#ifndef _MARK3_ECHO_H
#define _MARK3_ECHO_H



#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/slab.h>
#define MALLOC(a) kmalloc((a), GFP_KERNEL)
#define FREE(a) kfree(a)
#else
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#define MALLOC(a) malloc(a)
#define FREE(a) free(a)
#endif

/* Features */

/*
 * DO_BACKUP -- Backup coefficients, and revert in the presense of double talk to try to prevent
 * them from diverging during the ramp-up before the DTD kicks in
 */
#define DO_BACKUP     1

#define STEP_SHIFT			2		/* Convergence rate higher = slower / better (as a shift) */

#define SIGMA_REF_PWR		655		/* Keep denominator from being 0 */

#define MIN_TX_ENERGY		256		/* Must have at least this much reference */
#define MIN_RX_ENERGY		 32		/* Must have at least this much receive energy */

//#define MAX_ATTENUATION_SHIFT	6		/* Maximum amount of loss we care about */
#define MAX_ATTENUATION_SHIFT	12		/* Maximum amount of loss we care about */

#define MAX_BETA			1024

#define SUPPR_SHIFT			  4				/* Amount of loss at which we suppress audio */

#define HANG_TIME			600		/* Hangover time */

#define NTAPS				256			/* Number of echo can taps */

#define BACKUP				256			/* Backup every this number of samples */

#define POWER_OFFSET		5			/* Shift power by this amount to be sure we don't overflow the
										   reference power.  Higher = less likely to overflow, lower = more accurage */

#include "arith.h"

typedef struct {
	short buf[NTAPS * 2];
	short max;
	int maxexp;
} cbuf_s;

typedef struct {
	short	a_s[NTAPS];				/* Coefficients in shorts */
	int  	a_i[NTAPS];				/* Coefficients in ints*/
#ifdef DO_BACKUP
	int  	b_i[NTAPS];				/* Coefficients (backup1) */
	int  	c_i[NTAPS];				/* Coefficients (backup2) */
#endif	
	cbuf_s 	ref;					/* Reference excitation */
	cbuf_s 	sig;					/* Signal (echo + near end + noise) */
	cbuf_s	e;						/* Error */
	int		refpwr;					/* Reference power */
	int		taps;					/* Number of taps */
	int		tappwr;					/* Power of taps */
	int		hcntr;					/* Hangtime counter */
	int pos;						/* Position in curcular buffers */
	int backup;						/* Backup timer */
} echo_can_state_t;

static inline void echo_can_free(echo_can_state_t *ec)
{
	FREE(ec);
}

static inline void buf_add(cbuf_s *b, short sample, int pos, int taps)
{
	/* Store and keep track of maxima */
	int x;
	b->buf[pos] = sample;
	b->buf[pos + taps] = sample;
	if (sample > b->max) {
		b->max = sample;
		b->maxexp = taps;
	} else {
		b->maxexp--;
		if (!b->maxexp) {
			b->max = 0;
			for (x=0;x<taps;x++)
				if (b->max < abs(b->buf[pos + x])) {
					b->max = abs(b->buf[pos + x]);
					b->maxexp = x + 1;
				}
		}
	}
}

static inline short echo_can_update(echo_can_state_t *ec, short ref, short sig)
{
	int x;
	short u;
	int refpwr;
	int beta;			/* Factor */
	int se;			/* Simulated echo */
#ifdef DO_BACKUP
	if (!ec->backup) {
		/* Backup coefficients periodically */
		ec->backup = BACKUP;
		memcpy(ec->c_i,ec->b_i,sizeof(ec->c_i));
		memcpy(ec->b_i,ec->a_i,sizeof(ec->b_i));
	} else
		ec->backup--;
#endif		
	/* Remove old samples from reference power calculation */
	ec->refpwr -= ((ec->ref.buf[ec->pos] * ec->ref.buf[ec->pos]) >> POWER_OFFSET);

	/* Store signal and reference */
	buf_add(&ec->ref, ref, ec->pos, ec->taps);
	buf_add(&ec->sig, sig, ec->pos, ec->taps);

	/* Add new reference power */	
	ec->refpwr += ((ec->ref.buf[ec->pos] * ec->ref.buf[ec->pos]) >> POWER_OFFSET);


	/* Calculate simulated echo */
	se = CONVOLVE2(ec->a_s, ec->ref.buf + ec->pos, ec->taps);		
	se >>= 15;
	
	u = sig - se;
	if (ec->hcntr)
		ec->hcntr--;

	/* Store error */
	buf_add(&ec->e, sig, ec->pos, ec->taps);
	if ((ec->ref.max > MIN_TX_ENERGY) && 
	    (ec->sig.max > MIN_RX_ENERGY) &&
		(ec->e.max > (ec->ref.max >> MAX_ATTENUATION_SHIFT))) {
		/* We have sufficient energy */
		if (ec->sig.max < (ec->ref.max >> 1))  {
			/* No double talk */
			if (!ec->hcntr) {
				refpwr = ec->refpwr >> (16 - POWER_OFFSET);
				if (refpwr < SIGMA_REF_PWR)
					refpwr = SIGMA_REF_PWR;
				beta = (u << 16) / refpwr;
				beta >>= STEP_SHIFT;
				if (beta > MAX_BETA)	
					beta = MAX_BETA;
				if (beta < -MAX_BETA)
					beta = -MAX_BETA;
				/* Update coefficients */
				for (x=0;x<ec->taps;x++) {
					ec->a_i[x] += beta * ec->ref.buf[ec->pos + x];
					ec->a_s[x] = ec->a_i[x] >> 16;
				}
			}
		} else {
#ifdef DO_BACKUP
			if (!ec->hcntr) {
				/* Our double talk detector is turning on for the first time.  Revert
				   our coefficients, since we're probably well into the double talk by now */
				memcpy(ec->a_i, ec->c_i, sizeof(ec->a_i));
				for (x=0;x<ec->taps;x++) {
					ec->a_s[x] = ec->a_i[x] >> 16;
				}
			}
#endif			
			/* Reset hang-time counter, and prevent backups */
			ec->hcntr = HANG_TIME;
#ifdef DO_BACKUP
			ec->backup = BACKUP;
#endif			
		}
	}
#ifndef NO_ECHO__SUPPRESSOR	
	if (ec->e.max < (ec->ref.max >> SUPPR_SHIFT)) {
		/* Suppress residual echo */
		u *= u;
		u >>= 16;
	} 
#endif	
	ec->pos--;
	if (ec->pos < 0)
		ec->pos = ec->taps-1;
	return u;
}

static inline echo_can_state_t *echo_can_create(int taps, int adaption_mode)
{
	echo_can_state_t *ec;
	int x;

	taps = NTAPS;
	ec = MALLOC(sizeof(echo_can_state_t));
	if (ec) {
		memset(ec, 0, sizeof(echo_can_state_t));
		ec->taps = taps;
		ec->pos = ec->taps-1;
		for (x=0;x<31;x++) {
			if ((1 << x) >= ec->taps) {
				ec->tappwr = x;
				break;
			}
		}
	}
	return ec;
}

static inline int echo_can_traintap(echo_can_state_t *ec, int pos, short val)
{
	/* Reset hang counter to avoid adjustments after
	   initial forced training */
	ec->hcntr = ec->taps << 1;
	if (pos >= ec->taps)
		return 1;
	ec->a_i[pos] = val << 17;
	ec->a_s[pos] = val << 1;
	if (++pos >= ec->taps)
		return 1;
	return 0;
}


#endif
