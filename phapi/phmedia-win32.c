/*
 * phmededia - media streaming for ph API
 *
 * Copyright (C) 2004   Vadim Lebedev <vadim@mbdsys.com>
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

#ifdef WIN32

# include <osip2/osip_mt.h>
# include <osip2/osip.h>
# include <ortp.h>
# include <telephonyevents.h>

# include "phcall.h"
# include "phmedia.h"
# include "phcodec.h"
# include "tonegen.h"

# include <windows.h>
# include <mmsystem.h>
# include <mmreg.h>
# include <msacm.h>

# define MAX_IN_BUFFERS 32
# define USED_IN_BUFFERS 4
# define MAX_OUT_BUFFERS 32
# define USED_OUT_BUFFERS 32

# define TYPE(val) .type = (val)
# define CLOCK_RATE(val) .clock_rate = (val)
# define BYTES_PER_SAMPLE(val) .bytes_per_sample = (val)
# define ZERO_PATTERN(val)   .zero_pattern = (val)
# define PATTERN_LENGTH(val) .pattern_length = (val)
# define NORMAL_BITRATE(val) .normal_bitrate = (val)
# define MIME_TYPE(val) .mime_type = (val)

# define USE_CODECS 1

# define DO_ECHO_CAN 1

#define AEC_BIS 1

# if DO_ECHO_CAN

#  define NO_ECHO__SUPPRESSOR 1


#  if AEC_BIS
void create_AEC(void);
short do_AEC(short x, short y);
void kill_AEC(void);
#  else /* !AEC_BIS */
#   include "mec3-float.h"
#  endif /* !AEC_BIS */

#  define __ECHO_STATE_MUTE			(1 << 8)
#  define ECHO_STATE_IDLE				(0)
#  define ECHO_STATE_PRETRAINING		(1 | (__ECHO_STATE_MUTE))
#  define ECHO_STATE_STARTTRAINING	(2 | (__ECHO_STATE_MUTE))
#  define ECHO_STATE_AWAITINGECHO		(3 | (__ECHO_STATE_MUTE))
#  define ECHO_STATE_TRAINING			(4 | (__ECHO_STATE_MUTE))
#  define ECHO_STATE_ACTIVE			(5)
#  define ZT_MAX_PRETRAINING   1000	/* 1000ms max pretraining time */

#  define MAX_FRAME_SIZE 160
#  define PCM_TRACE_LEN 50*MAX_FRAME_SIZE   /* data are stored for 50 frames of 10ms */

# else

#define AEC_MUTEX_LOCK(s) 
#define AEC_MUTEX_UNLOCK(s) 

# endif /* !DO_ECHO_CAN */

# define DTMFQ_MAX 32

#define DTMF_LOCK(s) g_mutex_lock(s->dtmfg_lock)
#define DTMF_UNLOCK(s) g_mutex_unlock(s->dtmfg_lock)

unsigned int		waveoutDeviceID = WAVE_MAPPER;
unsigned int		waveinDeviceID = WAVE_MAPPER;

phcodec_t	*ph_media_lookup_codec(int payload);

struct							phmstream
{
  int								fd;
  int								payload;
  int								running;
  int								min_size;
  struct osip_thread		*audio_in_thread;
  struct osip_thread		*audio_out_thread;
  HANDLE						event;
  RtpSession					*rtp_session;
  WAVEHDR					waveHdrOut[MAX_OUT_BUFFERS];
  HWAVEOUT				hWaveOut;
  char							dataBufferOut[MAX_OUT_BUFFERS][512];
  HWAVEIN					hWaveIn;
  WAVEHDR					waveHdrIn[MAX_IN_BUFFERS];
  char							dataBufferIn[MAX_IN_BUFFERS][512];
  unsigned long				timestamp;
  void								(*dtmfCallback)(phcall_t *ca, int dtmf);
  phcodec_t					*codec;
  void								*encoder_ctx;
  void								*decoder_ctx;
#if DO_ECHO_CAN
  struct	timeval              last_audio_read;
  GMutex                      *aec_mutex;
#define AEC_MUTEX_LOCK(s) g_mutex_lock(s->aec_mutex)
#define AEC_MUTEX_UNLOCK(s) g_mutex_unlock(s->aec_mutex)

# if !AEC_BIS
  echo_can_state_t			*ec;
# endif /* !AEC_BIS */
  int								echocancel;
  int 								echostate;	/* State of echo canceller */
  GMutex						*synclock;
  GCond						*sync_cond;
  char							*pcm_sent;
  int								pcm_need_resync; 
  int								pcm_rd;
  int								pcm_wr;
  int								bytes_to_throw;
  int								sent_cnt;
  int								recv_cnt;
#endif /* ! DO_ECHO_CAN */
  char							dtmfq_buf[DTMFQ_MAX];
  int								dtmfq_wr;
  int								dtmfq_rd;
  int								dtmfq_cnt;
  enum {DTMF_IDLE,
			DTMF_GEN,
			DTMF_SILENT}	dtmfg_phase;
  int								dtmfg_len;
  struct dtmfgen				dtmfg_ctx;
  GMutex						*dtmfg_lock;
};


