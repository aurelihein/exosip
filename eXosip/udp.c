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

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#endif

extern eXosip_t eXosip;
extern char *localip;
extern char *localport;

void eXosip_send_default_answer(eXosip_dialog_t *jd,
				osip_transaction_t *transaction,
				osip_event_t *evt,
				int status)
{
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;
  
  /* osip_list_add(eXosip.j_transactions, transaction, 0); */
  osip_transaction_set_your_instance(transaction, NULL);
  
  /* THIS METHOD DOES NOT ACCEPT STATUS CODE BETWEEN 101 and 299 */
  if (status>100 && status<299 && MSG_IS_INVITE(evt->sip))
    return ;
  
  if (jd!=NULL)
    i = _eXosip_build_response_default(&answer, jd->d_dialog, status, evt->sip);
  else
    i = _eXosip_build_response_default(&answer, NULL, status, evt->sip);

  if (i!=0)
    {
      return ;
    }

  osip_message_set_content_length(answer, "0");
  
  if (status==500)
    osip_message_set_retry_after(answer, "10");
  
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction,evt_answer);
  
}

void eXosip_process_options(eXosip_call_t *jc, eXosip_dialog_t *jd,
			    osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;

  osip_transaction_set_your_instance(transaction,
				     __eXosip_new_jinfo(jc,
							NULL /*jd */,
							NULL,
							NULL));
  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      return ;
    }
    
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid =  transaction->transactionid;

  osip_list_add(jd->d_inc_trs, transaction , 0);

  osip_transaction_add_event(transaction,evt_answer);
}

void eXosip_process_bye(eXosip_call_t *jc, eXosip_dialog_t *jd,
			osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;

  osip_transaction_set_your_instance(transaction,
				     __eXosip_new_jinfo(jc,
							NULL /*jd */,
							NULL,
							NULL));

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      return ;
    }
  osip_message_set_content_length(answer, "0");
    
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid =  transaction->transactionid;

  osip_list_add(jd->d_inc_trs, transaction , 0);

  /* Release the eXosip_dialog */
  osip_dialog_free(jd->d_dialog);
  jd->d_dialog = NULL;

  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_call(EXOSIP_CALL_CLOSED, jc, jd);
    if (eXosip.j_call_callbacks[EXOSIP_CALL_CLOSED]!=NULL)
      eXosip.j_call_callbacks[EXOSIP_CALL_CLOSED](EXOSIP_CALL_CLOSED, je);
    else if (eXosip.j_runtime_mode==EVENT_MODE)
      eXosip_event_add(je);    
  }

  osip_transaction_add_event(transaction,evt_answer);
}

int cancel_match_invite(osip_transaction_t *invite, osip_message_t *cancel)
{
  osip_generic_param_t *br;
  osip_generic_param_t *br2;
  osip_via_t *via;
  osip_via_param_get_byname (invite->topvia, "branch", &br);
  via = osip_list_get(cancel->vias, 0);
  if (via==NULL) return -1; /* request without via??? */
  osip_via_param_get_byname (via, "branch", &br2);
  if (br!=NULL && br2==NULL)
    return -1;
  if (br2!=NULL && br==NULL)
    return -1;
  if (br2!=NULL && br!=NULL) /* compliant UA  :) */
    {
      if (br->gvalue!=NULL && br2->gvalue!=NULL &&
	  0==strcmp(br->gvalue, br2->gvalue))
	return 0;
      return -1;
    }
  /* old backward compatibility mechanism */
  if (0 != osip_call_id_match (invite->callid, cancel->call_id))
    return -1;
  if (0 != osip_to_tag_match (invite->to, cancel->to))
    return -1;
  if (0 != osip_from_tag_match (invite->from, cancel->from))
    return -1;
  if (0 != osip_cseq_match (invite->cseq, cancel->cseq))
    return -1;
  if (0 != osip_via_match (invite->topvia, via))
    return -1;
  return 0;
}

void eXosip_process_cancel(osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_transaction_t *tr;
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;
  
  eXosip_call_t *jc;
  eXosip_dialog_t *jd;

  tr = NULL;
  jd = NULL;
  /* first, look for a Dialog in the map of element */
  for (jc = eXosip.j_calls; jc!= NULL ; jc=jc->next)
    {
      if (jc->c_inc_tr!=NULL)
	{
	  i = cancel_match_invite(jc->c_inc_tr, evt->sip);
	  if (i==0) { 
	    tr = jc->c_inc_tr;
	    break;
	  }
	}
      tr=NULL;
      for (jd = jc->c_dialogs; jd!= NULL ; jd=jd->next)
	{
	  int pos=0;
	  while (!osip_list_eol(jd->d_inc_trs, pos))
	    {
	      tr = osip_list_get(jd->d_inc_trs, pos);
	      i = cancel_match_invite(tr, evt->sip);
	      if (i==0)
		break;
	      tr = NULL;
	      pos++;
	    }
	}
      if (jd!=NULL) break; /* tr has just been found! */
    }

  if (tr==NULL) /* we didn't found the transaction to cancel */
    {
      i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot cancel transaction.\n");
	  osip_list_add(eXosip.j_transactions, tr, 0);
	  osip_transaction_set_your_instance(tr, NULL);
	  return ;
	}
      osip_message_set_content_length(answer, "0");
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid =  transaction->transactionid;
      osip_transaction_add_event(transaction,evt_answer);
      
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      return;
    }

  if (tr->state==IST_TERMINATED || tr->state==IST_CONFIRMED
      || tr->state==IST_COMPLETED)
    {
      /* I can't find the status code in the rfc?
	 (I read I must answer 200? wich I found strange)
	 I probably misunderstood it... and prefer to send 481
	 as the transaction has been answered. */
      if (jd==NULL)
	i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
      else
	i = _eXosip_build_response_default(&answer, jd->d_dialog, 481, evt->sip);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot cancel transaction.\n");
	  osip_list_add(eXosip.j_transactions, tr, 0);
	  osip_transaction_set_your_instance(tr, NULL);
	  return ;
	}
      osip_message_set_content_length(answer, "0");
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid =  transaction->transactionid;
      osip_transaction_add_event(transaction,evt_answer);
      
      if (jd!=NULL)
	osip_list_add(jd->d_inc_trs, transaction , 0);
      else
	osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      return ;
    }

    {
      if (jd==NULL)
	i = _eXosip_build_response_default(&answer, NULL, 200, evt->sip);
      else
	i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot cancel transaction.\n");
	  osip_list_add(eXosip.j_transactions, tr, 0);
	  osip_transaction_set_your_instance(tr, NULL);
	  return ;
	}
      osip_message_set_content_length(answer, "0");
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid =  transaction->transactionid;
      osip_transaction_add_event(transaction,evt_answer);
      
      if (jd!=NULL)
	osip_list_add(jd->d_inc_trs, transaction , 0);
      else
	osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);

      /* answer transaction to cancel */
      if (jd==NULL)
	i = _eXosip_build_response_default(&answer, NULL, 487,
					  tr->orig_request);
      else
	i = _eXosip_build_response_default(&answer, jd->d_dialog, 487,
					  tr->orig_request);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot cancel transaction.\n");
	  osip_list_add(eXosip.j_transactions, tr, 0);
	  osip_transaction_set_your_instance(tr, NULL);
	  return ;
	}
      osip_message_set_content_length(answer, "0");
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid =  tr->transactionid;
      osip_transaction_add_event(tr,evt_answer);
    }
}

