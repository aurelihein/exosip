/*
 * sh_phone - Shell Phone
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This software is protected by copyright. For more information
 * write to <jack@atosc.org>.
 */

#include "jcalls.h"

#ifdef ORTP_SUPPORT

extern char _localip[30];

#include <osip2/osip_mt.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#define AUDIO_DEVICE "/dev/dsp"

int fd = -1;
static int min_size = 0;

#ifdef SPEEX_SUPPORT	  
void *os_sound_start_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_in[160];
  short sp_data_in_s[640];
  float sp_data_in_f[640];
  int have_more;
  int timestamp = 0;
  int i;

  while (ca->enable_audio != -1)
    {
      memset(data_in, 0, 160);
      /* receive ONE packet */
      i = rtp_session_recv_with_ts(ca->rtp_session, data_in, 160, timestamp, &have_more);

      speex_bits_reset(&ca->dec_speex_bits);
      speex_bits_read_from(&ca->dec_speex_bits, data_in, i);

      while(1)
	{
	  int k;
	  k = speex_decode(ca->speex_dec, &ca->dec_speex_bits, sp_data_in_f);
	  if (k==-2)
	    {
	      exit(0);
	    }
	  else if (k==-1)
	    {
	      break;
	    }
	  else if (k==0)
	    {
	      int j;
	      for (j=0;j<ca->speex_fsize;j++)
		{
		  if (sp_data_in_f[j]>32767)
		    sp_data_in_f[j]=32767;
		  if (sp_data_in_f[j]<-32767)
		    sp_data_in_f[j]=-32767;
		  sp_data_in_s[j] = (short) sp_data_in_f[j];
		}
	      write(fd, sp_data_in_s, ca->speex_fsize);
	    }
	}
      timestamp += 160;
    }
  return NULL;
}

#else

void
*os_sound_start_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_in[160];
  int have_more;
  int timestamp = 0;
  int i;
  while (ca->enable_audio != -1)
    {
      memset(data_in, 0, 160);
      i = rtp_session_recv_with_ts(ca->rtp_session, data_in, 160, timestamp, &have_more);
      if (i>0) {
	write(fd, data_in, i);
      }
      timestamp += 160;
    }
  return NULL;
}

#endif

#ifdef SPEEX_SUPPORT	  
void
*os_sound_start_out_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_out[10000];
  short sp_data_out_s[640];
  float sp_data_out_f[640];
  int timestamp = 0;
  int i;
  while (ca->enable_audio != -1)
    {
      int k;
      speex_bits_reset(&ca->speex_bits);
      for (k=0; k<ca->speex_nb_packet;k++)
	{
	  int j;
	  i=read(fd, sp_data_out_s, ca->speex_fsize * sizeof(short));
	  if (i>0)
	    {	  
	      for (j=0; j<ca->speex_fsize;j++)
		{ /* convert to float */
		  sp_data_out_f[j] = sp_data_out_s[j];
		}
	      speex_encode(ca->speex_enc, sp_data_out_f, &ca->speex_bits);
	    }
	}    
      speex_bits_insert_terminator(&ca->speex_bits);

      /* convert to char */
      i = speex_bits_write(&ca->speex_bits, data_out, sizeof(data_out));
      
      rtp_session_send_with_ts(ca->rtp_session, data_out, i,timestamp);
      timestamp+=i;
    }
  return NULL;
}

#else /* not with speex */

void
*os_sound_start_out_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_out[10000];
  int timestamp = 0;
  int i;
  while (ca->enable_audio != -1)
    {
      memset(data_out, 0, min_size);
      i=read(fd, data_out, min_size);
      if (i>0)
        {
	  
	  rtp_session_send_with_ts(ca->rtp_session, data_out, i,timestamp);
	  timestamp+=i;
        }
    }
  return NULL;
}
#endif

int os_sound_init()
{
  ortp_init();
  ortp_scheduler_init();
  ortp_set_debug_file("oRTP",NULL);
#ifdef SPEEX_SUPPORT
  rtp_profile_set_payload(&av_profile,115,&lpc1015);
  rtp_profile_set_payload(&av_profile,110,&speex_nb);
  rtp_profile_set_payload(&av_profile,111,&speex_wb);
#endif
  return 0;
}

