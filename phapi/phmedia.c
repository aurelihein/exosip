/*
 * phmedia -  Phone Api media streamer
 *
 * Copyright (C) 2004 Vadim Lebedev <vadim@mbdsys.com>
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef ORTP_SUPPORT
#include <osip2/osip_mt.h>
#include <osip2/osip.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <ortp.h>
#include <telephonyevents.h>
#include "phcall.h"
#include "phmedia.h"
#include "phcodec.h"
#include "tonegen.h"

#define ph_printf  printf

#define USE_CODECS 1

#ifndef EMBED

#define DO_ECHO_CAN 1
#define AEC_BIS 1

#endif



#ifdef DO_ECHO_CAN

#define NO_ECHO__SUPPRESSOR 1	
#define abs(x) ((x>=0)?x:(-x))

#ifdef AEC_BIS
void create_AEC(void);
short do_AEC(short x, short y);
void kill_AEC(void);
#else
#include "mec3-float.h"
#endif

#define __ECHO_STATE_MUTE			(1 << 8)
#define ECHO_STATE_IDLE				(0)
#define ECHO_STATE_PRETRAINING		(1 | (__ECHO_STATE_MUTE))
#define ECHO_STATE_STARTTRAINING	(2 | (__ECHO_STATE_MUTE))
#define ECHO_STATE_AWAITINGECHO		(3 | (__ECHO_STATE_MUTE))
#define ECHO_STATE_TRAINING			(4 | (__ECHO_STATE_MUTE))
#define ECHO_STATE_ACTIVE			(5)
#define ZT_MAX_PRETRAINING   1000	/* 1000ms max pretraining time */

#endif

phcodec_t *ph_media_lookup_codec(int payload);

#if !defined(WIN32)
#define TYPE(val) .type = (val)
#define CLOCK_RATE(val) .clock_rate = (val)
#define BYTES_PER_SAMPLE(val) .bytes_per_sample = (val)
#define ZERO_PATTERN(val)   .zero_pattern = (val)
#define PATTERN_LENGTH(val) .pattern_length = (val)
#define NORMAL_BITRATE(val) .normal_bitrate = (val)
#define MIME_TYPE(val) .mime_type = (val)
#endif

struct phmstream
{
  int fd;
  int payload;
  int running;
  int min_size;
  struct osip_thread *audio_in_thread;
  struct osip_thread *audio_out_thread;
  RtpSession *rtp_session;
  phcall_t  *ca;
  void      (*dtmfCallback)(phcall_t *ca, int dtmf);
  phcodec_t *codec;
  void      *encoder_ctx;
  void      *decoder_ctx;

#ifdef DO_ECHO_CAN

#define MAX_FRAME_SIZE 160
#define PCM_TRACE_LEN 50*MAX_FRAME_SIZE   /* data are stored for 50 frames of 10ms */

#ifndef AEC_BIS
  echo_can_state_t* ec;
#endif
  int	echocancel;
  int 	echostate;	/* State of echo canceller */
  GMutex *synclock;
  GCond *sync_cond;
  char *pcm_sent;
  int pcm_need_resync; 
  int pcm_rd;
  int pcm_wr;
  int bytes_to_throw;
  int sent_cnt;
  int recv_cnt;
#endif

#define DTMFQ_MAX 32
  char  dtmfq_buf[DTMFQ_MAX];
  int   dtmfq_wr;
  int   dtmfq_rd;
  int   dtmfq_cnt;

  enum { DTMF_IDLE, DTMF_GEN, DTMF_SILENT } dtmfg_phase;
  int   dtmfg_len;
  struct dtmfgen dtmfg_ctx;
  GMutex *dtmfg_lock;

#define DTMF_LOCK(s) g_mutex_lock(s->dtmfg_lock)
#define DTMF_UNLOCK(s) g_mutex_unlock(s->dtmfg_lock)



  



};

typedef struct phmstream phmstream_t; 

# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \


#define AUDIO_DEVICE "/dev/dsp"


#define USE_PCM
#define MINIMIZE_COPYING 1
#define BLOCKING_MODE 1
#define SCHEDULING_MODE 1



void *ph_audio_out_thread(void *_p);
void *ph_audio_in_thread(void *_p);




