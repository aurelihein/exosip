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
#include "../src/eXosip2.h"

jcall_t jcalls[MAX_NUMBER_OF_CALLS];
char   _localip[30];
static int ___call_init = 0;

static int __call_init()
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      jcalls[k].state = NOT_USED;
    }

  eXosip_get_localip(_localip);
  
  return os_sound_init();
}

void __call_free()
{

}

#if defined(ORTP_SUPPORT) || defined(WIN32)
void
rcv_telephone_event(RtpSession *rtp_session, jcall_t *ca)
{
  /* telephone-event received! */
  
}
#endif

int
jcall_get_number_of_pending_calls()
{
  int pos=0;
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED)
	{
	  pos++;
	}
    }
  return pos;
}


jcall_t *jcall_get_call(int pos)
{
  return &jcalls[pos];
}


int jcall_get_callpos(jcall_t *ca)
{
  return ca - jcalls;
}

jcall_t *jcall_locate_call_by_cid(int cid)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==cid)
	return &jcalls[k];
    }

  return 0;
}


jcall_t *jcall_create_call(int cid)
{
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state == NOT_USED)
	{
	  memset(&(jcalls[k]), 0, sizeof(jcall_t));
	  jcalls[k].cid= cid;
	  jcalls[k].did= -1;
	  jcalls[k].state = -1;
	  return &jcalls[k];
	}
	  
    }

  return 0;
}




jcall_t *jcall_locate_call(eXosip_event_t *je, int createit)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      if (!createit)
	return 0;
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return NULL;

      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->cid<1)
	{
	  exit(0);
	  return NULL; /* not enough information for this event?? */
	}
    }

  
  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }

  ca->state = je->type;
  return ca;

}


jcall_t *jcall_find_call(int pos)
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED)
	{
	  if (pos==0)
	    return &(jcalls[k]);
	  pos--;
	}
    }
  return NULL;
}

int jcall_new(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state == NOT_USED)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  memset(&(jcalls[k]), 0, sizeof(jcall_t));

  ca->cid = je->cid;
  ca->did = je->did;

  if (ca->did<1 && ca->cid<1)
    {
      exit(0);
      return -1; /* not enough information for this event?? */
    }

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);

      os_sound_init();
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }

  eXosip_answer_call(ca->did, 180, NULL);

  ca->state = je->type;
  return 0;
}

int jcall_ack(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  if (je->remote_sdp_audio_ip[0]!='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }
  if (ca->remote_sdp_audio_ip[0]!='\0')
    {
      if (0==os_sound_start(ca, atoi(je->jc->c_sdp_port)))
	{
	  ca->enable_audio=1; /* audio is started */
	}
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jcall_remove(jcall_t *ca)
{
  if (ca==NULL)
    return -1;


  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_proceeding(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;

      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->cid<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;

}

int jcall_ringing(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;
      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->cid<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;;
  return 0;
}

int jcall_answered(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;
      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->cid<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (ca->remote_sdp_audio_ip[0]!='\0')
    {
      if (0==os_sound_start(ca, atoi(je->jc->c_sdp_port)))
	{
	  ca->enable_audio=1; /* audio is started */
	}
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jcall_redirected(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_requestfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }

  if (je->status_code==407 ||
      je->status_code==401)
    {
      static int oddnumber=0;
      if (oddnumber==0)
	{
	  eXosip_retry_call(je->cid);
	  oddnumber=1;
	} else oddnumber=0;
    }

  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_serverfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);


  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_globalfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_closed(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      os_sound_close(ca);
    }

  ca->state = NOT_USED;
  return 0;
}

int jcall_onhold(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jcall_offhold(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