void eXosip_process_invite_on_hold(eXosip_call_t *jc, eXosip_dialog_t *jd,
				  osip_transaction_t *transaction,
				  osip_event_t *evt, sdp_message_t *remote_sdp)
{
  sdp_message_t *local_sdp;
  osip_message_t *answer;
  osip_event_t *sipevent;
  int i;

  /* We must negociate... */
  local_sdp = NULL;
  if (remote_sdp!=NULL) {
    sdp_message_t *old_remote_sdp = osip_negotiation_ctx_get_remote_sdp(jc->c_ctx);
    if (old_remote_sdp!=NULL)
      {
	sdp_message_free(old_remote_sdp);
      }
    osip_negotiation_ctx_set_remote_sdp(jc->c_ctx, remote_sdp);
    local_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
    if (local_sdp!=NULL)
      {
	sdp_message_free(local_sdp);
	osip_negotiation_ctx_set_local_sdp(jc->c_ctx, NULL);
      }
    local_sdp = NULL;
    i = osip_negotiation_ctx_execute_negotiation(eXosip.osip_negotiation, jc->c_ctx);

    if (i!=200)
      {
	osip_list_add(eXosip.j_transactions, transaction, 0);
	eXosip_send_default_answer(jd, transaction, evt, i);
	return;
      }
    local_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
  }

  if (remote_sdp==NULL)
    {
      sdp_message_t *local_sdp;
      osip_negotiation_sdp_build_offer(eXosip.osip_negotiation, NULL, &local_sdp, "25563", NULL);
      osip_negotiation_ctx_set_local_sdp(jc->c_ctx, local_sdp);

      if (local_sdp==NULL)
	{
	  osip_list_add(eXosip.j_transactions, transaction, 0);
	  eXosip_send_default_answer(jd, transaction, evt, 500);
	  return;
	}
    }

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }

  if (local_sdp!=NULL)
    {
      char *local_body;
      char *size;

      osip_negotiation_ctx_set_local_sdp(jc->c_ctx, NULL);
      i = sdp_message_to_str(local_sdp, &local_body);
      sdp_message_free(local_sdp);
      if (i!=0) {
	osip_list_add(eXosip.j_transactions, transaction, 0);
	eXosip_send_default_answer(jd, transaction, evt, 500);
	osip_message_free(answer);
	return ;
      } 
      i = osip_message_set_body(answer, local_body);
      if (i!=0) {
	osip_list_add(eXosip.j_transactions, transaction, 0);
	eXosip_send_default_answer(jd, transaction, evt, 500);
	osip_free(local_body);
	osip_message_free(answer);
	return;
      }
      size = (char *) osip_malloc(6*sizeof(char));
      sprintf(size,"%i",strlen(local_body));
      osip_free(local_body);
      osip_message_set_content_length(answer, size);
      osip_free(size);
      i = osip_message_set_header(answer, "content-type", "application/sdp");
      if (i!=0) {
	osip_list_add(eXosip.j_transactions, transaction, 0);
	eXosip_send_default_answer(jd, transaction, evt, 500);
	osip_message_free(answer);
	return;
      }	

    }
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  sipevent = osip_new_outgoing_sipmessage(answer);
  sipevent->transactionid =  transaction->transactionid;

  osip_list_add(jd->d_inc_trs, transaction, 0);

  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_call(EXOSIP_CALL_HOLD, jc, jd);
    eXosip_event_add_status(je, answer);
    if (eXosip.j_call_callbacks[EXOSIP_CALL_HOLD]!=NULL)
      eXosip.j_call_callbacks[EXOSIP_CALL_HOLD](EXOSIP_CALL_HOLD, je);
    else if (eXosip.j_runtime_mode==EVENT_MODE)
      eXosip_event_add(je);    
  }
  osip_transaction_add_event(transaction, sipevent);
}

void eXosip_process_invite_off_hold(eXosip_call_t *jc, eXosip_dialog_t *jd,
				  osip_transaction_t *transaction,
				  osip_event_t *evt, sdp_message_t *sdp)
{
  eXosip_process_invite_on_hold(jc, jd, transaction, evt, sdp);

}

void eXosip_process_new_options(osip_transaction_t *transaction, osip_event_t *evt)
{
  eXosip_call_t *jc;

  eXosip_call_init(&jc);
  ADD_ELEMENT(eXosip.j_calls, jc);

  jc->c_inc_options_tr = transaction;

  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, NULL, NULL, NULL));
  eXosip_update();

}

