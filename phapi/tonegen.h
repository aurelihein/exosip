#ifndef __TONEGEN_H__
#define __TONEGEN_H__

#define TG_SINE_TABLE_SHIFT  12
#define TG_SINE_TABLE_SIZE  (1 << TG_SINE_TABLE_SHIFT)
#define TG_SINE_SCALE  (8*1024.0)

/* 
 * the sine table contains TG_SINE_TABLE_SIZE samples the sin(x) function
 * where x go for 0 to 2pi. The samples are in fixed point represantation
 */ 
extern short tg_sine_tab[];




/* Each DTMF digit is defined bay a (low,high) frequency pair */
struct tg_dtmf_desc
{
  unsigned short f1;   /* low frequency */
  unsigned short f2;   /*  high frequency */
  unsigned char  digit; 
}; 


/* tone geneartor context  */
struct tonegen
{
  unsigned short acc;  /*  phase accumulator */
  unsigned short inc;  /*  phase increment   */
};


/* tone generator context for a DTMF digit */
struct dtmfgen
{
  struct tonegen  g1;   /* low frequency generator */
  struct tonegen  g2;   /* high frequency generator */
};



void tg_init_sine_table(void);

short tg_next_sample(struct tonegen *tg);

void tg_tone_init(struct tonegen *tg, unsigned freq, unsigned samplefreq, int reset);

int tg_dtmf_init(struct dtmfgen *dg, char digit, int samplefreq, int reset);

short tg_dtmf_next_sample(struct dtmfgen *dg);



#endif