int os_sound_start(jcall_t *ca)
{
  int p,cond;
  int bits = 16;
  int stereo = 0;
  int rate = 8000;
  int blocksize = 512;

  fd=open(AUDIO_DEVICE, O_RDWR|O_NONBLOCK);
  if (fd<0) return -EWOULDBLOCK;
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

  ioctl(fd, SNDCTL_DSP_RESET, 0);
  
  p =  bits;  /* 16 bits */
  ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);
  
  p =  stereo;  /* number of channels */
  ioctl(fd, SNDCTL_DSP_CHANNELS, &p);
  
  p =  rate;  /* rate in khz*/
  ioctl(fd, SNDCTL_DSP_SPEED, &p);
  
  ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
  if (min_size>blocksize)
    {
      cond=1;
      p=min_size/blocksize;
      while(cond)
	{
	  int i=ioctl(fd, SNDCTL_DSP_SUBDIVIDE, &p);
	  /* printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno); */
	  if ((i==0) || (p==1)) cond=0;
	  else p=p/2;
	}
    }
  ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
  if (min_size>blocksize)
    {
      printf("dsp block size set to %i.",min_size);
      exit(0);
    }else{
      /* no need to access the card with less latency than needed*/
      min_size=blocksize;
    }

  printf("blocksize = %i\n", min_size);

  if (ca->payload==0)
    p =  AFMT_MU_LAW;  /* rate in khz*/
  else if (ca->payload==8)
    p = AFMT_A_LAW;
  else if (ca->payload==110||ca->payload==111)
    p = AFMT_S16_NE; /* choose LE or BE (endian) */
  ioctl(fd, SNDCTL_DSP_SETFMT, &p);
  
#ifdef SPEEX_SUPPORT
  {
    float vbr_qual;
    int value;
    int quality;
    ca->speex_enc  = speex_encoder_init(&speex_nb_mode); /* 8kHz */
    /* 16kHz speex_enc = speex_encoder_init(&speex_wb_mode);   */
    /* 32kHz speex_enc = speex_encoder_init(&speex_uwb_mode);  */
    ca->speex_dec  = speex_decoder_init(&speex_nb_mode);
    value = 1;
    speex_decoder_ctl(ca->speex_dec, SPEEX_SET_ENH, &value);
    quality = 8; /* 15kb */
    speex_encoder_ctl(ca->speex_enc, SPEEX_SET_QUALITY, &quality);
    /* ou bien le bit rate:
       value = 15000; // 15kb
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_BITRATE, &value);
    */
    /* silence suppression (VAD)
       value = 1; // 15kb
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_VAD, &value);
       Discontinuous transmission (DTX)
       value = 1; // 15kb
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_DTX, &value);
       
       Variable Bit Rate (VBR)
       value = 1; // 15kb
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_VBR, &value);
       vbr_qual = 5,0; // between 0 and 10
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_VBR_QUALITY, &vbr_qual);
       
       Average bit rate: (ABR)
       value = 15000; // 15kb
       speex_encoder_ctl(ca->speex_enc, SPEEX_SET_ABR, &value);
    */
    speex_encoder_ctl(ca->speex_enc, SPEEX_GET_FRAME_SIZE,
		      &ca->speex_fsize);
    
    ca->speex_nb_packet = 1;
    speex_bits_init(&(ca->speex_bits));
    speex_bits_init(&(ca->dec_speex_bits));
  }
#endif
  
  ca->rtp_session = rtp_session_new(RTP_SESSION_SENDRECV);
  rtp_session_set_scheduling_mode(ca->rtp_session, 1); /* yes */
  rtp_session_set_blocking_mode(ca->rtp_session, 1);
  
  rtp_session_set_profile(ca->rtp_session, &av_profile);
  rtp_session_set_jitter_compensation(ca->rtp_session, 60);
  rtp_session_set_local_addr(ca->rtp_session, _localip, 10500);
  rtp_session_set_remote_addr(ca->rtp_session,
			      ca->remote_sdp_audio_ip,
			      ca->remote_sdp_audio_port);
  rtp_session_set_payload_type(ca->rtp_session, ca->payload);
  rtp_session_signal_connect(ca->rtp_session, "telephone-event",
			     (RtpCallback)rcv_telephone_event, ca);
  
  /* enter a loop (thread?) to send AUDIO data with
     rtp_session_send_with_ts(ca->rtp_session, data, data_length, timestamp);
  */
  ca->audio_thread = osip_thread_create(20000,
					os_sound_start_thread, ca);
  ca->out_audio_thread = osip_thread_create(20000,
					    os_sound_start_out_thread, ca);
  return 0;
}

void os_sound_close(jcall_t *ca)
{
  osip_thread_join(ca->audio_thread);
  osip_free(ca->audio_thread);
  osip_thread_join(ca->out_audio_thread);
  osip_free(ca->out_audio_thread);
  rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
					    (RtpCallback)rcv_telephone_event);
  rtp_session_destroy(ca->rtp_session);

#ifdef SPEEX_SUPPORT
  speex_bits_destroy(&ca->speex_bits);
  speex_bits_destroy(&ca->dec_speex_bits);
  speex_encoder_destroy(&ca->speex_enc);
  speex_decoder_destroy(&ca->speex_dec);
#endif
  
  close(fd); /* close the sound card */
}

#endif