void eXosip_process_new_invite(osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_event_t *evt_answer;
  int i;
  eXosip_call_t *jc;
  eXosip_dialog_t *jd;
  osip_message_t *answer;
  char contact[200];

  eXosip_call_init(&jc);
  /* eXosip_call_set_subect... */

  ADD_ELEMENT(eXosip.j_calls, jc);

  i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot create dialog.");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }
  osip_message_set_content_length(answer, "0");
  sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	  localip,
	  localport);
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      osip_message_free(answer);
      return ;
    }

  i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot create dialog!\n");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      osip_message_free(answer);
      return ;
    }
  ADD_ELEMENT(jc->c_dialogs, jd);

  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));

  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;
  osip_transaction_add_event(transaction, evt_answer);

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 180, evt->sip);

  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot send ringback.");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      osip_transaction_set_your_instance(transaction, NULL);
      osip_message_free(answer);
      return ;
    }

  osip_message_set_content_length(answer, "0");
  /*  send message to transaction layer */

  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;

  eXosip_update();

  {
    eXosip_event_t *je;
    je = eXosip_event_init_for_call(EXOSIP_CALL_NEW, jc, jd);
    if (je!=NULL)
      {
	osip_header_t *subject;
	char *tmp;
	osip_message_get_subject(evt->sip, 0, &subject);
	if (subject!=NULL && subject->hvalue!=NULL)
	  snprintf(je->subject, 255, "%s", subject->hvalue);
	osip_uri_to_str(evt->sip->req_uri, &tmp);
	if (tmp!=NULL)
	  {
	    snprintf(je->req_uri, 255, "%s", tmp);
	    osip_free(tmp);
	  }
	eXosip_event_add_sdp_info(je, evt->sip);
	eXosip_event_add_status(je, answer);
      }
    if (eXosip.j_call_callbacks[EXOSIP_CALL_NEW]!=NULL)
      eXosip.j_call_callbacks[EXOSIP_CALL_NEW](EXOSIP_CALL_NEW, je);
    else if (eXosip.j_runtime_mode==EVENT_MODE)
      eXosip_event_add(je);    
  }

  jc->c_inc_tr = transaction;
  osip_transaction_add_event(transaction, evt_answer);

}

void eXosip_process_invite_within_call(eXosip_call_t *jc, eXosip_dialog_t *jd,
				       osip_transaction_t *transaction, osip_event_t *evt)
{
  sdp_message_t *sdp;
  int i;
  int pos;
  int pos_media;
  char *sndrcv;
  char *ipaddr;

  /* Is this a "on hold" message? */
  sdp = NULL;
  pos = 0;
  i = 500;
  while (!osip_list_eol(evt->sip->bodies,pos))
    {
      osip_body_t *body;
      body = (osip_body_t *)osip_list_get(evt->sip->bodies,pos);
      pos++;
      
      i = sdp_message_init(&sdp);
      if (i!=0) break;
      
      /* WE ASSUME IT IS A SDP BODY AND THAT    */
      /* IT IS THE ONLY ONE, OF COURSE, THIS IS */
      /* NOT TRUE */
      if (body->body!=NULL)
	{
	  i = sdp_message_parse(sdp,body->body);
	  if (i==0) {
	    i = 200;
	    break;
	  }
	}
      sdp_message_free(sdp);
      sdp = NULL;
    }

  if (pos!=0 && i!=200)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 400);
      return;
    }

  /* TODO: we should verify the content-type */
  if (pos!=0)
    {
      pos_media=-1;
      pos = 0;
      ipaddr = NULL;
      while (!sdp_message_endof_media(sdp, pos_media))
	{
	  ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);
	  while (ipaddr!=NULL) /* only one is allowed here? */
	    {
	      if (pos==1 && pos_media==-1)
		break;
	      if (0==strcmp("0.0.0.0", ipaddr))
		break;
	      pos++;
	      ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);
	    }
	  if (pos==1 && pos_media==-1)
	    ipaddr=NULL;
	  if (ipaddr!=NULL)
	    break;
	  pos = 0;
	  pos_media++;
	}
      
      if (ipaddr==NULL)
	{
	  sndrcv = NULL;
	  pos_media=-1;
	  pos = 0;
	  while (!sdp_message_endof_media(sdp, pos_media))
	    {
	      sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
	      while (sndrcv!=NULL)
		{
		  if (0==strcmp("inactive", sndrcv) || 0==strcmp("sendonly", sndrcv))
		    break;
		  pos++;
		  sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
		}
	      if (sndrcv!=NULL)
		break;
	      pos = 0;
	      pos_media++;
	    }
	}


      if (ipaddr!=NULL || (sndrcv!=NULL && (0==strcmp("inactive", sndrcv)
					    || 0==strcmp("sendonly", sndrcv))))
	{
	  /*  We received an INVITE to put on hold the other party. */
	  eXosip_process_invite_on_hold(jc, jd, transaction, evt, sdp);
	  return;
	}
      else
	{
	  /* This is a call modification, probably for taking it of hold */
	  eXosip_process_invite_off_hold(jc, jd, transaction, evt, sdp);
	  return;
	}
    }
  eXosip_process_invite_off_hold(jc, jd, transaction, evt, NULL);
  return;
}

int eXosip_event_package_is_supported(osip_transaction_t *transaction,
				      osip_event_t *evt)
{
  osip_header_t *event_hdr;
  int code;

  /* get the event type and return "489 Bad Event". */
  osip_message_header_get_byname(evt->sip, "event", 0, &event_hdr);
  if (event_hdr==NULL || event_hdr->hvalue==NULL)
    {
#ifdef SUPPORT_MSN
      /* msn don't show any event header */
      code = 200;      /* Bad Request... anyway... */
#else
      code = 400;      /* Bad Request */
#endif
    }
  else if (0!=osip_strcasecmp(event_hdr->hvalue, "presence"))
    code = 489;
  else code = 200;
  if (code!=200)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(NULL, transaction, evt, code);
      return 0;
    }
  return -1;
}

