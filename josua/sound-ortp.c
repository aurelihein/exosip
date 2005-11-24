/*
 * josua - Jack's open sip User Agent
 *
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

#include "jcalls.h"

#ifdef ORTP_SUPPORT

#include <osip2/osip_mt.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include <sys/time.h>
#include <fcntl.h>

# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \


#define AUDIO_DEVICE "/dev/dsp"

static int min_size = 0;

#ifdef SPEEX_SUPPORT
void *
os_sound_start_thread (void *_ca)
{
  call_t *ca = (call_t *) _ca;
  char data_in[160];
  short sp_data_in_s[640];
  float sp_data_in_f[640];
  int have_more;
  int timestamp = 0;
  int i;

  while (ca->enable_audio != -1)
    {
      memset (data_in, 0, 160);
      /* receive ONE packet */
      i =
        rtp_session_recv_with_ts (ca->rtp_session, data_in, 160, timestamp,
                                  &have_more);

      speex_bits_reset (&ca->dec_speex_bits);
      speex_bits_read_from (&ca->dec_speex_bits, data_in, i);

      while (1)
        {
          int k;

          k = speex_decode (ca->speex_dec, &ca->dec_speex_bits, sp_data_in_f);
          if (k == -2)
            {
              exit (0);
          } else if (k == -1)
            {
              break;
          } else if (k == 0)
            {
              int j;

              for (j = 0; j < ca->speex_fsize; j++)
                {
                  if (sp_data_in_f[j] > 32767)
                    sp_data_in_f[j] = 32767;
                  if (sp_data_in_f[j] < -32767)
                    sp_data_in_f[j] = -32767;
                  sp_data_in_s[j] = (short) sp_data_in_f[j];
                }
              write (ca->fd, sp_data_in_s, ca->speex_fsize);
            }
        }
      timestamp += 160;
    }
  return NULL;
}

#else
#define MINIMIZE_COPYING 1
#define BLOCKING_MODE 1

void
dbgbpt1 ()
{
}

void
dbgbpt2 ()
{
}

static void
tvsub (register struct timeval *out, register struct timeval *in)
{
  out->tv_usec -= in->tv_usec;

  while (out->tv_usec < 0)
    {
      --out->tv_sec;
      out->tv_usec += 1000000;
    }

  out->tv_sec -= in->tv_sec;
}


void *
os_sound_start_thread (void *_ca)
{
  call_t *ca = (call_t *) _ca;
  char data_in[512];
  char data_in_dec[1024];
  int have_more = 0;
  int timestamp = 0;
  int i;
  mblk_t *mp;
  struct timeval pkt_in_time, pkt_out_time, sleeptime;
  struct timespec sleepns;

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL, "oRTP thread started\n"));
  while (ca->enable_audio != -1)
    {
#if !MINIMIZE_COPYING
      do
        {
          memset (data_in, 0, 160);
          i =
            rtp_session_recv_with_ts (ca->rtp_session, data_in, 160,
                                      timestamp, &have_more);
          if (i > 0)
            {

              if (ca->payload == 8)     /* A-Law */
                alaw_dec (data_in, data_in_dec, 160);
              if (ca->payload == 0)     /* Mu-Law */
                mulaw_dec (data_in, data_in_dec, 160);

              write (ca->fd, data_in_dec, i * 2);
            }
        }
      while (have_more);
#else /* MINIMIZE_COPYING */


      if ((mp = rtp_session_recvm_with_ts (ca->rtp_session, timestamp)) != NULL)
        {
          int len = mp->b_cont->b_wptr - mp->b_cont->b_rptr;

          gettimeofday (&pkt_in_time, 0);
          if (ca->enable_audio != -1)
            {
              if (ca->payload == 8)     /* A-Law */
                alaw_dec (mp->b_cont->b_rptr, data_in_dec, len);

              if (ca->payload == 0)     /* Mu-Law */
                mulaw_dec (mp->b_cont->b_rptr, data_in_dec, len);

              write (ca->fd, data_in_dec, len * 2);
            }
          if (ca->enable_audio == -1)
            dbgbpt1 ();
          freemsg (mp);
          gettimeofday (&pkt_out_time, 0);
          /* compute the time we spent processing the packet */
          tvsub (&pkt_out_time, &pkt_in_time);
        }
#endif

      sleeptime.tv_usec = 10000;
      sleeptime.tv_sec = 0;
      tvsub (&sleeptime, &pkt_out_time);
      TIMEVAL_TO_TIMESPEC (&sleeptime, &sleepns);
#if !BLOCKING_MODE
      if (ca->enable_audio != -1)
        nanosleep (&sleepns, 0);
#endif
      timestamp += 160;
    }

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "rtp reading thread stopped\n"));
  dbgbpt2 ();
  return NULL;
}


