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

#ifndef __JCALLS_H__
#define __JCALLS_H__

#ifdef MEDIASTREAMER_SUPPORT
#undef PACKAGE
#undef VERSION
#include <mediastream.h>
#ifndef VERSION
#define VERSION "sh-phone-xxx"
#endif
#endif

#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)

void mulaw_dec(char *mulaw_data /* contains size char */,
	       char *s16_data    /* contains size*2 char */,
	       int size);
void mulaw_enc(char *s16_data    /* contains 320 char */,
	       char *mulaw_data  /* contains 160 char */,
	       int pcm_size);
void alaw_dec(char *alaw_data   /* contains size char */,
	      char *s16_data    /* contains size*2 char */,
	      int size);
void alaw_enc(char *s16_data   /* contains 320 char */,
	      char *alaw_data  /* contains 160 char */,
	      int pcm_size);
	      
#endif

#if defined(UCL_SUPPORT)
#include <uclmmbase/uclconf.h>
#include <uclmmbase/config_unix.h>
#include <uclmmbase/config_win32.h>
#include <uclmmbase/rtp.h>
#endif

#if defined(ORTP_SUPPORT)
#undef PACKAGE
#undef VERSION
#include <ortp.h>
#ifndef VERSION
#define VERSION "sh-phone-xxx"
#endif
#endif

#ifdef WIN32
#undef PACKAGE
#undef VERSION
#include <ortp.h>
#ifndef VERSION
#define VERSION "sh-phone-xxx"
#endif
#endif

#include <eXosip/eXosip.h>


struct jcall {
  int cid;
  int did;

  char reason_phrase[50];
  int  status_code;

  char textinfo[256];
  char req_uri[256];
  char local_uri[256];
  char remote_uri[256];
  char subject[256];

  char remote_sdp_audio_ip[50];
  int  remote_sdp_audio_port;
  int  payload;
  char payload_name[50];
  
#ifdef MEDIASTREAMER_SUPPORT
  int enable_audio; /* 0 started, -1 stopped */
  AudioStream *audio;
#elif defined(WIN32)
  RtpSession *rtp_session;
  int enable_audio; /* 0 started, -1 stopped */
  struct osip_thread *audio_thread;
#elif defined(ORTP_SUPPORT)
  RtpSession *rtp_session;
  int enable_audio; /* 0 started, -1 stopped */
  struct osip_thread *audio_thread;
  struct osip_thread *out_audio_thread;
#elif defined(UCL_SUPPORT)
  struct rtp *rtp_session;
  int enable_audio; /* 0 started, -1 stopped */
  struct osip_thread *audio_thread;
#endif

#define NOT_USED      0
  int state;

};

typedef struct jcall jcall_t;

int    os_sound_init();
int    os_sound_start(jcall_t *ca, int port);
void  *os_sound_start_thread(void *_ca);
void  *os_sound_start_out_thread(void *_ca);
void   os_sound_close(jcall_t *ca);

#if defined(ORTP_SUPPORT) || defined(WIN32)
void rcv_telephone_event(RtpSession *rtp_session, jcall_t *ca);
#endif

extern jcall_t jcalls[];

#ifndef MAX_NUMBER_OF_CALLS
#define MAX_NUMBER_OF_CALLS 10
#endif

jcall_t *jcall_find_call(int pos);
int jcall_get_number_of_pending_calls();
jcall_t *jcall_locate_call(eXosip_event_t *je, int createit);
jcall_t *jcall_locate_call_by_cid(int cid);
jcall_t *jcall_create_call(int cid);
jcall_t *jcall_get_call(int num);
int jcall_get_callpos(jcall_t *ca);

int jcall_new(eXosip_event_t *je);
int jcall_ack(eXosip_event_t *je);
int jcall_answered(eXosip_event_t *je);
int jcall_proceeding(eXosip_event_t *je);
int jcall_ringing(eXosip_event_t *je);
int jcall_redirected(eXosip_event_t *je);
int jcall_requestfailure(eXosip_event_t *je);
int jcall_serverfailure(eXosip_event_t *je);
int jcall_globalfailure(eXosip_event_t *je);

int jcall_closed(eXosip_event_t *je);
int jcall_onhold(eXosip_event_t *je);
int jcall_offhold(eXosip_event_t *je);


int jcall_remove(jcall_t *ca);

#endif
