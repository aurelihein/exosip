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

#include <stdio.h>
#ifdef ORTP_SUPPORT

#undef PACKAGE
#undef VERSION
#include <ortp.h>

#elif defined(MEDIASTREAMER_SUPPORT)

#undef PACKAGE
#undef VERSION
#include <mediastream.h>
#undef PACKAGE
#undef VERSION
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
  
#ifdef ORTP_SUPPORT
  int enable_audio; /* 0 started, -1 stopped */
  RtpSession *rtp_session;
  struct osip_thread *audio_thread;
  struct osip_thread *out_audio_thread;
#elif defined(MEDIASTREAMER_SUPPORT)
  int enable_audio; /* 0 started, -1 stopped */
  AudioStream *audio;
#endif

#define NOT_USED      0
  int state;

};

typedef struct jcall jcall_t;

extern jcall_t jcalls[];

#ifndef MAX_NUMBER_OF_CALLS
#define MAX_NUMBER_OF_CALLS 10
#endif

jcall_t *jcall_find_call(int pos);
int jcall_get_number_of_pending_calls();

int jcall_new(eXosip_event_t *je);
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
