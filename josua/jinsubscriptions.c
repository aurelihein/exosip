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

#include "jinsubscriptions.h"

jinsubscription_t jinsubscriptions[MAX_NUMBER_OF_INSUBSCRIPTIONS];

static int ___jinsubscription_init = 0;

static void __jinsubscription_init()
{
  int k;
  if (___jinsubscription_init==0)
    {
      ___jinsubscription_init = -1;
      for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
	{
	  memset(&(jinsubscriptions[k]), 0, sizeof(jinsubscription_t));
	  jinsubscriptions[k].state = NOT_USED;
	}
    }
}


int
jinsubscription_get_number_of_pending_insubscriptions()
{
  int pos=0;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED)
	{
	  pos++;
	}
    }
  return pos;
}

jinsubscription_t *jinsubscription_find_insubscription(int pos)
{
  int k;
  __jinsubscription_init();

  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED)
	{
	  if (pos==0)
	    return &(jinsubscriptions[k]);
	  pos--;
	}
    }
  return NULL;
}

int jinsubscription_new(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init();

  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid)
	break;
    }
  if (k!=MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      /* new subscrib for existing subscription */
      ca = &(jinsubscriptions[k]);
      
      ca->online_status = je->online_status;
      ca->ss_status = je->ss_status;
      ca->ss_reason = je->ss_reason;
      
      if (ca->ss_status==EXOSIP_SUBCRSTATE_TERMINATED)
	ca->state = NOT_USED;
      return 0;
    }

  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state == NOT_USED)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);
  memset(&(jinsubscriptions[k]), 0, sizeof(jinsubscription_t));

  ca->nid = je->nid;
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

int jinsubscription_remove(jinsubscription_t *ca)
{
  __jinsubscription_init();
  if (ca==NULL)
    return -1;
  ca->state = NOT_USED;
  return 0;
}

int jinsubscription_proceeding(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
	{
	  if (jinsubscriptions[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
	return -1;

      ca = &(jinsubscriptions[k]);
      memset(&(jinsubscriptions[k]), 0, sizeof(jinsubscription_t));
      
      ca->nid = je->nid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jinsubscriptions[k]);

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

int jinsubscription_answered(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
	{
	  if (jinsubscriptions[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
	return -1;
      ca = &(jinsubscriptions[k]);
      memset(&(jinsubscriptions[k]), 0, sizeof(jinsubscription_t));
      
      ca->nid = je->nid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jinsubscriptions[k]);

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

int jinsubscription_redirected(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jinsubscription_requestfailure(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jinsubscription_serverfailure(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jinsubscription_globalfailure(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid
	  && jinsubscriptions[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int jinsubscription_closed(eXosip_event_t *je)
{
  jinsubscription_t *ca;
  int k;
  __jinsubscription_init();
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
	  && jinsubscriptions[k].nid==je->nid)
	break;
    }
  if (k==MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = je->online_status;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

