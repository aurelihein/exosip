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
#include <osip2/osip_mt.h>

#ifdef WIN32


#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>


WAVEFORMATEX wfx;

#define MAX_IN_BUFFERS 32
#define USED_IN_BUFFERS 6
#define MAX_OUT_BUFFERS 32
#define USED_OUT_BUFFERS 32


unsigned int waveoutDeviceID = WAVE_MAPPER;
WAVEHDR waveHdrOut[MAX_OUT_BUFFERS];
HWAVEOUT hWaveOut;
char dataBufferOut[MAX_OUT_BUFFERS][3200];

unsigned int waveinDeviceID = WAVE_MAPPER;
HWAVEIN hWaveIn;
WAVEHDR waveHdrIn[MAX_IN_BUFFERS];
char dataBufferIn[MAX_IN_BUFFERS][3200];

call_t *current_call = NULL;

static void CALLBACK
SpeakerCallback (HWAVEOUT _hWaveOut, UINT uMsg, DWORD dwInstance,
                 DWORD dwParam1, DWORD dwParam2)
{
  WAVEHDR *wHdr;

  switch (uMsg)
    {
      case WOM_OPEN:
        fprintf (stderr, "SpeakerCallback : WOM_OPEN\n");
        break;
      case WOM_CLOSE:
        fprintf (stderr, "SpeakerCallback : WOM_CLOSE\n");
        break;
      case WOM_DONE:
        fprintf (stderr, "SpeakerCallback : WOM_DONE\n");
        wHdr = (WAVEHDR *) dwParam1;

        break;
      default:
        break;
    }
}


static void CALLBACK
WaveInCallback (HWAVEIN hWaveIn, UINT uMsg, DWORD dwInstance, DWORD dwParam1,
                DWORD dwParam2)
{
  WAVEHDR *wHdr;
  MMRESULT mr = NOERROR;

  switch (uMsg)
    {
      case WIM_OPEN:
/*	!	fprintf(stderr, "WaveInCallback : WIM_OPEN\n"); */
        break;
      case WIM_CLOSE:
/*	!	fprintf(stderr, "WaveInCallback : WIM_CLOSE\n"); */
        break;
      case WIM_DATA:
        wHdr = (WAVEHDR *) dwParam1;
/* !	fprintf(stderr, "WaveInCallback : WIM_DATA\n"); */
        if (current_call == NULL)
          return;
        /* fprintf(stderr, received DATA from MIC: %s\n", wHdr->lpData); */
/* !		fprintf(stderr, "wHdr->dwBytesRecorded: %i\n", wHdr->dwBytesRecorded); */
/* !		fprintf(stderr, "wHdr->dwUser         : %i\n", wHdr->dwUser); */
        mr =
          waveInAddBuffer (hWaveIn, &(waveHdrIn[wHdr->dwUser]),
                           sizeof (waveHdrIn[wHdr->dwUser]));
        if (mr != MMSYSERR_NOERROR)
          {
/* !			fprintf(stderr, "__call_free: waveInAddBuffer: 0x%i\n", mr); */
            /* exit(-1); */
        } else
          {
            static int timestamp = 0;

            if (current_call)
              {
                rtp_session_send_with_ts (current_call->rtp_session,
                                          wHdr->lpData, wHdr->dwBytesRecorded,
                                          timestamp);
                timestamp += wHdr->dwBytesRecorded;
              }
          }
        break;
      default:
        break;
    }
}

LPGSM610WAVEFORMAT gsmformat;

int g_g723Drivers = 0;
int g_gsmDrivers = 0;

BOOL CALLBACK
acmDriverEnumCallback (HACMDRIVERID hadid, DWORD dwInstance, DWORD fdwSupport)
{
  MMRESULT mmr;
  HACMDRIVER driver;
  int i;

  ACMDRIVERDETAILS details;

  details.cbStruct = sizeof (ACMDRIVERDETAILS);
  mmr = acmDriverDetails (hadid, &details, 0);

  mmr = acmDriverOpen (&driver, hadid, 0);

  for (i = 0; i < details.cFormatTags; i++)
    {
      ACMFORMATTAGDETAILS fmtDetails;

      ZeroMemory (&fmtDetails, sizeof (fmtDetails));
      fmtDetails.cbStruct = sizeof (ACMFORMATTAGDETAILS);
      fmtDetails.dwFormatTagIndex = i;
      mmr = acmFormatTagDetails (driver, &fmtDetails, ACM_FORMATTAGDETAILSF_INDEX);
      if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC)
        {
          /* fprintf(stderr, "supported codec: %s\n" ,details.szLongName ); */
          if (fmtDetails.dwFormatTag == WAVE_FORMAT_MSG723)
            {
              g_g723Drivers = /* 1 */ 0;
/*	!		  fprintf(stderr, "Find Codec  %s\n" ,details.szLongName ); */
#if 0
              fprintf (stderr, "G723 cFormatTags %i\n", details.cFormatTags);
              fprintf (stderr, "G723 cFilterTags %i\n", details.cFilterTags);
#endif
              return -1;
          } else if (fmtDetails.dwFormatTag == WAVE_FORMAT_GSM610)
            {
              g_gsmDrivers = 1;
/* !			  fprintf(stderr, "Find Codec  %s\n" ,details.szLongName ); */
              return -1;
            }
      } else
        {
/* !		  fprintf(stderr, "Unsupported codec: %s\n" ,details.szLongName ); */
        }
    }
  mmr = acmDriverClose (driver, 0);
  return -1;
}