typedef  struct phmstream phmstream_t; 

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

/**
* @param deviceID string contaning the input and output device identifier
* example : "IN=2 OUT=1" ; "2" is input device identifier and "1" is ouput one
* @brief translate deviceID to init the wave in and wave out device identifier
*/
static void		ph_media_init_deviceID(const char *deviceID)
{
	const char	*inputDevice;
	const char *outputDevice;

	if (!deviceID || !*deviceID) {
		fprintf(stderr, "Bad device identifiers (%s)\n", deviceID);
		return;
	}
	inputDevice = strstr(deviceID, "IN=");
	outputDevice=strstr(deviceID, "OUT=");
	if (!inputDevice || !outputDevice || !*inputDevice || !*outputDevice)
	{
		fprintf(stderr, "Bad device identifiers (%s)\n", deviceID);
		return;
	}
	waveinDeviceID = atoi(inputDevice + 3);
	waveoutDeviceID = atoi(outputDevice + 4);
	if (waveinDeviceID >= waveInGetNumDevs())
		waveinDeviceID = WAVE_MAPPER;
	if (waveoutDeviceID >= waveOutGetNumDevs())
		waveoutDeviceID = WAVE_MAPPER;
}

#if USE_CODECS
phcodec_t			*ph_media_lookup_codec(int payload)
{
  PayloadType	*pt = rtp_profile_get_payload(&av_profile, payload);
  phcodec_t		*codec = ph_codec_list;
  int					mlen;

  while(codec)
    {
		mlen = strlen(codec->mime);
		if (!strnicmp(codec->mime, pt->mime_type, mlen))
			return codec;
      codec = codec->next;
    }
  return 0;
}
#endif /* !USE_CODECS */

static void CALLBACK SpeakerCallback(HWAVEOUT _hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR *wHdr;
	switch(uMsg) {
	case WOM_OPEN:
		fprintf(stderr, "SpeakerCallback : WOM_OPEN\n");
		break;
	case WOM_CLOSE:
		fprintf(stderr, "SpeakerCallback : WOM_CLOSE\n");
		break;
	case WOM_DONE:
		fprintf(stderr, "SpeakerCallback : WOM_DONE\n");
		wHdr = (WAVEHDR *)dwParam1;

		break;
	default:
		break;
	}
}


static void CALLBACK WaveInCallback(HWAVEIN hWaveIn, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR	*wHdr;
  	MMRESULT	mr = NOERROR;
	phmstream_t *stream = (phmstream_t *) dwInstance;
	int					framesize = 320;
	int					enclen;
	char				data_out_enc[1000];
#if USE_CODECS
	phcodec_t		*codec = stream->codec;

	framesize = codec->decoded_framesize;
#endif /* !USE_CODECS */
	switch(uMsg) {
	case WIM_OPEN:
/*	!	fprintf(stderr, "WaveInCallback : WIM_OPEN\n"); */
		break;
	case WIM_CLOSE:
/*	!	fprintf(stderr, "WaveInCallback : WIM_CLOSE\n"); */
		break;
	case WIM_DATA:
		wHdr = (WAVEHDR *)dwParam1;
/* !	fprintf(stderr, "WaveInCallback : WIM_DATA\n"); */
		if (!stream->running)
		  break;

#if USE_CODECS
		enclen = codec->encode(stream->encoder_ctx, wHdr->lpData, wHdr->dwBytesRecorded, data_out_enc, sizeof (data_out_enc));
		rtp_session_send_with_ts(stream->rtp_session, data_out_enc, enclen, stream->timestamp);
		stream->timestamp+=wHdr->dwBytesRecorded / 2;
#else
		rtp_session_send_with_ts(stream->rtp_session, wHdr->lpData, wHdr->dwBytesRecorded, stream->timestamp);
		stream->timestamp+=wHdr->dwBytesRecorded;
#endif /* !USE_CODECS */
		
		mr = waveInAddBuffer(hWaveIn, wHdr, sizeof(*wHdr));

		if (mr != MMSYSERR_NOERROR)
		{
/* !			fprintf(stderr, "__call_free: waveInAddBuffer: 0x%i\n", mr); */
			/* exit(-1); */
		}
		break;
	default:
		break;
	}
}