static void 
ph_telephone_event(RtpSession *rtp_session, int event, phcall_t *ca) 
{
  static const char evttochar[] = "0123456789*#ABCD!";
  phmstream_t *stream = (phmstream_t *)ca->phstream;
  if (stream->dtmfCallback)
    stream->dtmfCallback(ca, evttochar[event]);

}



void 
dbgbpt1(void)
{
}
void 
dbgbpt2(void)
{
}

static void
tvsub(register struct timeval *out, register struct timeval *in)
{
	out->tv_usec -= in->tv_usec;

	while(out->tv_usec < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}

	out->tv_sec -= in->tv_sec;
}


#if USE_CODECS

phcodec_t *ph_media_lookup_codec(int payload)
{
  PayloadType *pt = rtp_profile_get_payload(&av_profile, payload);
  phcodec_t *codec = ph_codec_list;

  while(codec)
    {
      int mlen = strlen(codec->mime);
      if (!strncasecmp(codec->mime, pt->mime_type, mlen))
	return codec;
      codec = codec->next;
    }

  return 0;


}

#endif		   

#ifdef DO_ECHO_CAN

void do_echo_update(phmstream_t *s, char *data, int len);
void store_pcm(phmstream_t *s, char *buf, int len);
void echo_resync(phmstream_t *s);

/* should be called in case of transmit underrun */
void echo_resync(phmstream_t *s){
  g_mutex_lock(s->synclock);
  s->pcm_rd = s->pcm_wr = 0;
  s->sent_cnt = s->recv_cnt = 0;
  s->pcm_need_resync = 1;
  g_mutex_unlock(s->synclock);
}
void store_pcm(phmstream_t *s, char *buf, int len){
  int lg;
  struct audio_buf_info info;
  g_mutex_lock(s->synclock);
  if(len <= (PCM_TRACE_LEN - s->pcm_wr)){
    memcpy(s->pcm_sent + s->pcm_wr, buf, len);
    s->pcm_wr += len;
    if(s->pcm_wr == PCM_TRACE_LEN)
      s->pcm_wr = 0;
  }else{
    lg = PCM_TRACE_LEN - s->pcm_wr;
    memcpy(s->pcm_sent + s->pcm_wr, buf, lg);
    memcpy(s->pcm_sent, buf+lg, len-lg);
    s->pcm_wr = len-lg;
  }
  s->sent_cnt += len;
  if(s->pcm_need_resync && s->sent_cnt >= 512){
    ioctl(s->fd, SNDCTL_DSP_GETISPACE, &info);
    printf("EC:Resync OK %d bytes to throw\n", info.bytes);
    s->bytes_to_throw = info.bytes;
    s->pcm_need_resync = 0;
  }
  g_mutex_unlock(s->synclock);
}
void do_echo_update(phmstream_t *s, char *data, int length){
  int i, lg, len;
  short *p1, *p2;

  len = length;

  if(!s->pcm_need_resync){
    if(s->bytes_to_throw){
      if(s->bytes_to_throw >= len){
	s->bytes_to_throw -= len;
	printf("EC1:%d bytes thrown\n", len);
	return;
      }else{
	printf("EC2:%d bytes thrown\n", s->bytes_to_throw);
	len = length - s->bytes_to_throw;
	s->bytes_to_throw = 0;
      }
    }
    s->recv_cnt += len;
#if 1
  if(s->recv_cnt > s->sent_cnt){
    printf("\nUNDERRUN: %d %d\n", s->sent_cnt, s->recv_cnt);
    echo_resync(s);
    return;
  }
#endif
    p1 = (short *)(s->pcm_sent + s->pcm_rd);
    p2 = (short *)data;

    if((PCM_TRACE_LEN - s->pcm_rd) >= len){
      for (i=0; i<len/2; i++) {
#ifndef AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else
	*p2 = do_AEC(*p1++, *p2);
#endif
	p2++;
      }
      s->pcm_rd += len;
      if(s->pcm_rd == PCM_TRACE_LEN)
	s->pcm_rd = 0; 
    }else{
      lg = PCM_TRACE_LEN - s->pcm_rd;
      for (i=0; i<lg/2; i++) {
#ifndef AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else
	*p2 = do_AEC(*p1++, *p2);
#endif
	p2++;
      }
      p1 = (short *)(s->pcm_sent);
      for (i=0; i<(len - lg)/2; i++) {
#ifndef AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else
	*p2 = do_AEC(*p1++, *p2);
#endif
	p2++;
      }
      s->pcm_rd = len-lg;
    }
  }
}
#endif