int
os_sound_init ()
{
  ortp_init ();
  ortp_scheduler_init ();
  ortp_set_debug_file ("oRTP", NULL);

  eXosip_sdp_negotiation_remove_audio_payloads ();

  g_g723Drivers = 0;
  g_gsmDrivers = 0;
  acmDriverEnum (acmDriverEnumCallback, 0, ACM_DRIVERENUMF_DISABLED);


  if (g_g723Drivers == 0)
    {
    }
/* !    fprintf(stderr, "No G723 decoders found!\n" ); */
  else
    {
#if 0
      eXosip_sdp_negotiation_add_codec (osip_strdup ("4"),
                                        NULL,
                                        osip_strdup ("RTP/AVP"),
                                        NULL, NULL, NULL,
                                        NULL, NULL, osip_strdup ("4 G723/8000"));
#endif
    }

  if (g_gsmDrivers == 0)
    {
      /* !   fprintf(stderr, "No GSM decoders found!\n" ); */
  } else
    {
      eXosip_sdp_negotiation_add_codec (osip_strdup ("3"),
                                        NULL,
                                        osip_strdup ("RTP/AVP"),
                                        NULL, NULL, NULL,
                                        NULL, NULL, osip_strdup ("3 GSM/8000"));
    }

  /* add A-Law & Mu-Law */
  eXosip_sdp_negotiation_add_codec (osip_strdup ("0"),
                                    NULL,
                                    osip_strdup ("RTP/AVP"),
                                    NULL, NULL, NULL,
                                    NULL, NULL, osip_strdup ("0 PCMU/8000"));

  eXosip_sdp_negotiation_add_codec (osip_strdup ("8"),
                                    NULL,
                                    osip_strdup ("RTP/AVP"),
                                    NULL, NULL, NULL,
                                    NULL, NULL, osip_strdup ("8 PCMA/8000"));
  return 0;
}

#if 0
for (i = 0; i < USED_IN_BUFFERS /* number of frame */ ; i++)
  {
    result =
      waveOutUnprepareHeader (hWaveOut, &(waveHdrIn[i]), sizeof (waveHdrIn[i]));
    if (result != MMSYSERR_NOERROR)
      {
        fprintf (stderr, "__call_free: waveOutUnprepareHeader: 0x%i\n", result);
      }
  }

result = waveOutClose (hWaveOut);
if (result != MMSYSERR_NOERROR)
  {
    fprintf (stderr, "__call_free: waveOutClose: 0x%i\n", result);
  }
#endif

