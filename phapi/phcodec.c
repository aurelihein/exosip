#include <stdlib.h>
#include "phcodec.h"

#include "gsm/gsm.h"
#include "gsm/private.h"


#include "ilbc/iLBC_define.h"
#include "ilbc/iLBC_encode.h"
#include "ilbc/iLBC_decode.h"

//#ifndef EMBED
#ifndef NO_GSM
#define ENABLE_GSM 1
#endif
#ifndef NO_ILBC
#define ENABLE_ILBC 1
#endif

#ifdef WIN32
# define inline _inline
#endif /* !WIN32 */

/*
 *  PCM - A-Law conversion
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 *
 *  Wrapper for linphone Codec class by Simon Morlat <simon.morlat@free.fr>
 */

static inline int val_seg(int val)
{
	int r = 0;
	val >>= 7;
	if (val & 0xf0) {
		val >>= 4;
		r += 4;
	}
	if (val & 0x0c) {
		val >>= 2;
		r += 2;
	}
	if (val & 0x02)
		r += 1;
	return r;
}

/*
 * s16_to_alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * s16_to_alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

static inline unsigned char s16_to_alaw(int pcm_val)
{
	int		mask;
	int		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;
	} else {
		mask = 0x55;
		pcm_val = -pcm_val;
		if (pcm_val > 0x7fff)
			pcm_val = 0x7fff;
	}

	if (pcm_val < 256)
		aval = pcm_val >> 4;
	else {
		/* Convert the scaled magnitude to segment number. */
		seg = val_seg(pcm_val);
		aval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0x0f);
	}
	return aval ^ mask;
}

/*
 * alaw_to_s16() - Convert an A-law value to 16-bit linear PCM
 *
 */
static inline alaw_to_s16(unsigned char a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;
	t = a_val & 0x7f;
	if (t < 16)
		t = (t << 4) + 8;
	else {
		seg = (t >> 4) & 0x07;
		t = ((t & 0x0f) << 4) + 0x108;
		t <<= seg -1;
	}
	return ((a_val & 0x80) ? t : -t);
}
/*
 * s16_to_ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

static unsigned char s16_to_ulaw(int pcm_val)	/* 2's complement (16-bit range) */
{
	int mask;
	int seg;
	unsigned char uval;

	if (pcm_val < 0) {
		pcm_val = 0x84 - pcm_val;
		mask = 0x7f;
	} else {
		pcm_val += 0x84;
		mask = 0xff;
	}
	if (pcm_val > 0x7fff)
		pcm_val = 0x7fff;

	/* Convert the scaled magnitude to segment number. */
	seg = val_seg(pcm_val);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0x0f);
	return uval ^ mask;
}

/*
 * ulaw_to_s16() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static inline int ulaw_to_s16(unsigned char u_val)
{
	int t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & 0x0f) << 3) + 0x84;
	t <<= (u_val & 0x70) >> 4;

	return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
}




void mulaw_dec(char *mulaw_data /* contains size char */,
	       char *s16_data    /* contains size*2 char */,
	       int size)
{
  int i;
  for(i=0;i<size;i++)
    {
      *((signed short*)s16_data)=ulaw_to_s16( (unsigned char) mulaw_data[i]);
      s16_data+=2;
    }
}

void mulaw_enc(char *s16_data    /* contains pcm_size char */,
	       char *mulaw_data  /* contains pcm_size/2 char */,
	       int pcm_size)
{
  int i;
  int limit = pcm_size/2;
  for(i=0;i<limit;i++)
    {
      mulaw_data[i]=s16_to_ulaw( *((signed short*)s16_data) );
      s16_data+=2;
    }
}

void alaw_dec(char *alaw_data   /* contains size char */,
	      char *s16_data    /* contains size*2 char */,
	      int size)
{
  int i;
  for(i=0;i<size;i++)
    {
      ((signed short*)s16_data)[i]=alaw_to_s16( (unsigned char) alaw_data[i]);
    }
}

void alaw_enc(char *s16_data   /* contains 320 char */,
	      char *alaw_data  /* contains 160 char */,
	      int pcm_size)
{
  int i;
  int limit = pcm_size/2;
  for(i=0;i<limit;i++)
    {
      alaw_data[i]=s16_to_alaw( *((signed short*)s16_data) );
      s16_data+=2;
    }
}




static int pcmu_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static int pcmu_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static int pcma_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static int pcma_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);



static int pcmu_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  mulaw_enc((char *) src, (char *) dst, srcsize);
  return srcsize/2;
}

static int pcmu_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  mulaw_dec((char *) src, (char *) dst, srcsize);
  return srcsize*2;
}

static int pcma_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  alaw_enc((char *) src, (char *) dst, srcsize);
  return srcsize/2;
}