void eXosip_process_new_subscribe(osip_transaction_t *transaction,
				  osip_event_t *evt)
{
  osip_event_t *evt_answer;
  eXosip_notify_t *jn;
  eXosip_dialog_t *jd;
  osip_message_t *answer;
  char contact[200];
  int code;
  int i;

  eXosip_notify_init(&jn, evt->sip);
  _eXosip_notify_set_refresh_interval(jn, evt->sip);

  i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot create dialog.");
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_notify_free(jn);
      return;
    }
  sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	  localip,
	  localport);
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  if (i!=0)
    {
      osip_message_free(answer);
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_notify_free(jn);
      return ;
    }

  i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
  if (i!=0)
    {
      osip_message_free(answer);
      fprintf(stderr, "eXosip: cannot create dialog!\n");
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_notify_free(jn);
      return ;
    }
  ADD_ELEMENT(jn->n_dialogs, jd);

  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, NULL, jn));

  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;
  osip_transaction_add_event(transaction, evt_answer);

  ADD_ELEMENT(eXosip.j_notifies, jn);

  /* There should be a list of already accepted freinds for which we
     have already accepted the subscription. */
  if (0==_eXosip_notify_is_a_known_subscriber(evt->sip))
    code = 200;
  else
    code = 202;

  i = _eXosip_build_response_default(&answer, jd->d_dialog, code, evt->sip);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for subscribe\n"));
      osip_list_add(eXosip.j_transactions, transaction, 0);
      return;
    }

  _eXosip_notify_add_expires_in_2XX_for_subscribe(jn, answer);

  {
    char contact[200];
    sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	    localip,
	    localport);
    i = complete_answer_that_establish_a_dialog(answer, evt->sip,
						contact);
    if (i!=0)
      {
	osip_list_add(eXosip.j_transactions, transaction, 0);
	osip_message_free(answer);
	return;
      }
  }

  jn->n_inc_tr = transaction;

  /* eXosip_dialog_set_200ok(jd, answer); */
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;

  osip_transaction_add_event(transaction, evt_answer);

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);

  eXosip_update();
}

void eXosip_process_subscribe_within_call(eXosip_notify_t *jn,
					  eXosip_dialog_t *jd,
					  osip_transaction_t *transaction,
					  osip_event_t *evt)
{
  osip_message_t *answer;
  osip_event_t *sipevent;
  int i;

  _eXosip_notify_set_refresh_interval(jn, evt->sip);
  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }
  
  _eXosip_notify_add_expires_in_2XX_for_subscribe(jn, answer);

  {
    char contact [200];
    sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	    localip,
	    localport);
    i = complete_answer_that_establish_a_dialog(answer, evt->sip,
						contact);
    if (i!=0)
      {
	/* this info is yet known by the remote UA,
	   so we don't have to exit here */
      }
  }

  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, NULL, jn));
  sipevent = osip_new_outgoing_sipmessage(answer);
  sipevent->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction, sipevent);

  /* if subscribe request contains expires="0", close the subscription */
  {
    int now = time(NULL);
    if (jn->n_ss_expires-now<=0)
      {
	jn->n_ss_status=EXOSIP_SUBCRSTATE_TERMINATED;
	jn->n_ss_reason=TIMEOUT;
      }
  }

  osip_list_add(jd->d_inc_trs, transaction, 0);

  eXosip_notify_send_notify(jn, jd, jn->n_ss_status,
			    jn->n_online_status);
  return;
}

void
eXosip_process_notify_within_dialog(eXosip_subscribe_t *js,
				    eXosip_dialog_t *jd,
				    osip_transaction_t *transaction,
				    osip_event_t *evt)
{
  osip_message_t *answer;
  osip_event_t   *sipevent;
  osip_header_t  *sub_state;
#ifdef SUPPORT_MSN
  osip_header_t  *expires;
#endif
  int i;

  /* if subscription-state has a reason state set to terminated,
     we close the dialog */
#ifndef SUPPORT_MSN
  osip_message_header_get_byname(evt->sip, "subscription-state",
				 0,
				 &sub_state);
  if (sub_state==NULL||sub_state->hvalue==NULL)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 400);
      return;
    }
#endif

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }

#ifdef SUPPORT_MSN
  osip_message_header_get_byname(evt->sip, "expires",
				 0,
				 &expires);
  if (expires!=NULL||expires->hvalue!=NULL
      && 0==osip_strcasecmp(expires->hvalue, "0"))
    {
      /* delete the dialog! */
      REMOVE_ELEMENT(eXosip.j_subscribes, js);
      eXosip_subscribe_free(js);
    }
  else
    {
      osip_content_type_t *ctype;
      osip_body_t *body = NULL;
      osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, js, NULL));
      js->s_ss_status = EXOSIP_SUBCRSTATE_ACTIVE;
      js->s_online_status = EXOSIP_NOTIFY_UNKNOWN; /* default value */

      ctype = osip_message_get_content_type(evt->sip);
      if (ctype!=NULL && ctype->type!=NULL && ctype->subtype!=NULL)
	{
	  if (0==osip_strcasecmp(ctype->type, "application")
	      && 0==osip_strcasecmp(ctype->subtype, "xpidf+xml"))
	    osip_message_get_body(evt->sip, 0, &body);
	  else
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				    "Unknown body: %s/%s\n",
				    ctype->type, ctype->subtype));
	    }
	}

      if (body!=NULL)
	{
	  /* search for the open string */
	  char *tmp = body->body;
	  while (tmp!='\0')
	    {
	      if (tmp[0]=='o' && 0==strncmp(tmp, "online", 6))
		{
		  js->s_online_status = EXOSIP_NOTIFY_ONLINE;
		  /* search for the contact entry */
		  while (tmp[0]!='\0')
		    {
		      if (tmp[0]=='c' && 0==strncmp(tmp, "contact", 7))
			{
			  /* ... */
			}
		      tmp++;
		    }
		  break;
		}
	      else if (tmp[0]=='c' && 0==strncmp(tmp, "busy", 4))
		{
		  js->s_online_status = EXOSIP_NOTIFY_AWAY;
		  break;
		}
	      tmp++;
	    }
	}

    }