int
open_sndcard (int format)
{
  MMRESULT mr = NOERROR;

  switch (format)
    {
      case WAVE_FORMAT_MULAW:
        wfx.wFormatTag = WAVE_FORMAT_MULAW;
        wfx.cbSize = 0;
        wfx.nAvgBytesPerSec = 8000;
        wfx.nBlockAlign = 1;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = 8000;
        wfx.wBitsPerSample = 8;
        break;
      case WAVE_FORMAT_ALAW:
        wfx.wFormatTag = WAVE_FORMAT_ALAW;
        wfx.cbSize = 0;
        wfx.nAvgBytesPerSec = 8000;
        wfx.nBlockAlign = 1;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = 8000;
        wfx.wBitsPerSample = 8;
        break;
      case WAVE_FORMAT_GSM610:
        gsmformat =
          GlobalAlloc (GMEM_MOVEABLE, (UINT) (sizeof (GSM610WAVEFORMAT)));
        gsmformat = (LPGSM610WAVEFORMAT) GlobalLock (gsmformat);

        gsmformat->wfx.wFormatTag = WAVE_FORMAT_GSM610;
        gsmformat->wfx.nChannels = 1;
        gsmformat->wfx.nSamplesPerSec = 8000;
        gsmformat->wfx.nAvgBytesPerSec = 1625;
        gsmformat->wfx.nBlockAlign = 65;
        gsmformat->wfx.wBitsPerSample = 0;
        gsmformat->wfx.cbSize = 2;
        gsmformat->wSamplesPerBlock = 320;
        break;
      case WAVE_FORMAT_MSG723:
#if 0
        g723format = (LPWAVEFORMATEX) LocalAlloc (LPTR, maxFormatSize);
        g723format->wFormatTag = WAVE_FORMAT_MSG723;
        g723format->cbSize = 0;
        g723format->nAvgBytesPerSec = 800;
        g723format->nBlockAlign = 1;
        g723format->nChannels = 1;
        g723format->nSamplesPerSec = 8000;
        g723format->wBitsPerSample = 8;
#endif
        wfx.wFormatTag = WAVE_FORMAT_MSG723;
        wfx.cbSize = 0;
        wfx.nAvgBytesPerSec = 800;
        wfx.nBlockAlign = 1;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = 8000;
        wfx.wBitsPerSample = 8;
        break;
        /*
           case WAVE_FORMAT_PCM:
           wfx.wFormatTag = WAVE_FORMAT_PCM;
           wfx.cbSize = 0;
           wfx.nAvgBytesPerSec = 16000;
           wfx.nBlockAlign = 2;
           wfx.nChannels = 1;
           wfx.nSamplesPerSec = 8000;
           wfx.wBitsPerSample = 16;
           break;
         */
      default:
        break;
    }

  if (gsmformat != NULL)
    mr =
      waveOutOpen (&hWaveOut, waveoutDeviceID,
                   (LPWAVEFORMATEX) & (gsmformat->wfx),
                   (DWORD) 0 /* SpeakerCallback */ , 0 /* arg */ ,
                   CALLBACK_NULL /* CALLBACK_FUNCTION */ );
  else
    mr =
      waveOutOpen (&hWaveOut, waveoutDeviceID, &wfx,
                   (DWORD) 0 /* SpeakerCallback */ , 0 /* arg */ ,
                   CALLBACK_NULL /* CALLBACK_FUNCTION */ );
  if (mr != NOERROR)
    {
/* !		fprintf(stderr, "__call_free: waveOutOpen: 0x%i\n", mr); */
      exit (-1);
      return -1;
  } else
    {
      int i;

      for (i = 0; i < USED_OUT_BUFFERS; i++)
        {
          memset (&(waveHdrOut[i]), 0, sizeof (waveHdrOut[i]));
          waveHdrOut[i].lpData = dataBufferOut[i];
          if (format == WAVE_FORMAT_GSM610)
            waveHdrOut[i].dwBufferLength = 65;  /* frameSize */
          else
            waveHdrOut[i].dwBufferLength = 320; /* frameSize */
          waveHdrOut[i].dwFlags = 0;
          waveHdrOut[i].dwUser = i;

          mr =
            waveOutPrepareHeader (hWaveOut, &(waveHdrOut[i]),
                                  sizeof (waveHdrOut[i]));
          if (mr != MMSYSERR_NOERROR)
            {
/* !				fprintf(stderr, "__call_free: waveOutPrepareHeader: 0x%i\n", mr); */
              exit (-1);
          } else
            {
            }
        }
    }

  if (gsmformat != NULL)
    mr =
      waveInOpen (&hWaveIn, waveinDeviceID,
                  (LPWAVEFORMATEX) & (gsmformat->wfx), (DWORD) WaveInCallback,
                  0, CALLBACK_FUNCTION);
  else
    mr =
      waveInOpen (&hWaveIn, waveinDeviceID, &wfx, (DWORD) WaveInCallback, 0,
                  CALLBACK_FUNCTION);
  if (mr != MMSYSERR_NOERROR)
    {
/* !		fprintf(stderr, "__call_free: waveInOpen: 0x%i\n", mr); */
      exit (-1);
      return -1;
  } else
    {
      int i;

      for (i = 0; i < MAX_IN_BUFFERS; i++)
        {
          waveHdrIn[i].lpData = dataBufferIn[i];
          waveHdrIn[i].dwBufferLength = 320;    /* frameSize */
          waveHdrIn[i].dwFlags = 0;
          waveHdrIn[i].dwUser = i;
          mr =
            waveInPrepareHeader (hWaveIn, &(waveHdrIn[i]), sizeof (waveHdrIn[i]));
          if (mr == MMSYSERR_NOERROR)
            {
              mr =
                waveInAddBuffer (hWaveIn, &(waveHdrIn[i]), sizeof (waveHdrIn[i]));
              if (mr == MMSYSERR_NOERROR)
                {
/* !					fprintf(stderr, "__call_free: waveInAddBuffer: 0x%i\n", mr); */
                  /* return -1; */
                }
          } else
            {
/* !				fprintf(stderr, "__call_free: waveInPrepareHeader: 0x%i\n", mr); */
              exit (-1);
              return -1;
            }
        }
      mr = waveInStart (hWaveIn);
      if (mr != MMSYSERR_NOERROR)
        {
/* !			fprintf(stderr, "__call_free: waveInStart: 0x%i\n", mr); */
          exit (-1);
          return -1;
        }
    }
  return 0;
}