void
*ph_audio_in_thread(void *p)
{
  phmstream_t *stream = (phmstream_t*)p;
  char data_in[512];
  char data_in_dec[1024];
  int have_more = 0;
  int timestamp = 0;
  int i;
  mblk_t *mp;
  struct timeval pkt_in_time, pkt_out_time, sleeptime;
  struct timespec sleepns;
  int ts_inc = 0;
  int framesize = 160;
  int needsleep;

#if USE_CODECS
  phcodec_t *codec = stream->codec;
  framesize = codec->encoded_framesize;
#endif

  ph_printf("rtp reading thread started\n");

  while (stream->running)
    {
     ts_inc = 0;
     needsleep = 1;

#if !MINIMIZE_COPYING
      do
	{
	  i = rtp_session_recv_with_ts(stream->rtp_session, data_in, framesize, timestamp, &have_more);
	  if (i>0) 
	    {
	      needsleep = 0; 
#if USE_CODECS
	      i = codec->decode(stream->decoder_ctx, data_in, i, data_in_dec, sizeof(data_in_dec));
	      if (i > 0)
	      {
		write(stream->fd, data_in_dec, i);
		ts_inc += i/2;
#ifdef DO_ECHO_CAN
		store_pcm(stream, data_in_dec, i);
#endif
	      }
#else
	      if (stream->payload==8) /* A-Law */
		alaw_dec(data_in, data_in_dec, i);
	      if (stream->payload==0) /* Mu-Law */
		mulaw_dec(data_in, data_in_dec, i);
	      write(stream->fd, data_in_dec, i*2);
#ifdef DO_ECHO_CAN
	      store_pcm(stream, data_in_dec, i*2);
#endif
#endif
	  }
	} while(have_more);
#else /* MINIMIZE_COPYING */
      ts_inc = 0;
      while ( (mp=rtp_session_recvm_with_ts(stream->rtp_session,timestamp))!=NULL) 
	{
	  int 	len=mp->b_cont->b_wptr-mp->b_cont->b_rptr;
	  needsleep = 0;
	  gettimeofday(&pkt_in_time, 0);
	  if (stream->running)
	    {

#if USE_CODECS
	    len = codec->decode(stream->decoder_ctx, mp->b_cont->b_rptr, len, data_in_dec, sizeof(data_in_dec));
	    if (len > 0)
	      {
	      write(stream->fd, data_in_dec, len);
	      ts_inc += len/2;
	      }
#ifdef DO_ECHO_CAN
	      store_pcm(stream, data_in_dec, len);
#endif
#else

	    if (stream->payload==8) /* A-Law */
	      alaw_dec(mp->b_cont->b_rptr, data_in_dec, len);
	    
	    if (stream->payload==0) /* Mu-Law */
	      mulaw_dec(mp->b_cont->b_rptr, data_in_dec, len);

	      write(stream->fd, data_in_dec, len*2);
	      ts_inc += len;
#ifdef DO_ECHO_CAN
	      store_pcm(stream, data_in_dec, len*2);
#endif

#endif
	    }
	  if (!stream->running)
	    dbgbpt1();
	  freemsg(mp);
	  gettimeofday(&pkt_out_time, 0);
	  /* compute the time we spent processing the packet */
	  tvsub(&pkt_out_time, &pkt_in_time);
	}

#endif

      sleeptime.tv_usec = 10000; sleeptime.tv_sec = 0;
      tvsub(&sleeptime, &pkt_out_time);
      TIMEVAL_TO_TIMESPEC(&sleeptime, &sleepns);

      if (needsleep)
	{
	  rtp_session_resync(stream->rtp_session, timestamp);
	}

      if (needsleep || !BLOCKING_MODE)
	if (stream->running)
	  nanosleep(&sleepns, 0);


#if 0
      if (!ts_inc)
	ts_inc = codec->decoded_framesize/2;  
#endif

      timestamp += ts_inc;
    }
  ph_printf("rtp reading thread stopping\n");
  dbgbpt2();
  return NULL;
}