#else
  /* modify the status of user */
  if (0==osip_strncasecmp(sub_state->hvalue, "active", 6))
    {
      osip_content_type_t *ctype;
      osip_body_t *body = NULL;
      js->s_ss_status = EXOSIP_SUBCRSTATE_ACTIVE;
      js->s_online_status = EXOSIP_NOTIFY_UNKNOWN; /* default value */

      /* if there is a body which we understand, analyse it */
      ctype = osip_message_get_content_type(evt->sip);
      if (ctype!=NULL && ctype->type!=NULL && ctype->subtype!=NULL)
	{
	  if (0==osip_strcasecmp(ctype->type, "application")
	      && 0==osip_strcasecmp(ctype->subtype, "cpim-pidf+xml"))
	    osip_message_get_body(evt->sip, 0, &body);
	  else
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				    "Unknown body: %s/%s\n",
				    ctype->type, ctype->subtype));
	    }
	}

      if (body!=NULL)
	{
	  /* search for the open string */
	  char *tmp = body->body;
	  while (tmp!='\0')
	    {
	      if (tmp[0]=='o' && 0==strncmp(tmp, "open", 4))
		{
		  js->s_online_status = EXOSIP_NOTIFY_ONLINE;
		  /* search for the contact entry */
		  while (tmp[0]!='\0')
		    {
		      if (tmp[0]=='c' && 0==strncmp(tmp, "contact", 7))
			{
			  /* ... */
			}
		      tmp++;
		    }
		  break;
		}
	      else if (tmp[0]=='c' && 0==strncmp(tmp, "closed", 6))
		{
		  js->s_online_status = EXOSIP_NOTIFY_AWAY;
		  break;
		}
	      tmp++;
	    }
	}
    }
  else if (0==osip_strncasecmp(sub_state->hvalue, "pending", 7))
    {
      js->s_ss_status = EXOSIP_SUBCRSTATE_PENDING;
      js->s_online_status = EXOSIP_NOTIFY_PENDING;
    }

  if (0==osip_strncasecmp(sub_state->hvalue, "terminated", 10))
    {
      /* delete the dialog! */
      js->s_ss_status = EXOSIP_SUBCRSTATE_TERMINATED;
      js->s_online_status = EXOSIP_NOTIFY_UNKNOWN;
      REMOVE_ELEMENT(eXosip.j_subscribes, js);
      eXosip_subscribe_free(js);
    }
  else
    {
      osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, js, NULL));
    }
#endif

  osip_list_add(jd->d_inc_trs, transaction, 0);

  sipevent = osip_new_outgoing_sipmessage(answer);
  sipevent->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction, sipevent);
  return;
}