void *
os_sound_start_thread (void *_ca)
{
  int pos_whdr = 0;
  call_t *ca = (call_t *) _ca;
  char data_in[3000];
  int have_more;
  int timestamp = 0;
  int i;

  current_call = ca;

  while (ca->enable_audio != -1)
    {
      int length;
      static int sound_played = 0;
      static int cpt = 0;

      memset (data_in, 0, 3000);
      if (ca->payload == 3)
        length = 260;           /* 280 ?? */
      else
        length = 160;

      i =
        rtp_session_recv_with_ts (ca->rtp_session, data_in, length, timestamp,
                                  &have_more);
      if (i > 0)
        {
          MMRESULT mr = NOERROR;

          memset (waveHdrOut[pos_whdr].lpData, 0, length);
          memcpy (waveHdrOut[pos_whdr].lpData, data_in, i);
          waveHdrOut[pos_whdr].dwBufferLength = i;
          mr =
            waveOutWrite (hWaveOut, &waveHdrOut[pos_whdr],
                          sizeof (waveHdrOut[pos_whdr]));

          pos_whdr++;
          if (pos_whdr == USED_OUT_BUFFERS)
            {
              pos_whdr = 0;     /* loop over the prepared blocks */
            }
          if (mr != MMSYSERR_NOERROR)
            switch (mr)
              {
                case MMSYSERR_INVALHANDLE:
                  fprintf (stderr,
                           "__call_free: waveOutWrite: 0x%i MMSYSERR_INVALHANDLE\n",
                           mr);
                  break;
                case MMSYSERR_NODRIVER:
                  fprintf (stderr,
                           "__call_free: waveOutWrite: 0x%i MMSYSERR_NODRIVER\n",
                           mr);
                  break;
                case MMSYSERR_NOMEM:
                  fprintf (stderr,
                           "__call_free: waveOutWrite: 0x%i MMSYSERR_NOMEM\n", mr);
                  break;
                case WAVERR_UNPREPARED:
                  fprintf (stderr,
                           "__call_free: waveOutWrite: 0x%i WAVERR_UNPREPARED\n",
                           mr);
                  break;
                case WAVERR_STILLPLAYING:
                  fprintf (stderr,
                           "__call_free: waveOutWrite: 0x%i WAVERR_STILLPLAYING\n",
                           mr);
                default:
                  fprintf (stderr, "__call_free: waveOutWrite: 0x%i\n", mr);
          } else
            ++sound_played;
          fprintf (stderr, "sound played = %i\n", sound_played);
          fprintf (stderr, "cpt = %i\n", ++cpt);
        }
      timestamp += length;
    }
  current_call = NULL;
  return NULL;
}

int
os_sound_start (call_t * ca, int port)
{
  int format = WAVE_FORMAT_MULAW;
  char localip[128];

  eXosip_guess_localip (AF_INET, localip, 128);

  if (ca->payload == 0)
    format = WAVE_FORMAT_MULAW;
  else if (ca->payload == 8)
    format = WAVE_FORMAT_ALAW;
  else if (ca->payload == 18)
    format = WAVE_FORMAT_G729A;
  else if (ca->payload == 4)
    format = WAVE_FORMAT_GSM610 /* WAVE_FORMAT_MSG723 */ ;
  else if (ca->payload == 3)
    format = WAVE_FORMAT_GSM610;
  else
    return -1;                  /* not supported codec?? */

  if (open_sndcard (format) != 0)
    return -1;

  ca->rtp_session = rtp_session_new (RTP_SESSION_SENDRECV);
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
  ca->audio_thread = osip_thread_create (20000, os_sound_start_thread, ca);
  return 0;
}

void
os_sound_close (call_t * ca)
{
  osip_thread_join (ca->audio_thread);
  osip_free (ca->audio_thread);
  rtp_session_signal_disconnect_by_callback (ca->rtp_session,
                                             "telephone-event",
                                             (RtpCallback) rcv_telephone_event);
  rtp_session_destroy (ca->rtp_session);

  waveInReset (hWaveIn);
  waveInClose (hWaveIn);
  waveOutReset (hWaveOut);
  waveOutClose (hWaveOut);
}

#endif
