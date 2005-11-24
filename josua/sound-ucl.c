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

#ifdef UCL_SUPPORT
#if !defined(__APPLE_CC__) && !defined(WIN32)

#include <osip2/osip_mt.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#define AUDIO_DEVICE "/dev/dsp"

#define USE_PCM

static int min_size = 0;

static void sdes_print (struct rtp *session, uint32_t ssrc, rtcp_sdes_type stype);
static void packet_print (struct rtp *session, rtp_packet * p);
static void rtp_event_handler (struct rtp *session, rtp_event * e);

static void
sdes_print (struct rtp *session, uint32_t ssrc, rtcp_sdes_type stype)
{
  const char *sdes_type_names[] = {
    "end", "cname", "name", "email", "telephone",
    "location", "tool", "note", "priv"
  };
  const uint8_t n = sizeof (sdes_type_names) / sizeof (sdes_type_names[0]);

  if (stype > n)
    {
      /* Theoretically impossible */
      fprintf (stderr, "boo! invalud sdes field %d\n", stype);
      return;
    }

  fprintf (stderr, "SSRC 0x%08x reported SDES type %s - ", ssrc,
           sdes_type_names[stype]);

  if (stype == RTCP_SDES_PRIV)
    {
      /* Requires extra-handling, not important for example */
      fprintf (stderr, "don't know how to display.\n");
  } else
    {
      fprintf (stderr, "%s\n", rtp_get_sdes (session, ssrc, stype));
    }
}

static void
packet_print (struct rtp *session, rtp_packet * p)
{
  call_t *ca = (call_t *) rtp_get_userdata (session);

#ifdef USE_PCM
  char data_in_dec[1280];
#endif
  fprintf (stderr, "Received data (payload %d timestamp %06d size %d)\n",
           p->pt, p->ts, p->data_len);
  if (ca == NULL)
    {
      fprintf (stderr, "No call for current session\n");
      return;
    }
#ifdef USE_PCM

  if (p->pt == 8)               /* A-Law */
    alaw_dec (p->data, data_in_dec, p->data_len);
  if (p->pt == 0)               /* Mu-Law */
    mulaw_dec (p->data, data_in_dec, p->data_len);

  write (ca->fd, data_in_dec, p->data_len * 2);

#else
  write (ca->fd, p->data, p->data_len);
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                          "received %i data written to the audio driver\n",
                          p->data_len));
#endif

  /*Unless filtering is enabled we are likely to see
     out packets if sending to a multicast group. */
  /*
     if (p->ssrc == rtp_my_ssrc(session)) {
     fprintf(stderr, "that I just sent.\n");
     } else {
     fprintf(stderr, "from SSRC 0x%08x\n", p->ssrc); 
     } 
   */
}

static void
rtp_event_handler (struct rtp *session, rtp_event * e)
{
  rtp_packet *p;
  rtcp_sdes_item *r;

  switch (e->type)
    {
      case RX_RTP:
        p = (rtp_packet *) e->data;
        packet_print (session, p);
        free (p);               /* xfree() is mandatory to release RTP packet data */
        break;
      case RX_SDES:
        r = (rtcp_sdes_item *) e->data;
        sdes_print (session, e->ssrc, r->type);
        break;
      case RX_BYE:
        break;
      case SOURCE_CREATED:
        fprintf (stderr, "New source created, SSRC = 0x%08x\n", e->ssrc);
        break;
      case SOURCE_DELETED:
        fprintf (stderr, "Source deleted, SSRC = 0x%08x\n", e->ssrc);
        break;
      case RX_SR:
      case RX_RR:
      case RX_RR_EMPTY:
      case RX_RTCP_START:
      case RX_RTCP_FINISH:
      case RR_TIMEOUT:
      case RX_APP:
        break;
    }
  fflush (stdout);
}

