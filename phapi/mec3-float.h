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

#define ECHO_CAN_FP

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/slab.h>
#define MALLOC(a) kmalloc((a), GFP_KERNEL)
#define FREE(a) kfree(a)
#include <math.h>
#else /*! __KERNEL__ */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MALLOC(a) malloc(a)
#define FREE(a) free(a)
#endif /* !__KERNEL__ */

/*
 * Define COEFF_BACKUP for experimental coefficient backup code
 */


#define STEP_SIZE			0.4		/* Convergence rate */

#define SIGMA_P				0.01		/* Minimum adjustment */
#define SIGMA_REF_PWR		0.01		/* Keep denominator from being 0 */

#define MIN_TX_ENERGY		256.0/32767.0		/* Must have at least this much reference */
#define MIN_RX_ENERGY		 32.0/32767.0		/* Must have at least this much receive energy */

#define MAX_ATTENUATION		64.0				/* Maximum amount of loss we care about */
#define MAX_BETA			0.1

#define SUPPR_ATTENUATION	16.0				/* Amount of loss at which we suppress audio */

#define HANG_TIME			600		/* Hangover time */

#define NTAPS				8*160			/* Number of echo can taps */

#define BACKUP				256			/* Backup every this number of samples */

#define COEFF_BACKUP                    1

typedef struct {
	float buf[NTAPS * 2];
	float max;
	int maxexp;
} cbuf_f;

typedef struct {
	float  	a[NTAPS];				/* Coefficients */
#ifdef COEFF_BACKUP
	float  	b[NTAPS];				/* Coefficients */
	float  	c[NTAPS];				/* Coefficients */
	int backup;						/* Backup timer */
#endif	
	cbuf_f 	ref;					/* Reference excitation */
	cbuf_f 	sig;					/* Signal (echo + near end + noise) */
	cbuf_f	e;						/* Error */
	float	refpwr;					/* Reference power */
	int		taps;					/* Number of taps */
	int		hcntr;					/* Hangtime counter */
	int pos;						/* Position in curcular buffers */
} echo_can_state_t;

static inline void echo_can_free(echo_can_state_t *ec)
{
	FREE(ec);
}

static inline void buf_add(cbuf_f *b, float sample, int pos, int taps)
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
				if (b->max < fabs(b->buf[pos + x])) {
					b->max = fabs(b->buf[pos + x]);
					b->maxexp = x + 1;
				}
		}
	}
}

static inline short echo_can_update(echo_can_state_t *ec, short iref, short isig)
{
	int x;
	float ref;
	float sig;
	float u;
	float refpwr;
	float beta;			/* Factor */
	float se;			/* Simulated echo */
	/* Convert to floats about 1.0 */
	ref = (((float)iref)/32767.0);
	sig = (((float)isig)/32767.0);

#if 0
	printf("start: %d, finish: %d\n", ec->start, ec->finish);
#endif	

#ifdef COEFF_BACKUP
	if (!ec->backup) {
		/* Backup coefficients periodically */
		ec->backup = BACKUP;
		memcpy(ec->c,ec->b,sizeof(ec->c));
		memcpy(ec->b,ec->a,sizeof(ec->b));
	} else
		ec->backup--;
#endif		
	/* Remove old samples from reference power calculation */
	ec->refpwr -= (ec->ref.buf[ec->pos] * ec->ref.buf[ec->pos]);

	/* Store signal and reference */
	buf_add(&ec->ref, ref, ec->pos, ec->taps);
	buf_add(&ec->sig, sig, ec->pos, ec->taps);

	/* Add new reference power */	
	ec->refpwr += (ec->ref.buf[ec->pos] * ec->ref.buf[ec->pos]);


	/* Calculate simulated echo */
	se = 0.0;
	for (x=0;x<ec->taps;x++) 
		se += ec->a[x] * ec->ref.buf[ec->pos + x];

#if 0
	if (!(ec->pos2++ % 1024)) {
		printk("sig: %d, se: %d\n", (int)(32768.0 * sig), (int)(32768.0 * se));
	}
#endif
	u = sig - se;
	if (ec->hcntr)
		ec->hcntr--;

	/* Store error */
	buf_add(&ec->e, sig, ec->pos, ec->taps);
	if ((ec->ref.max > MIN_TX_ENERGY) && 
	    (ec->sig.max > MIN_RX_ENERGY) &&
		(ec->e.max * MAX_ATTENUATION > ec->ref.max)) {
		/* We have sufficient energy */
		if (ec->sig.max * 2.0 < ec->ref.max)  {
			/* No double talk */
			if (!ec->hcntr) {

			  if (ec->refpwr < SIGMA_REF_PWR)
					refpwr = SIGMA_REF_PWR;
				else
					refpwr = ec->refpwr;
				beta = STEP_SIZE * u / refpwr;
				if (beta > MAX_BETA)	
					beta = MAX_BETA;
				if (beta < -MAX_BETA)
					beta = -MAX_BETA;
				/* Update coefficients */
				for (x=0;x<ec->taps;x++) {
					ec->a[x] += beta * ec->ref.buf[ec->pos + x];
				}
			}
			//			printf("!");
		} else {
#ifdef COEFF_BACKUP
			if (!ec->hcntr) {
				/* Our double talk detector is turning on for the first time.  Revert
				   our coefficients, since we're probably well into the double talk by now */
				memcpy(ec->a, ec->c, sizeof(ec->a));
			}
			ec->backup = BACKUP;
#endif
			/* Reset hang-time counter, and prevent backups */
			ec->hcntr = HANG_TIME;
			//			printf("x");
		}
	}
#ifndef NO_ECHO_SUPPRESSOR	
	if (ec->e.max * SUPPR_ATTENUATION < ec->ref.max) {
		/* Suppress residual echo */
		u *= u;
	} 
#endif	
	ec->pos--;
	if (ec->pos < 0)
		ec->pos = ec->taps-1;
	u *= 32767.0;
	u *= 8.0;
	if (u < -32768.0)
		u = -32768.0;
	if (u > 32767.0)
		u = 32767.0;
	return (short)(u);
}

static inline echo_can_state_t *echo_can_create(int taps, int adaption_mode)
{
	echo_can_state_t *ec;
	taps = NTAPS;
	ec = MALLOC(sizeof(echo_can_state_t));
	if (ec) {
#if 0
		printk("Allocating MEC3 canceller (%d)\n", taps);
#endif
		memset(ec, 0, sizeof(echo_can_state_t));
		ec->taps = taps;
		if (ec->taps > NTAPS)
			ec->taps = NTAPS;
		ec->pos = ec->taps-1;
	}
	return ec;
}

#endif
