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

#include "jsubscriptions.h"

jsubscription_t jsubscriptions[MAX_NUMBER_OF_SUBSCRIPTIONS];

static int ___jsubscription_init = 0;

static void __jsubscription_init()
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      memset(&(jsubscriptions[k]), 0, sizeof(jsubscription_t));
      jsubscriptions[k].state = NOT_USED;
    }
}


int
jsubscription_get_number_of_pending_subscriptions()
{
  int pos=0;
  int k;
  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED)
	{
	  pos++;
	}
    }
  return pos;
}

jsubscription_t *jsubscription_find_subscription(int pos)
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED)
	{
	  if (pos==0)
	    return &(jsubscriptions[k]);
	  pos--;
	}
    }
  return NULL;
}

int jsubscription_new(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  if (___jsubscription_init==0)
    {
      ___jsubscription_init = -1;
      __jsubscription_init();
    }

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state == NOT_USED)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);
  memset(&(jsubscriptions[k]), 0, sizeof(jsubscription_t));

  ca->sid = je->sid;
  ca->did = je->did;

  if (ca->did<1 && ca->did<1)
    {
      exit(0);
      return -1; /* not enough information for this event?? */
    }

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }

  ca->state = je->type;
  return 0;
}

int jsubscription_remove(jsubscription_t *ca)
{
  if (ca==NULL)
    return -1;
  ca->state = NOT_USED;
  return 0;
}

int jsubscription_proceeding(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
	{
	  if (jsubscriptions[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
	return -1;

      ca = &(jsubscriptions[k]);
      memset(&(jsubscriptions[k]), 0, sizeof(jsubscription_t));
      
      ca->sid = je->sid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;

}

int jsubscription_answered(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
	{
	  if (jsubscriptions[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
	return -1;
      ca = &(jsubscriptions[k]);
      memset(&(jsubscriptions[k]), 0, sizeof(jsubscription_t));
      
      ca->sid = je->sid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);


  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jsubscription_redirected(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jsubscription_requestfailure(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jsubscription_serverfailure(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jsubscription_globalfailure(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}


int jsubscription_notify(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid
	  && jsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
	{
	  if (jsubscriptions[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
	return -1;
      ca = &(jsubscriptions[k]);
      memset(&(jsubscriptions[k]), 0, sizeof(jsubscription_t));
      
      ca->sid = je->sid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);


  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jsubscription_closed(eXosip_event_t *je)
{
  jsubscription_t *ca;
  int k;

  for (k=0;k<MAX_NUMBER_OF_SUBSCRIPTIONS;k++)
    {
      if (jsubscriptions[k].state != NOT_USED
	  && jsubscriptions[k].sid==je->sid)
	break;
    }
  if (k==MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