void eXosip_process_newrequest (osip_event_t *evt)
{
  osip_transaction_t *transaction;
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;
  int ctx_type;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t *jn;
  eXosip_dialog_t *jd;

  if (MSG_IS_INVITE(evt->sip))
    {
      ctx_type = IST;
    }
  else if (MSG_IS_ACK(evt->sip))
    { /* this should be a ACK for 2xx (but could be a late ACK!) */
      ctx_type = -1;
      osip_message_free(evt->sip);
      osip_free(evt);
      return ;	
    }
  else if (MSG_IS_REQUEST(evt->sip))
    {
      ctx_type = NIST;
    }
  else
    { /* We should handle late response and 200 OK before coming here. */
      ctx_type = -1;
      osip_message_free(evt->sip);
      osip_free(evt);
      return ;	
    }

  transaction=NULL;
  if (ctx_type != -1)
    {
      i = osip_transaction_init(&transaction,
			   (osip_fsm_type_t)ctx_type,
			   eXosip.j_osip,
			   evt->sip);
      if (i!=0)
	{
	  osip_message_free(evt->sip);
	  osip_free(evt);
	  return ;
	}

      evt->transactionid =  transaction->transactionid;
      osip_transaction_set_your_instance(transaction, NULL);

      i = _eXosip_build_response_default(&answer, NULL, 100, evt->sip);
      if (i!=0)
	{
	  __eXosip_delete_jinfo(transaction);
	  osip_transaction_free(transaction);
	  osip_message_free(evt->sip);
	  osip_free(evt);
	  return ;
	}
	
      osip_message_set_content_length(answer, "0");
      /*  send message to transaction layer */
	
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid = transaction->transactionid;
	
      /* add the REQUEST & the 100 Trying */
      osip_transaction_add_event(transaction, evt);
      osip_transaction_add_event(transaction, evt_answer);
    }

  if (MSG_IS_CANCEL(evt->sip))
    {
      /* special handling for CANCEL */
      /* in the new spec, if the CANCEL has a Via branch, then it
	 is the same as the one in the original INVITE */
      eXosip_process_cancel(transaction, evt);
      return ;
    }

  jd = NULL;
  /* first, look for a Dialog in the map of element */
  for (jc = eXosip.j_calls; jc!= NULL ; jc=jc->next)
    {
      for (jd = jc->c_dialogs; jd!= NULL ; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL)
	    {
	      if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip)==0)
		break;
	    }
	}
      if (jd!=NULL) break;
    }


  if (jd!=NULL)
    {
      osip_transaction_t *old_trn;
      /* it can be:
	 1: a new INVITE offer.
	 2: a REFER request from one of the party.
	 2: a BYE request from one of the party.
	 3: a REQUEST with a wrong CSeq.
	 4: a NOT-SUPPORTED method with a wrong CSeq.
      */
      
      if (!MSG_IS_BYE(evt->sip))
	{
	  /* reject all requests for a closed dialog */
	  old_trn = eXosip_find_last_inc_bye(jc, jd);
	  if (old_trn!=NULL)
	    {
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 481);
	      return ;
	    }
	  
	  old_trn = eXosip_find_last_out_bye(jc, jd);
	  if (old_trn!=NULL)
	    {
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 481);
	      return ;
	    }
	}

      if (MSG_IS_INVITE(evt->sip))
	{
	  /* the previous transaction MUST be freed */
	  old_trn = eXosip_find_last_inc_invite(jc, jd);
	    
	  if (old_trn!=NULL && old_trn->state!=IST_TERMINATED)
	    {
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  old_trn = eXosip_find_last_out_invite(jc, jd);
	  if (old_trn!=NULL && old_trn->state!=ICT_TERMINATED)
	    {
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 491);
	      return ;
	    }

	  osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
	  osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

	  eXosip_process_invite_within_call(jc, jd, transaction, evt);
	}
      else if (MSG_IS_BYE(evt->sip))
	{
	  old_trn = eXosip_find_last_inc_bye(jc, jd);
	    
	  if (old_trn!=NULL) /* && old_trn->state!=NIST_TERMINATED) */
	    { /* this situation should NEVER occur?? (we can't receive
		 two different BYE for one call! */
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return;
	    }
	  /* osip_transaction_free(old_trn); */
	  eXosip_process_bye(jc, jd, transaction, evt);
	}
      else if (MSG_IS_ACK(evt->sip))
	{
	}
      else if (MSG_IS_REFER(evt->sip))
	{
	  osip_list_add(eXosip.j_transactions, transaction, 0);
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      else if (MSG_IS_OPTIONS(evt->sip))
	{
	  eXosip_process_options(jc, jd, transaction, evt);
	}
      else
	{
	  osip_list_add(eXosip.j_transactions, transaction, 0);
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      return ;
    }


  if (MSG_IS_OPTIONS(evt->sip))
    {
      eXosip_process_new_options(transaction, evt);
      return;
    }
  else if (MSG_IS_INVITE(evt->sip))
    {
      eXosip_process_new_invite(transaction, evt);
      return;
    }
  else if (MSG_IS_BYE(evt->sip))
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(jd, transaction, evt, 481);
      return;
    }

  js = NULL;
  /* first, look for a Dialog in the map of element */
  for (js = eXosip.j_subscribes; js!= NULL ; js=js->next)
    {
      for (jd = js->s_dialogs; jd!= NULL ; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL)
	    {
	      if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip)==0)
		break;
	    }
	}
      if (jd!=NULL) break;
    }

  if (js!=NULL)
    {
      /* dialog found */
      osip_transaction_t *old_trn;
      /* it can be:
	 1: a new INVITE offer.
	 2: a REFER request from one of the party.
	 2: a BYE request from one of the party.
	 3: a REQUEST with a wrong CSeq.
	 4: a NOT-SUPPORTED method with a wrong CSeq.
      */
      if (MSG_IS_NOTIFY(evt->sip))
	{
	  /* the previous transaction MUST be freed */
	  old_trn = eXosip_find_last_inc_notify(js, jd);
	  
	  /* shouldn't we wait for the COMPLETED state? */
	  if (old_trn!=NULL && old_trn->state!=NIST_TERMINATED)
	    {
	      /* retry later? */
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
	  osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

	  eXosip_process_notify_within_dialog(js, jd, transaction, evt);
	}
      else
	{
	  osip_list_add(eXosip.j_transactions, transaction, 0);
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      return ;
    }

  if (MSG_IS_NOTIFY(evt->sip))
    {
      osip_list_add(eXosip.j_transactions, transaction, 0);
      eXosip_send_default_answer(NULL, transaction, evt, 481);
      return;
    }

  jn = NULL;
  /* first, look for a Dialog in the map of element */
  for (jn = eXosip.j_notifies; jn!= NULL ; jn=jn->next)
    {
      for (jd = jn->n_dialogs; jd!= NULL ; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL)
	    {
	      if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip)==0)
		break;
	    }
	}
      if (jd!=NULL) break;
    }

  if (jn!=NULL)
    {
      /* dialog found */
      osip_transaction_t *old_trn;
      /* it can be:
	 1: a new INVITE offer.
	 2: a REFER request from one of the party.
	 2: a BYE request from one of the party.
	 3: a REQUEST with a wrong CSeq.
	 4: a NOT-SUPPORTED method with a wrong CSeq.
      */
      if (MSG_IS_SUBSCRIBE(evt->sip))
	{
	  /* the previous transaction MUST be freed */
	  old_trn = eXosip_find_last_inc_subscribe(jn, jd);
	  
	  /* shouldn't we wait for the COMPLETED state? */
	  if (old_trn!=NULL && old_trn->state!=NIST_TERMINATED
	      && old_trn->state!=NIST_COMPLETED)
	    {
	      /* retry later? */
	      osip_list_add(eXosip.j_transactions, transaction, 0);
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
	  osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

	  eXosip_process_subscribe_within_call(jn, jd, transaction, evt);
	}
      else
	{
	  osip_list_add(eXosip.j_transactions, transaction, 0);
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      return ;
    }

  if (MSG_IS_SUBSCRIBE(evt->sip))
    {

      if (0==eXosip_event_package_is_supported(transaction, evt))
	{
	  return;
	}	
      eXosip_process_new_subscribe(transaction, evt);
      return;
    }

  /* default answer */
  osip_list_add(eXosip.j_transactions, transaction, 0);
  eXosip_send_default_answer(NULL, transaction, evt, 501);
}

void eXosip_process_response_out_of_transaction (osip_event_t *evt)
{
  osip_message_free(evt->sip);
  osip_free(evt);
}

/* if second==-1 && useconds==-1  -> wait for ever 
   if max_message_nb<=0  -> infinite loop....  */
int eXosip_read_message   ( int max_message_nb, int sec_max, int usec_max )
{
  fd_set osip_fdset;
  struct timeval tv;
  char *buf;
  
  tv.tv_sec = sec_max;
  tv.tv_usec = usec_max;
  
  buf = (char *)osip_malloc(SIP_MESSAGE_MAX_LENGTH*sizeof(char)+1);
  while (max_message_nb!=0 && eXosip.j_stop_ua==0)
    {
      int i;
      FD_ZERO(&osip_fdset);
      FD_SET(eXosip.j_socket, &osip_fdset);
      
      if ((sec_max==-1)||(usec_max==-1))
	i = select(eXosip.j_socket+1, &osip_fdset, NULL, NULL, NULL);
      else
	i = select(eXosip.j_socket+1, &osip_fdset, NULL, NULL, &tv);
      
      if (0==i || eXosip.j_stop_ua!=0)
	{
	}
      else if (-1==i)
	{
	  osip_free(buf);
	  return -2; /* error */
	}
      else
	{
	  struct sockaddr_in sa;
#ifdef __linux
	  socklen_t slen;
#else
	  int slen;
#endif
	  slen = sizeof(struct sockaddr_in);
	  i = recvfrom (eXosip.j_socket, buf, SIP_MESSAGE_MAX_LENGTH, 0,
			(struct sockaddr *) &sa, &slen);
	  if( i > 0 )
	    {
	      /* Message might not end with a "\0" but we know the number of */
	      /* char received! */
	      osip_transaction_t *transaction = NULL;
	      osip_event_t *sipevent;
	      osip_strncpy(buf+i,"\0",1);
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "Received message: \n%s\n", buf));
	      sipevent = osip_parse(buf);
	      osip_message_fix_last_via_header(sipevent->sip, inet_ntoa (sa.sin_addr), ntohs (sa.sin_port));
	      transaction = NULL;
	      if (sipevent!=NULL&&sipevent->sip!=NULL)
		{
		  i = osip_find_transaction_and_add_event(eXosip.j_osip, sipevent);
		  if (i!=0)
		    {
		      /* this event has no transaction, */
		      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				  "This is a request\n", buf));
		      eXosip_lock();
		      if (MSG_IS_REQUEST(sipevent->sip))
			eXosip_process_newrequest(sipevent);
		      else if (MSG_IS_RESPONSE(sipevent->sip))
			eXosip_process_response_out_of_transaction(sipevent);
		      eXosip_unlock();
		    }
		  else
		    {
		      /* handled by oSIP !*/
		    }
		}
	      else
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
			      "Could not parse SIP message\n"));
		  osip_message_free(sipevent->sip);
		  osip_free(sipevent);
		}
	    }
	  else
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
			  "Could not read socket\n"));
	    }
	}
      max_message_nb--;
    }
  osip_free(buf);
  return 0;
}