#endif

#ifdef SPEEX_SUPPORT
void *
os_sound_start_out_thread (void *_ca)
{
  call_t *ca = (call_t *) _ca;
  char data_out[10000];
  short sp_data_out_s[640];
  float sp_data_out_f[640];
  int timestamp = 0;
  int i;

  while (ca->enable_audio != -1)
    {
      int k;

      speex_bits_reset (&ca->speex_bits);
      for (k = 0; k < ca->speex_nb_packet; k++)
        {
          int j;
          i = read (ca->fd, sp_data_out_s, ca->speex_fsize * sizeof (short));
          if (i > 0)
            {
              for (j = 0; j < ca->speex_fsize; j++)
                {               /* convert to float */
                  sp_data_out_f[j] = sp_data_out_s[j];
                }
              speex_encode (ca->speex_enc, sp_data_out_f, &ca->speex_bits);
            }
        }
      speex_bits_insert_terminator (&ca->speex_bits);

      /* convert to char */
      i = speex_bits_write (&ca->speex_bits, data_out, sizeof (data_out));

      rtp_session_send_with_ts (ca->rtp_session, data_out, i, timestamp);
      timestamp += i;
    }
  return NULL;
}

#else /* not with speex */

void *
os_sound_start_out_thread (void *_ca)
{
  call_t *ca = (call_t *) _ca;
  char data_out[1000];
  char data_out_enc[1000];
  int timestamp = 0;
  int i;
  int min_size = 160;

  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "rtp writing thread started\n"));
  while (ca->enable_audio != -1)
    {
      memset (data_out, 0, min_size * 2);
      i = read (ca->fd, data_out, min_size * 2);
      if (ca->local_sendrecv == _SENDRECV || ca->local_sendrecv == _RECVONLY)
        {
      } else
        {
          if (i < min_size * 2)
            {
              memset (data_out, 0, min_size * 2);
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "restarting: %i\n", i));
              close (ca->fd);
              /* send a wav file! */
              ca->fd = open (ca->wav_file, O_RDONLY);
              if (ca->fd < 0)
                {
                  OSIP_TRACE (osip_trace
                              (__FILE__, __LINE__, OSIP_INFO2, NULL,
                               "Could not open wav file: %s\n", ca->wav_file));
                  ca->fd = -1;
                  break;
                }
              i = read (ca->fd, data_out, min_size * 2);
            }
        }
      if (i > 0)
        {

          if (ca->payload == 8) /* A-Law */
            alaw_enc (data_out, data_out_enc, i);
          if (ca->payload == 0) /* Mu-Law */
            mulaw_enc (data_out, data_out_enc, i);

          rtp_session_send_with_ts (ca->rtp_session, data_out_enc, i / 2,
                                    timestamp);
          timestamp = timestamp + (i / 2);
        }
    }
  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "rtp writing thread stopped\n"));
  return NULL;
}
#endif

int
os_sound_init ()
{
  ortp_init ();
  ortp_scheduler_init ();
  ortp_set_debug_file ("oRTP", NULL);
#ifdef SPEEX_SUPPORT
  rtp_profile_set_payload (&av_profile, 115, &lpc1015);
  rtp_profile_set_payload (&av_profile, 110, &speex_nb);
  rtp_profile_set_payload (&av_profile, 111, &speex_wb);
#endif
  return 0;
}

int
os_sound_start (call_t * ca, int port)
{
  int p, cond;
  int bits = 16;
  int stereo = 0;               /* 0 is mono */
  int rate = 8000;
  int blocksize = 512;
  char localip[128];

  eXosip_guess_localip (AF_INET, localip, 128);

  if (port == 0)
    return -1;


  if (ca->local_sendrecv == _SENDRECV || ca->local_sendrecv == _RECVONLY)
    {
      ca->fd = open (AUDIO_DEVICE, O_RDWR | O_NONBLOCK);
      if (ca->fd < 0)
        return -EWOULDBLOCK;
      fcntl (ca->fd, F_SETFL, fcntl (ca->fd, F_GETFL) & ~O_NONBLOCK);

      ioctl (ca->fd, SNDCTL_DSP_RESET, 0);

      p = bits;                 /* 16 bits */
      ioctl (ca->fd, SNDCTL_DSP_SAMPLESIZE, &p);

      p = stereo;               /* number of channels */
      ioctl (ca->fd, SNDCTL_DSP_CHANNELS, &p);

      p = AFMT_S16_NE;          /* choose LE or BE (endian) */
      ioctl (ca->fd, SNDCTL_DSP_SETFMT, &p);

      p = rate;                 /* rate in khz */
      ioctl (ca->fd, SNDCTL_DSP_SPEED, &p);

      ioctl (ca->fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
      if (min_size > blocksize)
        {
          cond = 1;
          p = min_size / blocksize;
          while (cond)
            {
              int i = ioctl (ca->fd, SNDCTL_DSP_SUBDIVIDE, &p);

              /* printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno); */
              if ((i == 0) || (p == 1))
                cond = 0;
              else
                p = p / 2;
            }
        }
      ioctl (ca->fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
      if (min_size > blocksize)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "dsp block size set to %i.", min_size));
          close (ca->fd);
          return -1;
      } else
        {
          /* no need to access the card with less latency than needed */
          min_size = blocksize;
        }

      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "blocksize = %i\n", min_size));
  } else
    {
      /* send a wav file! */
      ca->fd = open (ca->wav_file, O_RDONLY);
      if (ca->fd < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "Could not open wav file: %s\n", ca->wav_file));
          return -1;
        }
    }

