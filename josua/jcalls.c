/*
 * sh_phone - Shell Phone
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This software is protected by copyright. For more information
 * write to <jack@atosc.org>.
 */

#include "jcalls.h"

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

      if (0!=os_sound_start(ca))
	{
	}
      else
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

      if (0!=os_sound_start(ca))
	{
	}
      else
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

