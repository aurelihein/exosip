/***************************************************************
A.1 aec.h
***************************************************************/

#ifndef _AEC_H	/* include only once */

/* aec.h
 * Acoustic Echo Cancellation NLMS-pw algorithm
 * Author: Andre Adrian, DFS Deutsche Flugsicherung
 * <Andre.Adrian@dfs.de>
 *
 * Version 1.1
 */

/* dB Values */
const float M0dB = 1.0f;
const float M3dB = 0.71f;
const float M6dB = 0.50f;
 
/* dB values for 16bit PCM */
const float M10dB_PCM = 10362.0f;   
const float M20dB_PCM = 3277.0f;
const float M25dB_PCM = 1843.0f;
const float M30dB_PCM = 1026.0f;
const float M35dB_PCM = 583.0f;
const float M40dB_PCM = 328.0f;   
const float M45dB_PCM = 184.0f;
const float M50dB_PCM = 104.0f;
const float M55dB_PCM = 58.0f;
const float M60dB_PCM = 33.0f;

const float MAXPCM = 32767.0f;

/* Design constants (Change to fine tune the algorithms */

/* For Normalized Least Means Square - Pre-whitening */
#define NLMS_LEN  (240*8)      	  /* maximum NLMS filter length in taps */
//#define NLMS_LEN  (30*8)      	  /* maximum NLMS filter length in taps */
const float PreWhiteAlphaTF = (4000.0f/8000.0f);   /* FIR controls Transfer Frequency */

/* for Geigel Double Talk Detector */
const float GeigelThreshold = M3dB;
const int Thold = 30*8;       	      	  /* DTD hangover in taps */
const float UpdateThreshold = M30dB_PCM;

/* for Non Linear Processor */
const float NLPAttenuation = M6dB;

/* Below this line there are no more design constants */


/* Exponential Smoothing or IIR Infinite Impulse Response Filter */
class IIR_HP {
  float lowpassf;
  float alphaTF;  /* controls Transfer Frequency */

public:
  IIR_HP() {
    lowpassf = 0.0f;
    alphaTF = 0.0f;
  }
  
  void init(float alphaTF_) {
    alphaTF = alphaTF_;
  }

  float highpass(float in) {
    /* Highpass = Signal - Lowpass. Lowpass = Exponential Smoothing */
    lowpassf += alphaTF*(in - lowpassf);
    return in - lowpassf;
  }
};


#define POL   	6     	/* -6dB attenuation per octave per Pol */

class IIR_HP6 {
  float lowpassf[2*POL+1];
  float highpassf[2*POL+1];

public:
  IIR_HP6();
  float highpass(float in) {
    const float AlphaHp6 = 0.075; /* controls Transfer Frequency */
    const float Gain6   = 1.45f;  /* gain to undo filter attenuation */

    highpassf[0] = in;
    int i;
    for (i = 0; i < 2*POL; ++i) {
      /* Highpass = Signal - Lowpass. Lowpass = Exponential Smoothing */
      lowpassf[i+1] += AlphaHp6*(highpassf[i] - lowpassf[i+1]);
      highpassf[i+1] = highpassf[i] - lowpassf[i+1];
    }
    return Gain6*highpassf[2*POL];   
  }
};

 /* Recursive single pole FIR Finite Impulse response filter */
class FIR1 {
  float a0, a1, b1;
  float last_in, last_out;
  
public:
  FIR1();
	void init(float preWhiteTransferAlpha);
  float highpass(float in)  {
    float out = a0 * in + a1 * last_in + b1 * last_out;
    last_in = in;
    last_out = out;

    return out;
  }
};

 
#define NLMS_EXT  (10*8)    // Extention in taps to reduce mem copies
#define DTD_LEN 16          // block size in taps to optimize DTD calculation


class AEC {
  // Time domain Filters
  IIR_HP6 hp0, hp2;              // 300Hz cut-off Highpass
  IIR_HP hp1;               // DC-level remove Highpass)
  FIR1 Fx, Fe;              // pre-whitening Highpass for x, e
    
  // Geigel DTD (Double Talk Detector)
  float max_max_x;                // max(|x[0]|, .. |x[L-1]|)
  int hangover;
  float max_x[NLMS_LEN/DTD_LEN];  // optimize: less calculations for max()
  int dtdCnt;                     
  int dtdNdx;
  
  // NLMS-pw
  float x[NLMS_LEN+NLMS_EXT]; 	  // tap delayed loudspeaker signal
  float xf[NLMS_LEN+NLMS_EXT];    // pre-whitening tap delayed signal
  float w[NLMS_LEN];	      	    // tap weights
  int j;      	      	      	  // optimize: less memory copies
  int lastupdate;     	      	  // optimize: iterative dotp(x,x)
  double dotp_xf_xf;              // double to avoid loss of precision
  
public:
  AEC();
  
/* Geigel Double-Talk Detector
 *
 * in d: microphone sample (PCM as floating point value)
 * in x: loudspeaker sample (PCM as floating point value)
 * return: 0 for no talking, 1 for talking
 */
  int dtd(float d, float x);
  
/* Normalized Least Mean Square Algorithm pre-whitening (NLMS-pw)
 * The LMS algorithm was developed by Bernard Widrow
 * book: Widrow/Stearns, Adaptive Signal Processing, Prentice-Hall, 1985
 *
 * in mic: microphone sample (PCM as floating point value)
 * in spk: loudspeaker sample (PCM as floating point value)
 * in update: 0 for convolve only, 1 for convolve and update 
 * return: echo cancelled microphone sample
 */
  float nlms_pw(float mic, float spk, int update);
  
/* Acoustic Echo Cancellation and Suppression of one sample
 * in   d:  microphone signal with echo
 * in   x:  loudspeaker signal
 * return:  echo cancelled microphone signal
 */
  int AEC::doAEC(int d, int x);
};

#define _AEC_H
#endif