int eXosip_pendingosip_transaction_exist ( eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *tr;
  int now = time(NULL);

  tr = eXosip_find_last_inc_bye(jc, jd);
  if (tr!=NULL && tr->state!=NIST_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  osip_remove_transaction(eXosip.j_osip, tr);
	  eXosip_remove_transaction_from_call(tr, jc);
	  osip_transaction_free(tr);
	}
      else
	return 0;
    }

  tr = eXosip_find_last_out_bye(jc, jd);
  if (tr!=NULL && tr->state!=NICT_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  osip_remove_transaction(eXosip.j_osip, tr);
	  eXosip_remove_transaction_from_call(tr, jc);
	  osip_transaction_free(tr);
	}
      else
	return 0;
    }

  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr!=NULL && tr->state!=IST_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  /* osip_remove_transaction(eXosip.j_osip, tr);
	     eXosip_remove_transaction_from_call(tr, jc);
	     osip_transaction_free(tr); */
	}
      else
	return 0;
    }

  tr = eXosip_find_last_out_invite(jc, jd);
  if (tr!=NULL && tr->state!=ICT_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  /* osip_remove_transaction(eXosip.j_osip, tr);
	     eXosip_remove_transaction_from_call(tr, jc);
	     osip_transaction_free(tr); */
	}
      else
	return 0;
    }

  tr = eXosip_find_last_inc_refer(jc, jd);
  if (tr!=NULL && tr->state!=IST_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  osip_remove_transaction(eXosip.j_osip, tr);
	  eXosip_remove_transaction_from_call(tr, jc);
	  osip_transaction_free(tr);
	}
      else
	return 0;
    }

  tr = eXosip_find_last_out_refer(jc, jd);
  if (tr!=NULL && tr->state!=NICT_TERMINATED)
    { /* Don't want to wait forever on broken transaction!! */
      if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  /* remove the transaction from oSIP: */
	  osip_remove_transaction(eXosip.j_osip, tr);
	  eXosip_remove_transaction_from_call(tr, jc);
	  osip_transaction_free(tr);
	}
      else
	return 0;
    }

  return -1;
}

int eXosip_release_finished_calls ( eXosip_call_t *jc, eXosip_dialog_t *jd )
{    
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_bye(jc, jd);
  if (tr==NULL)
    tr = eXosip_find_last_out_bye(jc, jd);

  if (tr!=NULL &&
      ( tr->state==NIST_TERMINATED || tr->state==NICT_TERMINATED ))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
			    "eXosip: eXosip_release_finished_calls remove a dialog\n"));
      /* Remove existing reference to the dialog from transactions! */
      __eXosip_call_remove_dialog_reference_in_call(jc, jd);
      REMOVE_ELEMENT(jc->c_dialogs, jd);
      eXosip_dialog_free(jd);
      return 0;
    }
  return -1;
}