LPGSM610WAVEFORMAT  gsmformat;

int g_g723Drivers = 0;
int g_gsmDrivers = 0;

BOOL CALLBACK acmDriverEnumCallback( HACMDRIVERID hadid, DWORD dwInstance, DWORD fdwSupport ){
	MMRESULT mmr;
	HACMDRIVER driver;
	int i;

	ACMDRIVERDETAILS details;
	details.cbStruct = sizeof(ACMDRIVERDETAILS);
	mmr = acmDriverDetails( hadid, &details, 0 );

	mmr = acmDriverOpen( &driver, hadid, 0 );

	for(i = 0; i < details.cFormatTags; i++ ){
	  ACMFORMATTAGDETAILS fmtDetails;
	  ZeroMemory( &fmtDetails, sizeof(fmtDetails) );
	  fmtDetails.cbStruct = sizeof(ACMFORMATTAGDETAILS);
	  fmtDetails.dwFormatTagIndex = i;
	  mmr = acmFormatTagDetails( driver, &fmtDetails, ACM_FORMATTAGDETAILSF_INDEX );
	  if( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC ) {
		 /* fprintf(stderr, "supported codec: %s\n" ,details.szLongName ); */
	      if( fmtDetails.dwFormatTag == WAVE_FORMAT_MSG723 )
		  {
			  g_g723Drivers = /* 1 */ 0;
/*	!		  fprintf(stderr, "Find Codec  %s\n" ,details.szLongName ); */
#if 0
			  fprintf(stderr, "G723 cFormatTags %i\n" ,details.cFormatTags );
			  fprintf(stderr, "G723 cFilterTags %i\n" ,details.cFilterTags);
#endif
			  return -1;
		  }
	      else if( fmtDetails.dwFormatTag == WAVE_FORMAT_GSM610 )
		  {
			  g_gsmDrivers = 1;
/* !			  fprintf(stderr, "Find Codec  %s\n" ,details.szLongName ); */
			  return -1;
		  }
	  }
	  else
	  {
/* !		  fprintf(stderr, "Unsupported codec: %s\n" ,details.szLongName ); */
	  }
	}
	mmr = acmDriverClose( driver, 0 );
	return -1;
}

static PayloadType ilbc = 
{
	PAYLOAD_AUDIO_PACKETIZED,
	8000,
	0,
	NULL,
	0,
	13330,
	"ILBC"
};