/* 
 * mix a DTMF signal into the given signal buffer
 *
*/
void ph_generate_out_dtmf(phmstream_t *stream, short *signal, int siglen)
{
  int mixlen, mixn;

dtmf_again:      
  switch (stream->dtmfg_phase)
    {
    case DTMF_IDLE:
      /* if the DTMF queue is empty we do  nothing */
      if (!stream->dtmfq_cnt)
	break;

      /* start generating the requested tone */
      tg_dtmf_init(&stream->dtmfg_ctx, stream->dtmfq_buf[stream->dtmfq_rd++], 8000, 0);

      /* update queue pointers and state */
      DTMF_LOCK(stream);
      if (stream->dtmfq_rd >= DTMFQ_MAX)
	stream->dtmfq_rd = 0;
      stream->dtmfq_cnt--;
      stream->dtmfg_phase = DTMF_GEN;
      DTMF_UNLOCK(stream);

      /* we're going to generate 500msec of DTMF tone */
      stream->dtmfg_len = 8*500;

      // fall down

    case DTMF_GEN:

      mixlen = (siglen > stream->dtmfg_len) ? stream->dtmfg_len : siglen;

      for( mixn = 0;  mixn < mixlen; mixn++)
	signal[mixn] += tg_dtmf_next_sample(&stream->dtmfg_ctx);

      stream->dtmfg_len -= mixlen;

      /* if we didn't finish with the current dtmf digit yet,
	 we'll stay in the GEN state 
      */
      if (stream->dtmfg_len)
	break;

      /* we've done with the current digit, ensure we have 50msec before the next
	 DTMF digit 
      */
      stream->dtmfg_phase = DTMF_SILENT;
      stream->dtmfg_len =  50*8; 


      /* skip past processed part of signal */
      siglen -= mixlen;
      signal += mixlen;

      /* fall down */
    case DTMF_SILENT:

      mixlen = (siglen > stream->dtmfg_len) ? stream->dtmfg_len : siglen;
      stream->dtmfg_len -= mixlen;
      
      /* if we have more silence to generate, keep the SILENT state */
      if (stream->dtmfg_len)
	break;


      /* we can handle a next DTMF digit now */
      stream->dtmfg_phase = DTMF_IDLE;
      if (stream->dtmfq_cnt)
	{
	  signal += mixlen;
	  siglen -= mixlen;
	  goto dtmf_again;
	}
      break;

    }
}
  
void *
ph_audio_out_thread(void *_p)
{
  phmstream_t *stream = (phmstream_t *)_p;
  char data_out[1000];
  char data_out_enc[1000];
  int timestamp = 0;
  int i;
  int framesize = 320;

#if USE_CODECS
  phcodec_t *codec = stream->codec;

  framesize = codec->decoded_framesize;
#endif
  ph_printf("rtp writing thread started\n");

  while (stream->running)
    {
      i=read(stream->fd, data_out, framesize);

      if ((stream->dtmfg_phase != DTMF_IDLE) || (stream->dtmfq_cnt != 0))
	ph_generate_out_dtmf(stream, (short *) data_out, i/2);

      if (i>0)
        {
	  int enclen;
#ifdef DO_ECHO_CAN
	  do_echo_update(stream, data_out, i);
#endif

#if USE_CODECS
	  enclen = codec->encode(stream->encoder_ctx, data_out, framesize, data_out_enc, sizeof(data_out_enc));
	  rtp_session_send_with_ts(stream->rtp_session, data_out_enc, enclen, timestamp);
	  timestamp += i/2;

#else
	  if (stream->payload==8) /* A-Law */
	    alaw_enc(data_out, data_out_enc, i);
	  if (stream->payload==0) /* Mu-Law */
	    mulaw_enc(data_out, data_out_enc, i);
	  
	  rtp_session_send_with_ts(stream->rtp_session, data_out_enc, i/2, timestamp);
	  timestamp=timestamp+(i/2);
#endif

        }
    }
  ph_printf("rtp writing thread stopped\n");
  return NULL;
}

