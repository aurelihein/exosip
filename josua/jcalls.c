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

jcall_t jcalls[MAX_NUMBER_OF_CALLS];

static int ___josua_init = 0;

static void __josua_init()
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      jcalls[k].state = NOT_USED;
    }
}

int jcall_new(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___josua_init==0)
    {
      ___josua_init = -1;
      __josua_init();
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

  if (ca->did<1 && ca->did<1)
    {
      exit(0);
      return -1; /* not enough information for this event?? */
    }

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='0')
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
      
      if (ca->did<1 && ca->did<1)
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

  if (ca->remote_sdp_audio_ip[0]=='0')
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

int jcall_ringing(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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
      
      if (ca->did<1 && ca->did<1)
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

  if (ca->remote_sdp_audio_ip[0]=='0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
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
      
      if (ca->did<1 && ca->did<1)
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

  if (ca->remote_sdp_audio_ip[0]=='0')
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

int jcall_redirected(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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

  ca->state = NOT_USED;
  return 0;
}

int jcall_requestfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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

  ca->state = NOT_USED;
  return 0;
}

int jcall_serverfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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

  ca->state = NOT_USED;
  return 0;
}

int jcall_globalfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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

  ca->state = NOT_USED;
  return 0;
}

int jcall_closed(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

  ca->state = NOT_USED;
  return 0;
}

int jcall_onhold(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

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

  if (ca->remote_sdp_audio_ip[0]=='0')
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

  if (ca->remote_sdp_audio_ip[0]=='0')
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