void *
os_sound_start_thread (void *_ca)
{
  int i;
  call_t *ca = (call_t *) _ca;
  struct timeval timeout;
  uint32_t rtp_ts, round;
  uint8_t mulaw_buffer[MULAW_BYTES * 16];
  int mulaw_buffer_pos;

#ifdef USE_PCM
  char data_out[MULAW_BYTES * 2 * 16];
#endif

  fprintf (stderr, "Sending and listening to ");
  fprintf (stderr, "%s port %d (local SSRC = 0x%08x)\n",
           rtp_get_addr (ca->rtp_session),
           rtp_get_rx_port (ca->rtp_session), rtp_my_ssrc (ca->rtp_session));

  round = 0;
  mulaw_buffer_pos = 0;

  while (ca->enable_audio != -1)
    {
      struct timeval t_beg;
      struct timeval t_end;
      struct timeval interval;

      gettimeofday (&t_beg, NULL);

      round++;
      /* original line rtp_ts = round * MULAW_MS; */
      rtp_ts = round * MULAW_BYTES;

      if (ca->local_sendrecv != _RECVONLY)
        {
          if (mulaw_buffer_pos < MULAW_BYTES * 4)
            {
              /* Send control packets */
              rtp_send_ctrl (ca->rtp_session, rtp_ts, NULL);

              /* Send data packets */
#ifdef USE_PCM
              i = read (ca->fd, data_out, MULAW_BYTES * 4);
              if (ca->local_sendrecv == _SENDRECV
                  || ca->local_sendrecv == _RECVONLY)
                {
              } else
                {
                  if (i < MULAW_BYTES * 4)
                    {
                      memset (data_out, 0, MULAW_BYTES * 4);
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
                                       "Could not open wav file: %s\n",
                                       ca->wav_file));
                          ca->fd = -1;
                          break;
                        }
                      i = read (ca->fd, data_out, MULAW_BYTES * 4);
                    }
                }
              if (ca->payload == 8)     /* A-Law */
                alaw_enc (data_out, mulaw_buffer + mulaw_buffer_pos, i);
              if (ca->payload == 0)     /* Mu-Law */
                mulaw_enc (data_out, mulaw_buffer + mulaw_buffer_pos, i);
              i = i / 2;
              mulaw_buffer_pos = mulaw_buffer_pos + i;
#else
              memset (mulaw_buffer + mulaw_buffer_pos, 0, MULAW_BYTES * 2);
              i = read (ca->fd, mulaw_buffer + mulaw_buffer_pos, MULAW_BYTES * 2);      /* ?? */
              mulaw_buffer_pos = mulaw_buffer_pos + i;
#endif
            }

          i = 0;
          if (mulaw_buffer_pos >= MULAW_BYTES)
            {
              i = rtp_send_data (ca->rtp_session, rtp_ts, ca->payload,
                                 0, 0, 0, (char *) mulaw_buffer, MULAW_BYTES,
                                 0, 0, 0);
              memmove (mulaw_buffer, mulaw_buffer + MULAW_BYTES, MULAW_BYTES * 15);
              mulaw_buffer_pos = mulaw_buffer_pos - MULAW_BYTES;
            }
      } else
        {
          /* Send control packets */
          rtp_send_ctrl (ca->rtp_session, rtp_ts, NULL);
        }

      /* Receive control and data packets */
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      i = rtp_recv (ca->rtp_session, &timeout, rtp_ts);
      /* State maintenance */
      rtp_update (ca->rtp_session);

      gettimeofday (&t_end, NULL);

      /* make a diff between t_beg and t_end */
      interval.tv_sec = t_end.tv_sec - t_beg.tv_sec;
      interval.tv_usec = t_end.tv_usec - t_beg.tv_usec;

      if (interval.tv_usec < 0)
        {
          interval.tv_usec += 1000000L;
          --interval.tv_sec;
        }
      interval.tv_usec = (MULAW_MS * 1000 - interval.tv_usec);
      if (interval.tv_usec < 0)
        {
          interval.tv_usec += 1000000L;
          --interval.tv_sec;
        }
      if (interval.tv_sec < 0)
        {
          interval.tv_sec = 0;
          interval.tv_usec = 0;
        }
      select (0, NULL, NULL, NULL, &interval);
    }
  return NULL;
}

int
os_sound_init ()
{
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

#ifdef USE_PCM
      p = AFMT_S16_NE;          /* choose LE or BE (endian) */
      ioctl (ca->fd, SNDCTL_DSP_SETFMT, &p);
#else
      if (ca->payload == 0)
        p = AFMT_MU_LAW;
      else if (ca->payload == 8)
        p = AFMT_A_LAW;
      else if (ca->payload == 110 || ca->payload == 111)
        p = AFMT_S16_NE;        /* choose LE or BE (endian) */
      ioctl (ca->fd, SNDCTL_DSP_SETFMT, &p);
#endif

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

              /* fprintf(stderr, "SUB_DIVIDE said error=%i,errno=%i\n",i,errno); */
              if ((i == 0) || (p == 1))
                cond = 0;
              else
                p = p / 2;
            }
        }
      ioctl (ca->fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
      if (min_size > blocksize)
        {
          fprintf (stderr, "dsp block size set to %i.", min_size);
          exit (0);
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
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting sendrecv session\n"));
  } else if (ca->local_sendrecv == _SENDONLY)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting sendonly session\n"));
  } else
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "starting recvonly session\n"));
    }

  ca->rtp_session = rtp_init (ca->remote_sdp_audio_ip,
                              port,
                              ca->remote_sdp_audio_port,
                              16, 64000, rtp_event_handler, (void *) ca);

  if (ca->rtp_session)
    {
      const char *username = "jack";    /* should be taken from the SDP */
      const char *telephone = "0033-MY-NUMBER";
      const char *app_name = "josua";
      uint32_t my_ssrc = rtp_my_ssrc (ca->rtp_session);

      /* set local participant info */
      rtp_set_sdes (ca->rtp_session, my_ssrc, RTCP_SDES_NAME,
                    username, strlen (username));
      rtp_set_sdes (ca->rtp_session, my_ssrc, RTCP_SDES_PHONE,
                    telephone, strlen (telephone));
      rtp_set_sdes (ca->rtp_session, my_ssrc, RTCP_SDES_TOOL,
                    app_name, strlen (app_name));

      /* Filter out local packets if requested */
      /* rtp_set_option(ca->rtp_session, RTP_OPT_FILTER_MY_PACKETS, filter_me); */
      rtp_set_option (ca->rtp_session, RTP_OPT_WEAK_VALIDATION, TRUE);

      ca->enable_audio = 1;
      ca->audio_thread = osip_thread_create (20000, os_sound_start_thread, ca);
  } else
    {
      fprintf (stderr, "Could not initialize session for %s port %d\n",
               ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port);
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
      /* Say bye-bye */
      rtp_send_bye (ca->rtp_session);
      rtp_done (ca->rtp_session);

      ca->rtp_session = NULL;
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
#endif