static PayloadType ilbc =
{
	TYPE( PAYLOAD_AUDIO_PACKETIZED),
	CLOCK_RATE(8000),
	BYTES_PER_SAMPLE( 0),
	ZERO_PATTERN(NULL),
	PATTERN_LENGTH( 0),
	NORMAL_BITRATE( 13330),
	MIME_TYPE ("ILBC")
};

int ph_media_init()
{
  static int first_time = 1;

#if USE_CODECS
  ph_media_codecs_init();
#endif

  if (first_time)
    {
      ortp_init();
      rtp_profile_set_payload(&av_profile,PH_MEDIA_DTMF_PAYLOAD, &telephone_event);
      rtp_profile_set_payload(&av_profile,PH_MEDIA_ILBC_PAYLOAD, &ilbc);

      ortp_scheduler_init();
      ortp_set_debug_file("oRTP", NULL);
      tg_init_sine_table();


    }
  return 0;
}


int ph_media_supported_payload(ph_media_payload_t *pt, const char *ptstring)
{
  PayloadType *rtppt;

  pt->number = rtp_profile_get_payload_number_from_rtpmap(&av_profile, ptstring);
  if (pt->number == -1)
    return 0;

  rtppt = rtp_profile_get_payload(&av_profile, pt->number);

  strncpy(pt->string, rtppt->mime_type, sizeof(pt->string));
  pt->rate = rtppt->clock_rate;
 
  return 1;
  
}



int ph_media_cleanup()
{
}


int ph_media_start(phcall_t *ca, int port, void (*dtmfCallback)(phcall_t *ca, int event), const char * deviceId)
{
  int p,cond;
  int bits = 16;
  int stereo = 0; /* 0 is mono */
  int rate = 8000;
  int blocksize = 512;
  int fd;
  int min_size = 512;
  RtpSession *session;
  phmstream_t *stream;
#if USE_CODECS
  phcodec_t *codec;
#endif
#ifdef DO_ECHO_CAN
  int taps=256;
#endif

  if (port == 0)
    return -1;


  if (ca->hasaudio)
    ph_media_stop(ca);

#if USE_CODECS

  codec = ph_media_lookup_codec(ca->payload);
  if (!codec)
    {
      return -1;
    }
#endif



  stream = (phmstream_t *)osip_malloc(sizeof(phmstream_t));
  memset(stream, 0, sizeof(*stream));

  fd=open(AUDIO_DEVICE, O_RDWR|O_NONBLOCK);
  if (fd<0) return -EWOULDBLOCK;
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

  ioctl(fd, SNDCTL_DSP_RESET, 0);
  
  p =  bits;  /* 16 bits */
  ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);
  
  p =  stereo;  /* number of channels */
  ioctl(fd, SNDCTL_DSP_CHANNELS, &p);
  
  p = AFMT_S16_NE; /* choose LE or BE (endian) */
  ioctl(fd, SNDCTL_DSP_SETFMT, &p);


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
	  /* ph_printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno); */
	  if ((i==0) || (p==1)) cond=0;
	  else p=p/2;
	}
    }
  ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
  if (min_size>blocksize)
    {
      ph_printf("dsp block size set to %i.",min_size);
      exit(0);
    }else{
      /* no need to access the card with less latency than needed*/
      min_size=blocksize;
    }

  ph_printf("blocksize = %i\n", min_size);

#if USE_CODECS

  stream->codec = codec;
  if (codec->encoder_init)
    stream->encoder_ctx = codec->encoder_init();

  if (codec->decoder_init)
  stream->decoder_ctx = codec->decoder_init();

#endif
  
  
  session = rtp_session_new(RTP_SESSION_SENDRECV);
  rtp_session_set_scheduling_mode(session, SCHEDULING_MODE); /* yes */
  rtp_session_set_blocking_mode(session, BLOCKING_MODE);
  
  rtp_session_set_profile(session, &av_profile);
  rtp_session_set_jitter_compensation(session, 60);
  rtp_session_set_local_addr(session, "0.0.0.0", port);
  rtp_session_set_remote_addr(session,
			      ca->remote_sdp_audio_ip,
			      ca->remote_sdp_audio_port);
  rtp_session_set_payload_type(session, ca->payload);
  rtp_session_signal_connect(session, "telephone-event",
			     (RtpCallback)ph_telephone_event, ca);


  
  ca->hasaudio = 1;
  ca->phstream = stream;
  stream->running = 1;
  stream->rtp_session = session;
  stream->payload = ca->payload;
  stream->fd = fd;
  stream->ca = ca;
  stream->dtmfCallback = dtmfCallback;