static int pcma_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  alaw_dec((char *) src, (char *) dst, srcsize);
  return srcsize*2;
}




static phcodec_t pcmu_codec = 
{
  "PCMU", 160, 320, 0, 0, 0, 0, 
  pcmu_encode, pcmu_decode
};

static phcodec_t pcma_codec =
{
  "PCMA", 160, 320, 0, 0, 0, 0, 
  pcma_encode, pcma_decode
};


#ifdef ENABLE_GSM

#define GSM_ENCODED_FRAME_SIZE 33
#define GSM_FRAME_SAMPLES 160
#define GSM_FRAME_DURATION 20

static int ph_gsm_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static int ph_gsm_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static void *ph_gsm_init(void);
     static void ph_gsm_cleanup(void *ctx);

static void *
ph_gsm_init(void)
{
  return gsm_create();
}

static void 
ph_gsm_cleanup(void *ctx)
{
  gsm_destroy(ctx);
}


int ph_gsm_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  gsm_encode(ctx, src, dst);
  return GSM_ENCODED_FRAME_SIZE;

}

int ph_gsm_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
  gsm_decode(ctx, src, dst);
  return GSM_FRAME_SAMPLES*2;
}

static phcodec_t gsm_codec =
{
  "GSM", GSM_ENCODED_FRAME_SIZE, GSM_FRAME_SAMPLES*2, ph_gsm_init, ph_gsm_init, ph_gsm_cleanup, ph_gsm_cleanup, 
  ph_gsm_encode, ph_gsm_decode
};


#endif

#ifdef ENABLE_ILBC

static int ph_ilbc_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static int ph_ilbc_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
static void *ph_ilbc_dec_init(void);
static void *ph_ilbc_enc_init(void);
static void ph_ilbc_dec_cleanup(void *ctx);
static void ph_ilbc_enc_cleanup(void *ctx);

static void *ph_ilbc_dec_init(void)
{
    iLBC_Dec_Inst_t *ctx;

    ctx = malloc(sizeof(*ctx));

    initDecode(ctx, 30, 1);

    return ctx;
}


static void *ph_ilbc_enc_init(void)
{
    iLBC_Enc_Inst_t *ctx;

    ctx = malloc(sizeof(*ctx));

    initEncode(ctx, 30);

    return ctx;

}


static void ph_ilbc_dec_cleanup(void *ctx)
{
  free(ctx);
}

static void ph_ilbc_enc_cleanup(void *ctx)
{
  free(ctx);
}


static int ph_ilbc_decode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
    int k; 
    float decflt[BLOCKL_MAX], tmp; 
    short *decshrt = (short*) dst;
    iLBC_Dec_Inst_t *dec = (iLBC_Dec_Inst_t *) ctx;

 
    iLBC_decode(decflt, (unsigned char *)src, dec, 1); 
 
 
    for (k=0; k< dec->blockl; k++){  
        tmp=decflt[k]; 
        if (tmp<MIN_SAMPLE) 
            tmp=MIN_SAMPLE; 
        else if (tmp>MAX_SAMPLE) 
            tmp=MAX_SAMPLE; 
        decshrt[k] = (short) tmp; 
    } 
 
    return (dec->blockl); 

}


static int ph_ilbc_encode(void *ctx, const void *src, int srcsize, void *dst, int dstsize)
{
    float tmp[BLOCKL_MAX];
    short *indata = (short *) src;
    int k; 
    iLBC_Enc_Inst_t *enc = (iLBC_Enc_Inst_t *) ctx;
 
    for (k=0; k<enc->blockl; k++) 
      tmp[k] = (float)indata[k]; 
 
    /* do the actual encoding */ 
 
    iLBC_encode((unsigned char *)dst, tmp, enc); 
 
 
    return (enc->no_of_bytes); 

}

 
  





static phcodec_t ilbc_codec =
{
  "ILBC", NO_OF_BYTES_30MS, BLOCKL_30MS, ph_ilbc_enc_init, ph_ilbc_dec_init, ph_ilbc_enc_cleanup, ph_ilbc_dec_cleanup, 
  ph_ilbc_encode, ph_ilbc_decode
};

#endif

phcodec_t *ph_codec_list;

void ph_media_codecs_init(void)
{
#ifdef ENABLE_ILBC
#ifdef ENABLE_GSM
  gsm_codec.next = &ilbc_codec;
#else
  pcmu_codec.next = &ilbc_codec;
#endif 
#endif

#ifdef ENABLE_GSM
  pcmu_codec.next = &gsm_codec;
#endif

  pcma_codec.next = &pcmu_codec;
  ph_codec_list = &pcma_codec;

}