void boostPriority(int tc)
{
#if 1
	SetThreadPriority(GetCurrentThread(), tc ? THREAD_PRIORITY_TIME_CRITICAL : THREAD_PRIORITY_HIGHEST );
#endif
}
int
ph_media_init()
{
  static int first_time = 1;

#if USE_CODECS
  ph_media_codecs_init();
#endif /* !USE_CODECS */

  if (!first_time)
    return 0;

  first_time = 0;
  ortp_init();
  rtp_profile_set_payload(&av_profile, PH_MEDIA_DTMF_PAYLOAD, &telephone_event);
  rtp_profile_set_payload(&av_profile, PH_MEDIA_ILBC_PAYLOAD, &ilbc);
  ortp_scheduler_init();
  ortp_set_debug_file("oRTP",NULL);
  tg_init_sine_table();

  //  eXosip_sdp_negotiation_remove_audio_payloads();

#if 0
  g_g723Drivers = 0;
  g_gsmDrivers  = 0;
  acmDriverEnum( acmDriverEnumCallback, 0, ACM_DRIVERENUMF_DISABLED );


  if(g_g723Drivers == 0)
  {
  }
/* !    fprintf(stderr, "No G723 decoders found!\n" ); */
  else
  {
#if 0
	  eXosip_sdp_negotiation_add_codec(osip_strdup("4"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("4 G723/8000"));
#endif
  }

  if(g_gsmDrivers == 0)
  {
 /* !   fprintf(stderr, "No GSM decoders found!\n" ); */
  }
  else
  {
	  eXosip_sdp_negotiation_add_codec(osip_strdup("3"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("3 GSM/8000"));
  }

  /* add A-Law & Mu-Law */
  eXosip_sdp_negotiation_add_codec(osip_strdup("0"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("0 PCMU/8000"));

  eXosip_sdp_negotiation_add_codec(osip_strdup("8"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("8 PCMA/8000"));
#endif

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

phmstream_t *
open_sndcard(int format, phcodec_t *codec, const char * deviceID)
{
  phmstream_t *stream;
  MMRESULT mr = NOERROR;
  WAVEFORMATEX		wfx;
  HWAVEIN hWaveIn;
  HWAVEOUT hWaveOut;

  stream = (phmstream_t*) osip_malloc(sizeof(phmstream_t));
  memset(stream, 0, sizeof(*stream));

#if USE_CODECS
	stream->codec = codec;
  if (codec->encoder_init)
    stream->encoder_ctx = codec->encoder_init();

  if (codec->decoder_init)
  stream->decoder_ctx = codec->decoder_init();
#endif /* !USE_CODECS */

  switch(format)
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
      gsmformat = GlobalAlloc(GMEM_MOVEABLE,(UINT)(sizeof(GSM610WAVEFORMAT)));
      gsmformat = (LPGSM610WAVEFORMAT)GlobalLock(gsmformat);
      
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
      wfx.wFormatTag = WAVE_FORMAT_MSG723;
      wfx.cbSize = 0;
      wfx.nAvgBytesPerSec = 800;
      wfx.nBlockAlign = 1;
      wfx.nChannels = 1;
      wfx.nSamplesPerSec = 8000;
      wfx.wBitsPerSample = 8;
      break;
	case WAVE_FORMAT_PCM:
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.cbSize = 0;
		wfx.nAvgBytesPerSec = 16000;
		wfx.nBlockAlign = 2;
		wfx.nChannels = 1;
		wfx.nSamplesPerSec = 8000;
		wfx.wBitsPerSample = 16;
		break;
    default:
		break;
    }

	ph_media_init_deviceID(deviceID);

   mr = waveOutOpen(&hWaveOut, waveoutDeviceID, &wfx, (DWORD)0/* SpeakerCallback */, 0/* arg */, CALLBACK_NULL /* CALLBACK_FUNCTION */);

  stream->hWaveOut = hWaveOut;

  if (mr != NOERROR)
    {
	fprintf(stderr, "__call_free: waveOutOpen: 0x%i\n", mr);
      exit(-1);
      return -1;
    }
  else
    {
      int i;
      for (i=0; i<USED_OUT_BUFFERS; i++)
	  {
			WAVEHDR *whp = &stream->waveHdrOut[i];

			whp->lpData = stream->dataBufferOut[i];
		    whp->dwBufferLength = 512;  /* frameSize */
			whp->dwFlags = 0;
			whp->dwUser = i;
	  
			mr = waveOutPrepareHeader(hWaveOut, whp, sizeof(*whp));
			if (mr != MMSYSERR_NOERROR)
			{
				fprintf(stderr, "__call_free: waveOutPrepareHeader: 0x%i\n", mr);
				exit(-1);
			}
		}
	}

	stream->event = CreateEvent(NULL,FALSE,FALSE,NULL);
	mr = waveInOpen(&hWaveIn, waveinDeviceID, &wfx, (DWORD) stream->event/*WaveInCallback*/, (DWORD) stream, CALLBACK_EVENT/*CALLBACK_FUNCTION*/);


	stream->hWaveIn = hWaveIn;

	if (mr != MMSYSERR_NOERROR)
	  {
		fprintf(stderr, "__call_free: waveInOpen: 0x%i\n", mr);
		exit(-1);
		return NULL;
	  }
	else 
	  {
	    int i;
	    for (i=0; i<USED_IN_BUFFERS; i++) 
	    {
		      WAVEHDR *whp = &stream->waveHdrIn[i];
		
			  whp->lpData = stream->dataBufferIn[i];
	
			  whp->dwBufferLength = codec->decoded_framesize;  /* frameSize */
			  whp->dwFlags = 0;
			  whp->dwUser = i;
			  mr = waveInPrepareHeader(hWaveIn, whp, sizeof(*whp));
			  if (mr != MMSYSERR_NOERROR)
				{
		      		fprintf(stderr, "__call_free: waveInPrepareHeader: 0x%i\n", mr); 
					exit(-1);
					return -1;
				}
			mr = waveInAddBuffer(hWaveIn, whp, sizeof (WAVEHDR));
			if (mr != MMSYSERR_NOERROR)
			{
				fprintf(stderr, "__call_free: waveInAddBuffer: 0x%i\n", mr); 
				/* return -1; */
			}
		}

	}
	return stream;
}



static void 
ph_telephone_event(RtpSession *rtp_session, phcall_t *ca)
{
}

#if DO_ECHO_CAN

void do_echo_update(phmstream_t *s, char *data, int len);
void store_pcm(phmstream_t *s, char *buf, int len);
void echo_resync(phmstream_t *s);

/* should be called in case of transmit underrun */
void echo_resync(phmstream_t *s){
  g_mutex_lock(s->synclock);
  s->pcm_rd = s->pcm_wr = 0;
  s->sent_cnt = s->recv_cnt = 0;
  s->pcm_need_resync = 1;
  s->bytes_to_throw = 0;
  g_mutex_unlock(s->synclock);
}

void store_pcm(phmstream_t *s, char *buf, int len)
{
  int lg;

  g_mutex_lock(s->synclock);
  if (len <= (PCM_TRACE_LEN - s->pcm_wr))
  {
    memcpy(s->pcm_sent + s->pcm_wr, buf, len);
    s->pcm_wr += len;
    if (s->pcm_wr == PCM_TRACE_LEN)
      s->pcm_wr = 0;
  }
  else
  {
    lg = PCM_TRACE_LEN - s->pcm_wr;
    memcpy(s->pcm_sent + s->pcm_wr, buf, lg);
    memcpy(s->pcm_sent, buf+lg, len-lg);
    s->pcm_wr = len-lg;
  }
  s->sent_cnt += len;
	if(s->pcm_need_resync && s->sent_cnt > 0)
	{
		int skip=0;
		struct timeval now;

		// compute the time elapsed since the moment application got something
		// from audio diver
		gettimeofday(&now, 0);
		AEC_MUTEX_LOCK(s);

		tvsub(&now, &s->last_audio_read);

	   
	    skip = 8*2 * now.tv_usec/1000;   // number of bytes waiting in the system buffers

		s->bytes_to_throw = skip;

		s->pcm_need_resync = 0;
		AEC_MUTEX_UNLOCK(s);
		printf("EC:Resync OK %d bytes to throw\n", skip);

  }
  g_mutex_unlock(s->synclock);
}

void do_echo_update2(phmstream_t *s, char *data, int length)
{
 int i, lg, len;
  short *p1, *p2;

  len = length;

  if (s->pcm_need_resync)
	  return ;

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
  if(s->recv_cnt > s->sent_cnt)
  {
    printf( "\nUNDERRUN: %d %d\n", s->sent_cnt, s->recv_cnt);
	echo_resync(s);
	return;
  }

    p1 = (short *)(s->pcm_sent + s->pcm_rd);
    p2 = (short *)data;

    if((PCM_TRACE_LEN - s->pcm_rd) >= len)
	{
      for (i=0; i<len/2; i++) 
	  {
#if !AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else /* AEC_BIS */
	*p2 = do_AEC(*p1++, *p2);
#endif /* AEC_BIS */
		p2++;
      }

      s->pcm_rd += len;
      if(s->pcm_rd == PCM_TRACE_LEN)
		s->pcm_rd = 0; 
    }
	else
	{
      lg = PCM_TRACE_LEN - s->pcm_rd;
      for (i=0; i<lg/2; i++)
	  {
#if !AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else /* AEC_BIS */
	*p2 = do_AEC(*p1++, *p2);
#endif /* AEC_BIS */
		p2++;
      }
      p1 = (short *)(s->pcm_sent);
      for (i=0; i<(len - lg)/2; i++)
	  {
#if !AEC_BIS
	*p2 = echo_can_update(s->ec, *p1++, *p2);
#else /* AEC_BIS */
	*p2 = do_AEC(*p1++, *p2);
#endif /* AEC_BIS */
		p2++;
      }
      s->pcm_rd = len-lg;
    }

}

void do_echo_update(phmstream_t *s, char *data, int length)
{
	while(length)
	{
		int len = (length > 8) ? 8 : length;

		do_echo_update2(s, data, len);
		data += len;
		length -= len;
	}
}
#endif /* !DO_ECHO_CAN */


void * 
ph_audio_in_thread(void *_p)
{
  phmstream_t	*stream = (phmstream_t *) _p;
  int					pos_whdr=0;
  int					have_more;
  int					timestamp = 0;
  int					ts_inc;
  int					len;
  phcodec_t		*codec = stream->codec;
  struct timeval	tvBRecv;
  struct timeval	tvARecv;
  MMRESULT	mr = NOERROR;
  int gotSome = 0;

  boostPriority(1);

  while (stream->running)
    {
	  int length;
	  mblk_t *mp;
	  WAVEHDR  *whp = &stream->waveHdrOut[pos_whdr];

	  ts_inc = 0;
	  /* FIXME How bizarre !!! */
	  if (stream->payload==3)
		  length = 260; /* 280 ?? */
	  else
		  length = 160;
#if USE_CODECS
	  length = codec->encoded_framesize;
#endif /* !USE_CODECS */
	gettimeofday(&tvBRecv, NULL);
   if ( (mp=rtp_session_recvm_with_ts(stream->rtp_session,timestamp))!=NULL) 
	{
		gettimeofday(&tvARecv, NULL);
		tvsub(&tvARecv, &tvBRecv);
		if (tvARecv.tv_usec > 160000)
			fprintf(stderr, "Rtp session received too long (%i ms)\n", tvARecv.tv_usec / 1000);
		len=mp->b_cont->b_wptr-mp->b_cont->b_rptr;
	 
	  if (!stream->running)
		  break;
	   
	      
#if USE_CODECS

		len = codec->decode(stream->decoder_ctx, mp->b_cont->b_rptr, len, whp->lpData, 512);
	    freemsg(mp);

		if (len <= 0)
	      continue;

		gotSome = 1;
	    ts_inc += len/2;


#else /* !USE_CODEC */
		if (stream->payload==8) /* A-Law */
			alaw_dec(mp->b_cont->b_rptr, whp->lpData, len);
	    
	      if (stream->payload==0) /* Mu-Law */
			  mulaw_dec(mp->b_cont->b_rptr, whp->lpData, len);

	      ts_inc += len;

		  freemsg(mp);

#endif /* !USE_CODECS */

		 whp->dwBufferLength = len;
#if DO_ECHO_CAN_WITHRESET
		 if (stream->pcm_need_resync)
		 {
			 waveInReset(stream->hWaveIn);
			 waveInStart(stream->hWaveIn);
		 }
#endif

	      mr = waveOutWrite(stream->hWaveOut, whp, sizeof(*whp));

#if DO_ECHO_CAN
		  store_pcm(stream, whp->lpData, len);
#endif /*! DO_ECHO_CAN */

	      pos_whdr++;
	      if (pos_whdr==USED_OUT_BUFFERS) 
		      pos_whdr=0; /* loop over the prepared blocks */
	      if (mr != MMSYSERR_NOERROR)
		switch (mr)
		  {
		  case MMSYSERR_INVALHANDLE :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i MMSYSERR_INVALHANDLE\n", mr);
		    break;
		  case MMSYSERR_NODRIVER :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i MMSYSERR_NODRIVER\n", mr);
		    break;
		  case MMSYSERR_NOMEM :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i MMSYSERR_NOMEM\n", mr);
		    break;
		  case WAVERR_UNPREPARED :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i WAVERR_UNPREPARED\n", mr);
		    break;
		  case WAVERR_STILLPLAYING :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i WAVERR_STILLPLAYING\n", mr);
		  default :
		    fprintf(stderr, "__call_free: waveOutWrite: 0x%i\n", mr);
		  }
	    }
   else
	{
		rtp_session_resync(stream->rtp_session, timestamp);
		Sleep(10);
	}
	  timestamp += ts_inc;
	}

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
  phmstream_t	*stream = (phmstream_t *) _p;
  WAVEHDR		*wHdr;
  MMRESULT	mr = NOERROR;
  int					pos_whdr=0;
  int					framesize = 320;
  int					enclen;
  char				data_out_enc[1000];
  int					i;

#if USE_CODECS
	phcodec_t		*codec = stream->codec;

	boostPriority(1);

	framesize = codec->decoded_framesize;

	gettimeofday(&stream->last_audio_read, 0);
    mr = waveInStart(stream->hWaveIn);
	if (mr != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "__call_free: waveInStart: 0x%i\n", mr); 
		exit(-1);
    }


#endif /* !USE_CODECS */
  AEC_MUTEX_LOCK(stream);
  while (stream->running)
  {
	   int waitResult;
	   if (pos_whdr < 0 || pos_whdr >= USED_IN_BUFFERS)
			fprintf(stderr, "pos_whdr=%d\n", pos_whdr);


	    wHdr = &stream->waveHdrIn[pos_whdr];
		if (!(wHdr->dwFlags & WHDR_DONE))
		{

			gettimeofday(&stream->last_audio_read, 0);
			AEC_MUTEX_UNLOCK(stream);
			waitResult = WaitForSingleObject(stream->event, 100);
			AEC_MUTEX_LOCK(stream);
			if (waitResult == WAIT_TIMEOUT)
				continue;


		}

		if (!stream->running)
			break;

		if (!(wHdr->dwFlags & WHDR_DONE))
			continue;


#if DO_ECHO_CAN
		do_echo_update(stream, wHdr->lpData, wHdr->dwBytesRecorded);
#endif /* !DO_ECHO_CAN */


		if ((stream->dtmfg_phase != DTMF_IDLE) || (stream->dtmfq_cnt != 0))
			ph_generate_out_dtmf(stream, (short *) wHdr->lpData, wHdr->dwBytesRecorded / 2);

#if USE_CODECS

		enclen = codec->encode(stream->encoder_ctx, wHdr->lpData, wHdr->dwBytesRecorded, data_out_enc, sizeof (data_out_enc));
		if (enclen < 0 || enclen > 1000)
		fprintf(stderr, "enclen=%d\n", enclen);

		rtp_session_send_with_ts(stream->rtp_session, data_out_enc, enclen, stream->timestamp);
	
		stream->timestamp+=wHdr->dwBytesRecorded / 2;
#else
		rtp_session_send_with_ts(stream->rtp_session, wHdr->lpData, wHdr->dwBytesRecorded, stream->timestamp);
		stream->timestamp+=wHdr->dwBytesRecorded;
#endif /* !USE_CODECS */

		wHdr->dwBytesRecorded = 0;
	    mr = waveInAddBuffer(stream->hWaveIn, wHdr, sizeof(*wHdr));
		if (mr != MMSYSERR_NOERROR)
			fprintf(stderr, "__call_free: waveInAddBuffer: 0x%i\n", mr); 

		pos_whdr++;
		if (pos_whdr == USED_IN_BUFFERS) 		
			pos_whdr = 0; /* loop over the prepared blocks */

		switch (mr)
		{
			case MMSYSERR_NOERROR :
				break;
			case MMSYSERR_INVALHANDLE : 
				fprintf(stderr, "waveInAddBuffer : Specified device handle is invalid.\n");
				break;
			case MMSYSERR_NODRIVER : 
				fprintf(stderr, "waveInAdBuffer : No device driver is present.\n");
				break;
			case MMSYSERR_NOMEM : 
				fprintf(stderr, "waveInAddBuffer : Unable to allocate or lock memory.\n");
				break;
			case WAVERR_UNPREPARED : 
				fprintf(stderr, "waveInAddBuffer : The buffer pointed to by the pwh parameter hasn't been prepared.\n");
				break;
			case WAVERR_STILLPLAYING :
				fprintf(stderr, "waveInAddBuffer : still something playing.\n");
			default :
				fprintf(stderr, "waveInAddBuffer error = 0x%x\n", mr);
		}
	}
  AEC_MUTEX_UNLOCK(stream);
  return NULL;
}

int ph_media_start(phcall_t *ca, int port, void (*dtmfCallback)(phcall_t *ca, int event), const char * deviceId)
{
  int					format = WAVE_FORMAT_PCM;
  phmstream_t	*stream;
#if USE_CODECS
  phcodec_t		*codec;
#endif /* !USE_CODECS */
#if DO_ECHO_CAN
  int					taps=256;
#endif /* !DO_ECHO_CAN */

#if USE_CODECS
  codec = ph_media_lookup_codec(ca->payload);
  if (!codec)
	  return 0;
#endif /* !USE_CODECS */

  stream = open_sndcard(format, codec, deviceId);
  if (!stream)
    return -1;

  stream->payload = ca->payload;

  stream->rtp_session = rtp_session_new(RTP_SESSION_SENDRECV);
  rtp_session_set_scheduling_mode(stream->rtp_session, 0); /* yes */
  rtp_session_set_blocking_mode(stream->rtp_session, 0);
  
  rtp_session_set_profile(stream->rtp_session, &av_profile);
  rtp_session_set_jitter_compensation(stream->rtp_session, 60);
  rtp_session_set_local_addr(stream->rtp_session, "0.0.0.0", port);
  rtp_session_set_remote_addr(stream->rtp_session,
			      ca->remote_sdp_audio_ip,
			      ca->remote_sdp_audio_port);
  rtp_session_set_payload_type(stream->rtp_session, stream->payload);
  rtp_session_signal_connect(stream->rtp_session, "telephone-event",
			     (RtpCallback)ph_telephone_event, ca);
  

  ca->hasaudio = 1;
  stream->running = 1;
  ca->phstream = stream;
  stream->dtmfCallback = dtmfCallback;
# if DO_ECHO_CAN
#  if AEC_BIS
  create_AEC();
#  else /* !AEC_BIS */
  stream->ec = echo_can_create(taps, 0);
  if(stream->ec == 0){
    fprintf(stderr, "Echo CAN creating failed\n");
  }
  if(stream->ec)
  {
#  endif /* !AEC_BIS */
    stream->echocancel = taps;
    stream->pcm_rd = stream->pcm_wr = 0;
    stream->pcm_sent = malloc(PCM_TRACE_LEN);
	stream->sent_cnt = stream->recv_cnt = 0;
    if(stream->pcm_sent == 0)
      fprintf(stderr, "No memory for EC  %d\n", stream->pcm_sent);
#if !AEC_BIS
  }
#endif /* AEC_BIS */
  stream->aec_mutex = g_mutex_new();
  stream->synclock = g_mutex_new();
  stream->sync_cond = g_cond_new();
  stream->pcm_need_resync = 1;
  stream->bytes_to_throw = 0;
# endif /* !DO_ECHO_CAN */
  stream->dtmfg_lock = g_mutex_new();
  stream->dtmfq_cnt = 0;
  stream->dtmfg_phase = DTMF_IDLE;

 stream->audio_in_thread = osip_thread_create(20000,
					ph_audio_in_thread, stream);
 stream->audio_out_thread = osip_thread_create(20000,
				ph_audio_out_thread, stream);


  return 0;
}



void ph_media_stop(phcall_t *ca)
{
  phmstream_t *stream = (phmstream_t *) ca->phstream;


  ca->hasaudio = 0;
  ca->phstream = 0;

  stream->running = 0;

  osip_thread_join(stream->audio_in_thread);
  osip_thread_join(stream->audio_out_thread);
  osip_free(stream->audio_in_thread);
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
#endif /* !USE_CODECS */

  waveInReset(stream->hWaveIn);
  waveInClose(stream->hWaveIn);
  waveOutReset(stream->hWaveOut);
  waveOutClose(stream->hWaveOut);

#if DO_ECHO_CAN
   printf("\nbytes echoed %d %d\n", stream->sent_cnt, stream->recv_cnt);

  free(stream->pcm_sent);

# if !AEC_BIS
  if(stream->ec)
    echo_can_free(stream->ec);
# else /* AEC_BIS */
  kill_AEC();
# endif /* AEC_BIS */

  g_mutex_free(stream->synclock);
  g_cond_free(stream->sync_cond);
  g_mutex_free(stream->aec_mutex);


#endif /* !DO_ECHO_CAN */

  g_mutex_free(stream->dtmfg_lock);

  osip_free(stream);
}

int							ph_media_send_dtmf(phcall_t *ca, int dtmf)
{
	phmstream_t		*stream = (phmstream_t *) ca->phstream;
	unsigned long	ts;

	if (!stream)
		return 1;

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
}

void ph_media_suspend(phcall_t *ca)
{
}



void ph_media_resume(phcall_t *ca)
{
}


int ph_media_cleanup()
{
	return 0;
}


#endif