#ifdef SPEEX_SUPPORT
  {
    float vbr_qual;
    int value;
    int quality;

    ca->speex_enc = speex_encoder_init (&speex_nb_mode);        /* 8kHz */
    /* 16kHz speex_enc = speex_encoder_init(&speex_wb_mode);   */
    /* 32kHz speex_enc = speex_encoder_init(&speex_uwb_mode);  */
    ca->speex_dec = speex_decoder_init (&speex_nb_mode);
    value = 1;
    speex_decoder_ctl (ca->speex_dec, SPEEX_SET_ENH, &value);
    quality = 8;                /* 15kb */
    speex_encoder_ctl (ca->speex_enc, SPEEX_SET_QUALITY, &quality);
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
    speex_encoder_ctl (ca->speex_enc, SPEEX_GET_FRAME_SIZE, &ca->speex_fsize);

    ca->speex_nb_packet = 1;
    speex_bits_init (&(ca->speex_bits));
    speex_bits_init (&(ca->dec_speex_bits));
  }
#endif

  if (ca->local_sendrecv == _SENDRECV)
    {
      ca->rtp_session = rtp_session_new (RTP_SESSION_SENDRECV);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting sendrecv session\n"));
  } else if (ca->local_sendrecv == _SENDONLY)
    {
      ca->rtp_session = rtp_session_new (RTP_SESSION_SENDONLY);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting sendonly session\n"));
  } else
    {
      ca->rtp_session = rtp_session_new (RTP_SESSION_RECVONLY);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting recvonly session\n"));
    }

  rtp_session_set_scheduling_mode (ca->rtp_session, 1); /* yes */
  rtp_session_set_blocking_mode (ca->rtp_session, 1);

  rtp_session_set_profile (ca->rtp_session, &av_profile);
  rtp_session_set_jitter_compensation (ca->rtp_session, 60);
  rtp_session_set_local_addr (ca->rtp_session, localip, port);
  rtp_session_set_remote_addr (ca->rtp_session,
                               ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port);
  rtp_session_set_payload_type (ca->rtp_session, ca->payload);
  rtp_session_signal_connect (ca->rtp_session, "telephone-event",
                              (RtpCallback) rcv_telephone_event, ca);

  /* enter a loop (thread?) to send AUDIO data with
     rtp_session_send_with_ts(ca->rtp_session, data, data_length, timestamp);
   */
  ca->enable_audio = 1;
  if (ca->local_sendrecv == _SENDRECV || ca->local_sendrecv == _RECVONLY)
    {
      ca->audio_thread = osip_thread_create (20000, os_sound_start_thread, ca);
    }
  if (ca->local_sendrecv == _SENDRECV || ca->local_sendrecv == _SENDONLY)
    {
      ca->out_audio_thread = osip_thread_create (20000,
                                                 os_sound_start_out_thread, ca);
    }
  return 0;
}

void
os_sound_close (call_t * ca)
{
  if (ca->rtp_session != NULL)
    {
      ca->enable_audio = -1;
      osip_thread_join (ca->audio_thread);
      osip_free (ca->audio_thread);
      ca->audio_thread = NULL;
      osip_thread_join (ca->out_audio_thread);
      osip_free (ca->out_audio_thread);
      ca->out_audio_thread = NULL;
      rtp_session_signal_disconnect_by_callback (ca->rtp_session,
                                                 "telephone-event",
                                                 (RtpCallback)
                                                 rcv_telephone_event);
      rtp_session_destroy (ca->rtp_session);
    }
#ifdef SPEEX_SUPPORT
  speex_bits_destroy (&ca->speex_bits);
  speex_bits_destroy (&ca->dec_speex_bits);
  speex_encoder_destroy (&ca->speex_enc);
  speex_decoder_destroy (&ca->speex_dec);
#endif

  close (ca->fd);               /* close the sound card */
}

#endif
