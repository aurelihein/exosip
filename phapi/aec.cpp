/***************************************************************
A.2 aec.cxx
***************************************************************/
/* aec.cxx
 * Acoustic Echo Cancellation NLMS-pw algorithm
 * Author: Andre Adrian, DFS Deutsche Flugsicherung
 * <Andre.Adrian@dfs.de>
 *
 * Version 1.1
 * Copyright (C) DFS Deutsche Flugsicherung (2004). All Rights Reserved.
 *           (C) Mbdsys SARL (2004).
 * 
 * You are allowed to use this source code in any open source or closed source
 * software you want. You are allowed to use the algorithms for a hardware
 * solution. You are allowed to modify the source code.
 * You are not allowed to remove the name of the author from this memo or from
 * the source code files. You are not allowed to monopolize the source code or
 * the algorithms behind the source code as your intellectual property.
 * This source code is free of royalty and comes with no warranty.
 */

#include <stdio.h>
#include <stdlib.h>
# ifdef WIN32
#  define _USE_MATH_DEFINES
#  include <math.h>
#  define roundf(x) floorf((x) + 0.5f) 
#  ifndef M_PI
#    define M_PI 3.14159265358979323846
#  endif /* !M_PI */
# else /* !WIN32 */
#  include <math.h>
# endif /* !WIN32 */
#include <string.h>
#include "aec.h"



IIR_HP6::IIR_HP6()
{
  memset(this, 0, sizeof(IIR_HP6));
}


/* Vector Dot Product */
float dotp(float a[], float b[]) {
  float sum0 = 0.0, sum1 = 0.0;
  int j;
  
  for (j = 0; j < NLMS_LEN; j+= 2) {
    // optimize: partial loop unrolling
    sum0 += a[j] * b[j];
    sum1 += a[j+1] * b[j+1];
  }
  return sum0+sum1;
}


/*
 * Algorithm:  Recursive single pole FIR high-pass filter
 *
 * Reference: The Scientist and Engineer's Guide to Digital Processing
 */

FIR1::FIR1()
{
}

void FIR1::init(float preWhiteTransferAlpha)
{
  float x = exp(-2.0 * M_PI * preWhiteTransferAlpha);
  
  a0 = (1.0f + x) / 2.0f;
  a1 = -(1.0f + x) / 2.0f;
  b1 = x;
  last_in = 0.0f;
  last_out = 0.0f;
}

AEC::AEC()
{
  hp1.init(0.01f);  /* 10Hz */
  Fx.init(PreWhiteAlphaTF);
  Fe.init(PreWhiteAlphaTF);
  
  max_max_x = 0.0f;
  hangover = 0;
  memset(max_x, 0, sizeof(max_x));
  dtdCnt = dtdNdx = 0;
  
  memset(x, 0, sizeof(x));
  memset(xf, 0, sizeof(xf));
  memset(w, 0, sizeof(w));
  j = NLMS_EXT;
  lastupdate = 0;
  dotp_xf_xf = 0.0f;
}


float AEC::nlms_pw(float mic, float spk, int update)
{
  float d = mic;      	        // desired signal
  x[j] = spk;
  xf[j] = Fx.highpass(spk);     // pre-whitening of x

  // calculate error value (mic signal - estimated mic signal from spk signal)
  float e = d - dotp(w, x + j);
  float ef = Fe.highpass(e);    // pre-whitening of e
  if (update) {    
    if (lastupdate) {
      // optimize: iterative dotp(xf, xf)
      dotp_xf_xf += (xf[j]*xf[j] - xf[j+NLMS_LEN-1]*xf[j+NLMS_LEN-1]);
    } else {
      dotp_xf_xf = dotp(xf+j, xf+j);
    } 
      
    // calculate variable step size
    float mikro_ef = 0.5f * ef / dotp_xf_xf;
    
    // update tap weights (filter learning)
    int i;
    for (i = 0; i < NLMS_LEN; i += 2) {
      // optimize: partial loop unrolling
      w[i] += mikro_ef*xf[i+j];
      w[i+1] += mikro_ef*xf[i+j+1];
    }
  }
  lastupdate = update;
  
  if (--j < 0) {
    // optimize: decrease number of memory copies
    j = NLMS_EXT;
    memmove(x+j+1, x, (NLMS_LEN-1)*sizeof(float));    
    memmove(xf+j+1, xf, (NLMS_LEN-1)*sizeof(float));    
  }
  
  return e;
}


int AEC::dtd(float d, float x)
{
  // optimized implementation of max(|x[0]|, |x[1]|, .., |x[L-1]|):
  // calculate max of block (DTD_LEN values)
  x = fabsf(x);
  if (x > max_x[dtdNdx]) {
    max_x[dtdNdx] = x;
    if (x > max_max_x) {
      max_max_x = x;
    }
  }
  if (++dtdCnt >= DTD_LEN) {
    dtdCnt = 0;
    // calculate max of max
    max_max_x = 0.0f;
    for (int i = 0; i < NLMS_LEN/DTD_LEN; ++i) {
      if (max_x[i] > max_max_x) {
        max_max_x = max_x[i];
      }
    }
    // rotate Ndx
    if (++dtdNdx >= NLMS_LEN/DTD_LEN) dtdNdx = 0;
    max_x[dtdNdx] = 0.0f;
  }

  // The Geigel DTD algorithm with Hangover timer Thold
  if (fabsf(d) >= GeigelThreshold * max_max_x) {
    hangover = Thold;
  }
    
  if (hangover) --hangover;
  
  if (max_max_x < UpdateThreshold) {
    // avoid update with silence or noise
    return 1;
  } else {
    return (hangover > 0);
  }
}


int AEC::doAEC(int d, int x) 
{
  float s0 = (float)d;
  float s1 = (float)x;
  
  // Mic Highpass Filter - telephone users are used to 300Hz cut-off
 s0 = hp0.highpass(s0);

  // Spk Highpass Filter - to remove DC
 s1 = hp1.highpass(s1);

  // Double Talk Detector
  int update = !dtd(s0, s1);
//  printf("%s", update?"!":"x");

  // Acoustic Echo Cancellation
  s0 = nlms_pw(s0, s1, update);

  // Acoustic Echo Suppression
  if (update) {
    // Non Linear Processor (NLP): attenuate low volumes
    s0 *= NLPAttenuation;
  }

  // Try to replace Mic Boost
 // s0 *= 8.0f;
  // Saturation
  if (s0 > MAXPCM) {
    s0 = (int)MAXPCM;
  } else if (s0 < -MAXPCM) {
    s0 = (int)-MAXPCM;
  } else {
    s0 = (int)roundf(s0);
  }
s0 = hp2.highpass(s0);  
 return(s0);
}

class AEC *gAec;


extern "C" {

void create_AEC(){
  gAec = new AEC();
}

short do_AEC(short ref, short mic){
  if(gAec)
    return((short)(gAec->doAEC((int)mic, (int)ref)));

  return mic;
}

void kill_AEC(){
  delete gAec;
}

}