int eXosip_release_aborted_calls ( eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  int now = time(NULL);
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    tr = eXosip_find_last_out_invite(jc, jd);

  if (tr==NULL)
    {
      if (jd!=NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				"eXosip: eXosip_release_aborted_calls remove an empty dialog\n"));
      __eXosip_call_remove_dialog_reference_in_call(jc, jd);
	  REMOVE_ELEMENT(jc->c_dialogs, jd);
	  eXosip_dialog_free(jd);
	  return 0;
	}
      return -1;
    }

  if (tr!=NULL
      && tr->state!=IST_TERMINATED
      && tr->state!=ICT_TERMINATED
      && tr->birth_time+180<now) /* Wait a max of 2 minutes */
    {
      if (jd!=NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				"eXosip: eXosip_release_aborted_calls remove a dialog for an unfinished transaction\n"));
	  __eXosip_call_remove_dialog_reference_in_call(jc, jd);
	  REMOVE_ELEMENT(jc->c_dialogs, jd);
	  {
	    eXosip_event_t *je;
	    je = eXosip_event_init_for_call(EXOSIP_CALL_NOANSWER, jc, jd);
	    if (eXosip.j_call_callbacks[EXOSIP_CALL_NOANSWER]!=NULL)
	      eXosip.j_call_callbacks[EXOSIP_CALL_NOANSWER](EXOSIP_CALL_NOANSWER, je);
	    else if (eXosip.j_runtime_mode==EVENT_MODE)
	      eXosip_event_add(je);
	  }
	  eXosip_dialog_free(jd);
	  return 0;
	}
    }

  if (tr!=NULL
      && (tr->state==IST_TERMINATED
	  || tr->state==ICT_TERMINATED))
    {
      if (tr==jc->c_inc_tr)
	{
	  if (jc->c_inc_tr->last_response==NULL)
	    {
	      /* OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
		 "eXosip: eXosip_release_aborted_calls transaction with no answer\n")); */
	    }
	  else if (MSG_IS_STATUS_3XX(jc->c_inc_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls answered with a 3xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_4XX(jc->c_inc_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls answered with a 4xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_5XX(jc->c_inc_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls answered with a 5xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_6XX(jc->c_inc_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls answered with a 6xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	}
      else if (tr==jc->c_out_tr)
	{
	  if (jc->c_out_tr->last_response==NULL)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls completed with no answer\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_3XX(jc->c_out_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls completed answered with 3xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_4XX(jc->c_out_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls completed answered with 4xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_5XX(jc->c_out_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls completed answered with 5xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	  else if (MSG_IS_STATUS_6XX(jc->c_out_tr->last_response))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				    "eXosip: eXosip_release_aborted_calls completed answered with 6xx\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, jd);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	      return 0;
	    }
	}
    }

  return -1;
}


void eXosip_release_terminated_calls ( void )
{
  eXosip_dialog_t *jd;
  eXosip_dialog_t *jdnext;
  eXosip_call_t *jc;
  eXosip_call_t *jcnext;
  int now = time(NULL);
  int pos;


  for (jc = eXosip.j_calls ; jc != NULL; )
    {
      jcnext=jc->next;
      /* free call terminated with a BYE */
      for (jd = jc->c_dialogs ; jd != NULL; )
	{
	  jdnext=jd->next;
	  if (0==eXosip_pendingosip_transaction_exist(jc, jd))
	    { }
	  else if (0==eXosip_release_finished_calls(jc, jd))
	    { jd = jc->c_dialogs; }
	  else if (0==eXosip_release_aborted_calls(jc, jd))
	    { jdnext = NULL; }
	  else
	    { }
	  jd=jdnext;
	}
      jc=jcnext;
    }

  for (jc = eXosip.j_calls ; jc != NULL; )
    {
      jcnext=jc->next;
      if (jc->c_dialogs==NULL)
	{
	  /* release call for options requests */
	  if (jc->c_inc_options_tr!=NULL)
	    {
	      if (jc->c_inc_options_tr->state==NIST_TERMINATED)
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
					"eXosip: remove an incoming OPTIONS with no final answer\n"));
		  REMOVE_ELEMENT(eXosip.j_calls, jc);
		  {
		    eXosip_event_t *je;
		    je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		    if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		      eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		    else if (eXosip.j_runtime_mode==EVENT_MODE)
		      eXosip_event_add(je);
		  }
		  eXosip_call_free(jc);
		}
	      else if (jc->c_inc_options_tr->state!=NIST_TERMINATED
		       && jc->c_inc_options_tr->birth_time+180<now)
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
					"eXosip: remove an incoming OPTIONS with no final answer\n"));
		  REMOVE_ELEMENT(eXosip.j_calls, jc);
		  {
		    eXosip_event_t *je;
		    je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		    if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		      eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		    else if (eXosip.j_runtime_mode==EVENT_MODE)
		      eXosip_event_add(je);
		  }
		  eXosip_call_free(jc);
		}
	    }
	  else if (jc->c_out_options_tr!=NULL)
	    {
	      if (jc->c_out_options_tr->state==NICT_TERMINATED)
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
					"eXosip: remove an outgoing OPTIONS with no final answer\n"));
		  REMOVE_ELEMENT(eXosip.j_calls, jc);
		  {
		    eXosip_event_t *je;
		    je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		    if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		      eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		    else if (eXosip.j_runtime_mode==EVENT_MODE)
		      eXosip_event_add(je);
		  }
		  eXosip_call_free(jc);
		}
	      else if (jc->c_out_options_tr->state!=NIST_TERMINATED
		       && jc->c_out_options_tr->birth_time+180<now)
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
					"eXosip: remove an outgoing OPTIONS with no final answer\n"));
		  REMOVE_ELEMENT(eXosip.j_calls, jc);
		  {
		    eXosip_event_t *je;
		    je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		    if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		      eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		    else if (eXosip.j_runtime_mode==EVENT_MODE)
		      eXosip_event_add(je);
		  }
		  eXosip_call_free(jc);
		}
	    }
	  else if (jc->c_inc_tr!=NULL && jc->c_inc_tr->state!=IST_TERMINATED
	      && jc->c_inc_tr->birth_time+180<now)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "eXosip: remove an incoming call with no final answer\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	    }
	  else if (jc->c_out_tr!=NULL && jc->c_out_tr->state!=ICT_TERMINATED
		   && jc->c_out_tr->birth_time+180<now)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "eXosip: remove an outgoing call with no final answer\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	    }
	  else if (jc->c_inc_tr!=NULL && jc->c_inc_tr->state!=IST_TERMINATED)
	    {  }
	  else if (jc->c_out_tr!=NULL && jc->c_out_tr->state!=ICT_TERMINATED)
	    {  }
	  else /* no active pending transaction */
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "eXosip: remove a call\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      {
		eXosip_event_t *je;
		je = eXosip_event_init_for_call(EXOSIP_CALL_RELEASED, jc, NULL);
		if (eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED]!=NULL)
		  eXosip.j_call_callbacks[EXOSIP_CALL_RELEASED](EXOSIP_CALL_RELEASED, je);
		else if (eXosip.j_runtime_mode==EVENT_MODE)
		  eXosip_event_add(je);
	      }
	      eXosip_call_free(jc);
	    }
	}
      jc = jcnext;
    }

  pos = 0;
  while (!osip_list_eol(eXosip.j_transactions, pos))
    {
      osip_transaction_t *tr = (osip_transaction_t*) osip_list_get(eXosip.j_transactions, pos);
      if (tr->state==IST_TERMINATED || tr->state==ICT_TERMINATED
	  || tr->state== NICT_TERMINATED || tr->state==NIST_TERMINATED)
	{ /* free (transaction is already removed from the oSIP stack) */
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		      "Release a terminated transaction\n"));
	  osip_list_remove(eXosip.j_transactions, pos);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
      else if (tr->birth_time+180<now) /* Wait a max of 2 minutes */
	{
	  osip_list_remove(eXosip.j_transactions, pos);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
      else
	pos++;
    }
}


