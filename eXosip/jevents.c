/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include <eXosip/eXosip.h>
#include <eXosip/eXosip2.h>

eXosip_t eXosip;


eXosip_event_t *
eXosip_event_init_for_call(int type,
			   eXosip_call_t *jc,
			   eXosip_dialog_t *jd)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jc = jc;
  je->jd = jd;

  /* fill in usefull info */
  if (type==EXOSIP_CALL_NEW
      || type==EXOSIP_CALL_PROCEEDING
      || type==EXOSIP_CALL_RINGING
      || type==EXOSIP_CALL_ANSWERED
      || type==EXOSIP_CALL_PROCEEDING
      || type==EXOSIP_CALL_DISCONNECTED)
    {
      if (jd->d_dialog!=NULL)
	{
	  osip_transaction_t *tr;
	  osip_header_t *subject;
	  char *tmp;
	  if (jd->d_dialog->remote_uri!=NULL)
	    {
	      osip_to_to_str(jd->d_dialog->remote_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->remote_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  if (jd->d_dialog->local_uri!=NULL)
	    {
	      osip_to_to_str(jd->d_dialog->local_uri, &tmp);
	      if (tmp!=NULL)
		{
		  snprintf(je->local_uri, 255, "%s", tmp);
		  osip_free(tmp);
		}
	    }
	  tr = eXosip_find_last_invite(jc, jd);
	  if (tr!=NULL && tr->orig_request!=NULL)
	    {
	      osip_message_get_subject(tr->orig_request, 0, &subject);
	      if (subject!=NULL && subject->hvalue!=NULL)
		snprintf(je->subject, 255, "%s", subject->hvalue);

	      osip_uri_to_str(tr->orig_request->req_uri, &tmp);
	      if (tmp!=NULL)
		snprintf(je->req_uri, 255, "%s", tmp);
	    }
	}
    }

  return je;
}

eXosip_event_t *
eXosip_event_init_for_subscribe(int type,
				eXosip_subscribe_t *js,
				eXosip_dialog_t *jd)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->js = js;
  je->jd = jd;
  return je;
}

eXosip_event_t *
eXosip_event_init_for_notify(int type,
			     eXosip_notify_t *jn,
			     eXosip_dialog_t *jd)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jn = jn;
  je->jd = jd;
  return je;
}

eXosip_event_t *
eXosip_event_init_for_reg(int type,
			  eXosip_reg_t *jr)
{
  eXosip_event_t *je;
  eXosip_event_init(&je, type);
  if (je==NULL) return NULL;
  je->jr = jr;
  return je;
}

int
eXosip_event_init(eXosip_event_t **je, int type)
{
  *je = (eXosip_event_t *) osip_malloc(sizeof(eXosip_event_t));
  if (*je==NULL) return -1;
  
  memset(*je, 0, sizeof(eXosip_event_t));
  (*je)->type = type;

  if (type==EXOSIP_CALL_NEW)
    {
      sprintf((*je)->textinfo, "New call received!");
    }
  else if (type==EXOSIP_CALL_PROCEEDING)
    {
      sprintf((*je)->textinfo, "Call is being processed!");
    }
  else if (type==EXOSIP_CALL_RINGING)
    {
      sprintf((*je)->textinfo, "Remote phone is ringing!");
    }
  else if (type==EXOSIP_CALL_ANSWERED)
    {
      sprintf((*je)->textinfo, "Remote phone has answered!");
    }
  else if (type==EXOSIP_CALL_DISCONNECTED)
    {
      sprintf((*je)->textinfo, "Call is over!");
    }
  else
    {
      (*je)->textinfo[0] = '\0';
    }
  return 0;
}

void
eXosip_event_free(eXosip_event_t *je)
{

  osip_free(je);
}

eXosip_call_t *
eXosip_event_get_callinfo(eXosip_event_t *je)
{
  return je->jc;
}

eXosip_dialog_t *
eXosip_event_get_dialoginfo(eXosip_event_t *je)
{
  return je->jd;
}

eXosip_reg_t *
eXosip_event_get_reginfo(eXosip_event_t *je)
{
  return je->jr;
}

eXosip_notify_t *
eXosip_event_get_notifyinfo(eXosip_event_t *je)
{
  return je->jn;
}

eXosip_subscribe_t *
eXosip_event_get_subscribeinfo(eXosip_event_t *je)
{
  return je->js;
}

int
eXosip_event_add(eXosip_event_t *je)
{
  return osip_fifo_add(eXosip.j_events, (void *) je);
}

eXosip_event_t *
eXosip_event_wait(int tv_s, int tv_ms)
{
  eXosip_event_t *je;
  je = (eXosip_event_t *) osip_fifo_tryget(eXosip.j_events);
  return je;
}

eXosip_event_t *
eXosip_event_get()
{
  eXosip_event_t *je;
  je = (eXosip_event_t *) osip_fifo_get(eXosip.j_events);
  return je;
}