#ifdef DO_ECHO_CAN  
#ifdef AEC_BIS
  create_AEC();
#else
  stream->ec = echo_can_create(taps, 0);
  if(stream->ec == 0){
    ph_printf("Echo CAN creating failed\n");
  }
  if(stream->ec){
#endif
    stream->echocancel = taps;
    stream->pcm_rd = stream->pcm_wr = 0;
    stream->pcm_sent = malloc(PCM_TRACE_LEN);
    stream->sent_cnt = stream->recv_cnt = 0;
    if(stream->pcm_sent == 0){
      ph_printf("No memory for EC  %d\n", p);
    }
#ifndef AEC_BIS
  }
#endif
  stream->synclock = g_mutex_new();
  stream->sync_cond = g_cond_new();
  stream->pcm_need_resync = 1;
  ph_printf("Echo CAN created OK\n");
  stream->bytes_to_throw = 0;
#endif


  stream->dtmfg_lock = g_mutex_new();
  stream->dtmfq_cnt = 0;
  stream->dtmfg_phase = DTMF_IDLE;

  stream->audio_out_thread = osip_thread_create(20000,
					    ph_audio_out_thread, stream);
  stream->audio_in_thread = osip_thread_create(20000,
					ph_audio_in_thread, stream);
  ph_printf("PH stream init OK\n");
  return 0;
}

void ph_media_stop(phcall_t *ca)
{
  int i;
  phmstream_t *stream = (phmstream_t *) ca->phstream;


  ca->hasaudio = 0;
  ca->phstream = 0;

  stream->running = 0;
  osip_thread_join(stream->audio_in_thread);
  osip_free(stream->audio_in_thread);

  osip_thread_join(stream->audio_out_thread);
  osip_free(stream->audio_out_thread);

  rtp_session_signal_disconnect_by_callback(stream->rtp_session, "telephone-event",
					    (RtpCallback)ph_telephone_event);


  ortp_set_debug_file("oRTP", stdout); 
  ortp_global_stats_display();
  ortp_set_debug_file("oRTP", NULL); 

  rtp_session_destroy(stream->rtp_session);


#if USE_CODECS
  if (stream->codec->encoder_cleanup)
    stream->codec->encoder_cleanup(stream->encoder_ctx);
  if (stream->codec->decoder_cleanup)
    stream->codec->decoder_cleanup(stream->decoder_ctx);
#endif

#ifdef DO_ECHO_CAN
   printf("\nbytes echoed %d %d\n", stream->sent_cnt, stream->recv_cnt);

  free(stream->pcm_sent);

#ifndef AEC_BIS
  if(stream->ec)
    echo_can_free(stream->ec);
#else
  kill_AEC();
#endif

  g_mutex_free(stream->synclock);
  g_cond_free(stream->sync_cond);

#endif

  g_mutex_free(stream->dtmfg_lock);

  close(stream->fd); /* close the sound card */

  osip_free(stream);
}


int ph_media_send_dtmf(phcall_t *ca, int dtmf, int mode)
{
  phmstream_t *stream = (phmstream_t *)(ca->phstream);
  unsigned long ts;

  if (!stream)
    return -1;

#if 0
  ts = rtp_session_get_current_send_ts(stream->rtp_session);
  rtp_session_send_dtmf(stream->rtp_session, dtmf, ts);
  return 0;
#else
  DTMF_LOCK(stream);
  if (stream->dtmfq_cnt < DTMFQ_MAX)
    {
      stream->dtmfq_buf[stream->dtmfq_wr++] = (char) dtmf;
      if (stream->dtmfq_wr == DTMFQ_MAX)
	stream->dtmfq_wr = 0;

      stream->dtmfq_cnt++;
      DTMF_UNLOCK(stream);
      return 0;
    }

  DTMF_UNLOCK(stream);
  return -1;
#endif
}


void ph_media_suspend(phcall_t *ca)
{
}



void ph_media_resume(phcall_t *ca)
{
}

#endif

