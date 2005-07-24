/*
  The antisipc program is a modular SIP softphone (SIP -rfc3261-)
  Copyright (C) 2005  Aymeric MOIZARD - <jack@atosc.org>
*/

#ifndef __CALLS_H__
#define __CALLS_H__

#define _SENDRECV 0
#define _SENDONLY 1
#define _RECVONLY 2

#ifdef MEDIASTREAMER_SUPPORT
#undef PACKAGE
#undef VERSION
#include <mediastream.h>
#ifndef VERSION
#define VERSION "sh-phone-xxx"
#endif
#endif

#if defined(UCL_SUPPORT)
#ifndef WIN32
#include <uclmmbase/uclconf.h>
#endif
#include <uclmmbase/config_unix.h>
#include <uclmmbase/config_win32.h>
#include <uclmmbase/rtp.h>
#ifdef WIN32
#include <uclmmbase/memory.h>
#endif
#endif

#if defined(ORTP_SUPPORT)
#undef PACKAGE
#undef VERSION
#include <ortp/ortp.h>
#ifndef VERSION
#define VERSION "sh-phone-xxx"
#endif
#endif

#include <eXosip2/eXosip.h>


#define MULAW_BYTES	160
#define MULAW_PAYLOAD	0
#define ALAW_PAYLOAD	8
#define MULAW_MS	20

#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)

void mulaw_dec (char *mulaw_data /* contains size char */ ,
                char *s16_data /* contains size*2 char */ ,
                int size);
void mulaw_enc (char *s16_data /* contains 320 char */ ,
                char *mulaw_data /* contains 160 char */ ,
                int pcm_size);
void alaw_dec (char *alaw_data /* contains size char */ ,
               char *s16_data /* contains size*2 char */ ,
               int size);
void alaw_enc (char *s16_data /* contains 320 char */ ,
               char *alaw_data /* contains 160 char */ ,
               int pcm_size);

#endif

struct call
{
  int cid;
  int did;
  int tid;

  char reason_phrase[50];
  int status_code;

  char textinfo[256];
  char req_uri[256];
  char local_uri[256];
  char remote_uri[256];
  char subject[256];

  char remote_sdp_audio_ip[50];
  int remote_sdp_audio_port;
  int payload;
  char payload_name[50];

  int fd;                       /* sound card descriptor */
  int enable_audio;             /* 0 started, -1 stopped */
  int local_sendrecv;           /* _SENDRECV, _SENDONLY, _RECVONLY */
  int remote_sendrecv;          /* _SENDRECV, _SENDONLY, _RECVONLY */
  char wav_file[256];
#ifdef MEDIASTREAMER_SUPPORT
  AudioStream *audio;
#elif defined(WIN32) && defined(ORTP_SUPPORT)
  RtpSession *rtp_session;
  struct osip_thread *audio_thread;
#elif defined(ORTP_SUPPORT)
  RtpSession *rtp_session;
  struct osip_thread *audio_thread;
  struct osip_thread *out_audio_thread;
#elif defined(UCL_SUPPORT) && defined(WIN32)
  struct rtp *rtp_session;
  struct osip_thread *audio_thread;

  WAVEFORMATEX wfx;

  WAVEHDR waveHdrOut[30];
  HWAVEOUT hWaveOut;
  char dataBufferOut[30][3200];

  WAVEHDR waveHdrIn[30];
  HWAVEIN hWaveIn;
  char dataBufferIn[30][3200];

  char mulaw_buffer_in[16 * MULAW_BYTES];
  int mulaw_buffer_pos_in;
  struct osip_mutex *mulaw_mutex_in;

  char mulaw_buffer_out[16 * MULAW_BYTES];
  int mulaw_buffer_pos_out;
  struct osip_mutex *mulaw_mutex_out;

#elif defined(UCL_SUPPORT)
  struct rtp *rtp_session;
  struct osip_thread *audio_thread;
#endif

#define NOT_USED      0
  int state;

};

typedef struct call call_t;

int os_sound_init (void);
int os_sound_start (call_t * ca, int port);
void *os_sound_start_thread (void *_ca);
void *os_sound_start_out_thread (void *_ca);
void os_sound_close (call_t * ca);

#if defined(ORTP_SUPPORT)
void rcv_telephone_event (RtpSession * rtp_session, call_t * ca);
#endif

extern call_t calls[];

#ifndef MAX_NUMBER_OF_CALLS
#define MAX_NUMBER_OF_CALLS 10
#endif

call_t *call_find_call (int pos);
int call_get_number_of_pending_calls (void);
call_t *call_locate_call (eXosip_event_t * je, int createit);
call_t *call_locate_call_by_cid (int cid);
call_t *call_create_call (int cid);
call_t *call_get_call (int num);
int call_get_callpos (call_t * ca);

int call_new (eXosip_event_t * je);
int call_ack (eXosip_event_t * je);
int call_answered (eXosip_event_t * je);
int call_proceeding (eXosip_event_t * je);
int call_ringing (eXosip_event_t * je);
int call_redirected (eXosip_event_t * je);
int call_requestfailure (eXosip_event_t * je);
int call_serverfailure (eXosip_event_t * je);
int call_globalfailure (eXosip_event_t * je);

int call_closed (eXosip_event_t * je);
int call_modified (eXosip_event_t * je);


int call_remove (call_t * ca);

#endif
