#ifdef WIN32
# define _USE_MATH_DEFINES
# include <math.h>
#  ifndef M_PI
#    define M_PI 3.14159265358979323846
#  endif /* !M_PI */
#else /* !WIN32 */
# include <math.h>
#endif /* !WIN32 */
#include "tonegen.h"


/* 
 * the sine table contains TG_SINE_TABLE_SIZE samples the sin(x) function
 * where x go for 0 to 2pi. The samples are in fixed point represantation
 */ 
short tg_sine_tab[TG_SINE_TABLE_SIZE];


void tg_init_sine_table(void)
{
  int i;
  float x;

  for (i = 0; i < TG_SINE_TABLE_SIZE; i++)
    {
      x = i * 2 * M_PI / TG_SINE_TABLE_SIZE;
      tg_sine_tab[i] = (short) (0.5 + sin(x) * TG_SINE_SCALE);
    }

}


const struct tg_dtmf_desc tg_dtmf_tones[] =
{
  { 941, 1336, '0'},
  { 697, 1209, '1'},
  { 697, 1336, '2'},
  { 697, 1477, '3'},
  { 770, 1209, '4'},
  { 770, 1336, '5'},
  { 770, 1477, '6'},
  { 852, 1209, '7'},
  { 852, 1336, '8'},
  { 852, 1477, '9'},
  { 941, 1209, '*'},
  { 941, 1477, '#'},
  { 697, 1633, 'A'},
  { 770, 1633, 'B'},
  { 852, 1633, 'C'},
  { 941, 1633, 'D'}
};





short tg_next_sample(struct tonegen *tg)
{
  tg->acc += tg->inc;

  return tg_sine_tab[tg->acc >> (16 - TG_SINE_TABLE_SHIFT)];
}


void tg_tone_init(struct tonegen *tg, unsigned freq, unsigned samplefreq, int reset)
{
  unsigned long tmp;

  if (reset)
    tg->acc = 0;

  tmp =  freq * (1lu << 16);
  tg->inc = tmp / samplefreq;
  
}

int tg_dtmf_init(struct dtmfgen *dg, char digit, int samplefreq, int reset)
{
  int i;

  for(i = 0; i < 16; i++)
    if (tg_dtmf_tones[i].digit == digit)
      {
	tg_tone_init(&dg->g1, tg_dtmf_tones[i].f1, samplefreq, reset);
	tg_tone_init(&dg->g2, tg_dtmf_tones[i].f2, samplefreq, reset);
	return 0;
      }

  /* invalid dtmf digit */
  return -1;
}

short tg_dtmf_next_sample(struct dtmfgen *dg)
{
  return (tg_next_sample(&dg->g1) + tg_next_sample(&dg->g2));
}
