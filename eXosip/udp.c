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

#include <eXosip.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
  
  osip_list_add(eXosip.j_transactions, transaction, 0);
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

  osip_parser_set_content_length(answer, "0");
  
  if (status==500)
    osip_parser_set_retry_after(answer, "10");
  
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction,evt_answer);
  
}

void eXosip_process_bye(eXosip_call_t *jc, eXosip_dialog_t *jd,
		       osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_event_t *evt_answer;
  osip_message_t *answer;
  int i;

  osip_list_add(jd->d_inc_trs, transaction , 0);
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      return ;
    }
  osip_parser_set_content_length(answer, "0");
    
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction,evt_answer);

  /* Release the eXosip_dialog */
  osip_dialog_free(jd->d_dialog);
  jd->d_dialog = NULL;
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
  if (0 != via_match (invite->topvia, via))
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
      i = cancel_match_invite(jc->c_inc_tr, evt->sip);
      if (i==0) { 
	tr = jc->c_inc_tr;
	break;
      }
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
	  return ;
	}
      osip_parser_set_content_length(answer, "0");
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
	  return ;
	}
      osip_parser_set_content_length(answer, "0");
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
	  return ;
	}
      osip_parser_set_content_length(answer, "0");
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
	  return ;
	}
      osip_parser_set_content_length(answer, "0");
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
    sdp_message_t *old_remote_sdp = sdp_negotiation_ctx_get_remote_sdp(jc->c_ctx);
    if (old_remote_sdp!=NULL)
      {
	sdp_message_free(old_remote_sdp);
      }
    sdp_negotiation_ctx_set_remote_sdp(jc->c_ctx, remote_sdp);
    local_sdp = sdp_negotiation_ctx_get_local_sdp(jc->c_ctx);
    if (local_sdp!=NULL)
      {
	sdp_message_free(local_sdp);
	sdp_negotiation_ctx_set_local_sdp(jc->c_ctx, NULL);
      }
    local_sdp = NULL;
    i = sdp_negotiation_ctx_execute_negotiation(jc->c_ctx);

    if (i!=200)
      {
	eXosip_send_default_answer(jd, transaction, evt, i);
	return;
      }
    local_sdp = sdp_negotiation_ctx_get_local_sdp(jc->c_ctx);
  }

  if (remote_sdp==NULL)
    {
      sdp_message_t *local_sdp;
      sdp_build_offer(NULL, &local_sdp, "25563", NULL);
      sdp_negotiation_ctx_set_local_sdp(jc->c_ctx, local_sdp);

      if (local_sdp==NULL)
	{
	  eXosip_send_default_answer(jd, transaction, evt, 500);
	  return;
	}
    }

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }

  if (local_sdp!=NULL)
    {
      char *local_body;
      char *size;

      sdp_negotiation_ctx_set_local_sdp(jc->c_ctx, NULL);
      i = sdp_message_to_str(local_sdp, &local_body);
      sdp_message_free(local_sdp);
      if (i!=0) {
	eXosip_send_default_answer(jd, transaction, evt, 500);
	msg_free(answer);
	return ;
      } 
      i = osip_parser_set_body(answer, local_body);
      if (i!=0) {
	eXosip_send_default_answer(jd, transaction, evt, 500);
	osip_free(local_body);
	msg_free(answer);
	return;
      }
      size = (char *) osip_malloc(6*sizeof(char));
      sprintf(size,"%i",strlen(local_body));
      osip_free(local_body);
      osip_parser_set_content_length(answer, size);
      osip_free(size);
      i = osip_parser_set_header(answer, "content-type", "application/sdp");
      if (i!=0) {
	eXosip_send_default_answer(jd, transaction, evt, 500);
	msg_free(answer);
	return;
      }	

    }
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  sipevent = osip_new_outgoing_sipmessage(answer);
  sipevent->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction, sipevent);
}

void eXosip_process_invite_off_hold(eXosip_call_t *jc, eXosip_dialog_t *jd,
				  osip_transaction_t *transaction,
				  osip_event_t *evt, sdp_message_t *sdp)
{
  eXosip_process_invite_on_hold(jc, jd, transaction, evt, sdp);
}

void eXosip_process_new_invite(osip_transaction_t *transaction, osip_event_t *evt)
{
  osip_event_t *evt_answer;
  int i;
  eXosip_call_t *jc;
  eXosip_dialog_t *jd;
  osip_message_t *answer;
  char *contact;

  eXosip_call_init(&jc);
  /* eXosip_call_set_subect... */

  jc->c_inc_tr = transaction;
  ADD_ELEMENT(eXosip.j_calls, jc);

  i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot create dialog.");
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }
  osip_parser_set_content_length(answer, "0");
  contact = (char *) osip_malloc(30);
  sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	  localip,
	  localport);
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  if (i!=0)
    {
      msg_free(answer);
      osip_free(contact);
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      return ;
    }

  i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
  if (i!=0)
    {
      msg_free(answer);
      fprintf(stderr, "eXosip: cannot create dialog!\n");
      osip_free(contact);
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
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      osip_free(contact);
      return;
    }
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  osip_free(contact);
  if (i!=0)
    {
      msg_free(answer);
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      return ;
    }

  osip_parser_set_content_length(answer, "0");
  /*  send message to transaction layer */

  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;
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

  // Is this a "on hold" message?
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
	  //  We received an INVITE to put on hold the other party.
	  eXosip_process_invite_on_hold(jc, jd, transaction, evt, sdp);
	  return;
	}
      else
	{
	  // This is a call modification, probably for taking it of hold
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
    code = 400;      /* Bad Request */
  else if (0!=osip_strcasecmp(event_hdr->hvalue, "presence"))
    code = 489;
  else code = 200;
  if (code!=200)
    {
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
  char *contact;
  int code;
  int i;

  eXosip_notify_init(&jn);
  _eXosip_notify_set_refresh_interval(jn, evt->sip);

  i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot create dialog.");
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      jn->n_inc_tr = transaction;
      eXosip_notify_free(jn);
      return;
    }
  contact = (char *) osip_malloc(30);
  sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	  localip,
	  localport);
  i = complete_answer_that_establish_a_dialog(answer, evt->sip, contact);
  if (i!=0)
    {
      msg_free(answer);
      osip_free(contact);
      fprintf(stderr, "eXosip: cannot complete answer!\n");
      jn->n_inc_tr = transaction;
      eXosip_notify_free(jn);
      return ;
    }

  i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
  if (i!=0)
    {
      msg_free(answer);
      fprintf(stderr, "eXosip: cannot create dialog!\n");
      osip_free(contact);
      jn->n_inc_tr = transaction;
      eXosip_notify_free(jn);
      return ;
    }
  jn->n_inc_tr = transaction;
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
      return;
    }

  {
    char *contact;
    contact = (char *) osip_malloc(50);
    sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	    localip,
	    localport);
    i = complete_answer_that_establish_a_dialog(answer, evt->sip,
						contact);
    osip_free(contact);
    if (i!=0)
      {
	msg_free(answer);
	return;
      }
  }

  //  eXosip_dialog_set_200ok(jd, answer);
  evt_answer = osip_new_outgoing_sipmessage(answer);
  evt_answer->transactionid = transaction->transactionid;

  osip_transaction_add_event(transaction, evt_answer);

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);

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
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }
  
  {
    char *contact;
    contact = (char *) osip_malloc(50);
    sprintf(contact, "<sip:%s@%s:%s>", evt->sip->to->url->username,
	    localip,
	    localport);
    i = complete_answer_that_establish_a_dialog(answer, evt->sip,
						contact);
    osip_free(contact);
    if (i!=0)
      {
	//msg_free(answer);
	//return;
	/* this info is yet known by the remote UA,
	   so we don't have to exit here */
      }
  }

  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, NULL, jn));
  sipevent = osip_new_outgoing_sipmessage(answer);
  sipevent->transactionid =  transaction->transactionid;
  osip_transaction_add_event(transaction, sipevent);

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
  osip_header_t  *expires;
  int i;

  /* if subscription-state has a reason state set to terminated,
     we close the dialog */
#ifndef SUPPORT_MSN
  osip_message_header_get_byname(evt->sip, "subscription-state",
				 0,
				 &sub_state);
  if (sub_state==NULL||sub_state->hvalue==NULL)
    {
      eXosip_send_default_answer(jd, transaction, evt, 400);
      return;
    }
#endif

  i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
  if (i!=0)
    {
      eXosip_send_default_answer(jd, transaction, evt, 500);
      return ;
    }

#ifdef SUPPORT_MSN
  osip_message_header_get_byname(evt->sip, "expires",
				 0,
				 &expires);
  if (expires==NULL||expires->hvalue==NULL)
    {
      osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, js, NULL));
    }
  else if (0==osip_strcasecmp(expires->hvalue, "0"))
    {
      /* delete the dialog! */
      eXosip_subscribe_free(js);
    }
  else
    {
      osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, js, NULL));
    }
#else
  if (0==osip_strcasecmp(sub_state->hvalue, "terminated"))
    {
      /* delete the dialog! */
      eXosip_subscribe_free(js);
    }
  else
    {
      osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, js, NULL));
    }
#endif


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
    }
  else if (MSG_IS_REQUEST(evt->sip))
    {
      ctx_type = NIST;
    }
  else
    { /* We should handle late response and 200 OK before coming here. */
      ctx_type = -1;
      msg_free(evt->sip);
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
	  msg_free(evt->sip);
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
	  msg_free(evt->sip);
	  osip_free(evt);
	  return ;
	}
	
      osip_parser_set_content_length(answer, "0");
      /*  send message to transaction layer */
	
      evt_answer = osip_new_outgoing_sipmessage(answer);
      evt_answer->transactionid = transaction->transactionid;
	
      // add the REQUEST
      // add the 100 Trying?
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
      if (MSG_IS_INVITE(evt->sip))
	{
	  /* the previous transaction MUST be freed */
	  old_trn = eXosip_find_last_inc_invite(jc, jd);
	    
	  if (old_trn!=NULL && old_trn->state!=IST_TERMINATED)
	    {
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  old_trn = eXosip_find_last_out_invite(jc, jd);
	  if (old_trn!=NULL && old_trn->state!=ICT_TERMINATED)
	    {
	      eXosip_send_default_answer(jd, transaction, evt, 491);
	      return ;
	    }

	  osip_list_add(jd->d_inc_trs, transaction, 0);

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
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	    }
	  /* osip_transaction_free2(old_trn); */
	  eXosip_process_bye(jc, jd, transaction, evt);
	}
      else if (MSG_IS_ACK(evt->sip))
	{
	  /*
	    ProcessAckWithinCall(pCall, evt);
	  */
	}
      else if (MSG_IS_REFER(evt->sip))
	{
	  /*
	    pCall->SetIncomingReferRequest(transaction);
	    osip_transaction_set_your_instance(transaction, , __eXosip_new_jinfo(jc, jd));
	    ProcessReferWithinCall(pCall, transaction, evt);
	  */
	}
      else
	{
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      return ;
    }


  if (MSG_IS_INVITE(evt->sip))
    {
      eXosip_process_new_invite(transaction, evt);
      return;
    }
  if (MSG_IS_BYE(evt->sip))
    {
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
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  osip_list_add(jd->d_inc_trs, transaction, 0);

	  osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
	  osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

	  eXosip_process_notify_within_dialog(js, jd, transaction, evt);
	}
      else
	{
	  eXosip_send_default_answer(jd, transaction, evt, 501);
	}
      return ;
    }

  if (MSG_IS_NOTIFY(evt->sip))
    {
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
	      eXosip_send_default_answer(jd, transaction, evt, 500);
	      return ;
	    }

	  osip_list_add(jd->d_inc_trs, transaction, 0);

	  osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
	  osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

	  eXosip_process_subscribe_within_call(jn, jd, transaction, evt);
	}
      else
	{
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
  eXosip_send_default_answer(NULL, transaction, evt, 501);
      
}

void eXosip_process_response_out_of_transaction (osip_event_t *evt)
{
  msg_free(evt->sip);
  osip_free(evt);
}

// if second==-1 && useconds==-1  -> wait for ever 
// if max_message_nb<=0  -> infinite loop.... 
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
	      msg_fix_last_via_header(sipevent->sip, inet_ntoa (sa.sin_addr), ntohs (sa.sin_port));
	      transaction = NULL;
	      if (sipevent!=NULL&&sipevent->sip!=NULL)
		{
		  i = osip_find_osip_transaction_and_add_event(eXosip.j_osip, sipevent);
		  if (i!=0)
		    {
		      // this event has no transaction,
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
		  msg_free(sipevent->sip);
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
	  osip_remove_nist(eXosip.j_osip, tr);
	  tr->state=NIST_TERMINATED;
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
	  osip_remove_nict(eXosip.j_osip, tr);
	  tr->state=NICT_TERMINATED;
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
	  osip_remove_ist(eXosip.j_osip, tr);
	  tr->state=IST_TERMINATED;
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
	  osip_remove_ict(eXosip.j_osip, tr);
	  tr->state=ICT_TERMINATED;
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
	  osip_remove_nist(eXosip.j_osip, tr);
	  tr->state=NIST_TERMINATED;
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
	  osip_remove_nict(eXosip.j_osip, tr);
	  tr->state=NICT_TERMINATED;
	}
      else
	return 0;
    }

  // if (pCall->GetDialog()!=NULL) return 0;

  return -1;
}

int eXosip_release_finished_calls ( eXosip_call_t *jc, eXosip_dialog_t *jd )
{    
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_bye(jc, jd);
  if (tr==NULL)
    tr = eXosip_find_last_out_bye(jc, jd);

  if (tr!=NULL &&
      ( tr->state==NIST_TERMINATED || tr->state==NICT_TERMINATED )
      && tr->last_response!=NULL
      && MSG_IS_STATUS_2XX(tr->last_response))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			    "eXosip: remove a dialog\n"));
      REMOVE_ELEMENT(jc->c_dialogs, jd);
      eXosip_dialog_free(jd);
      return 0;
    }
  return -1;
}

int eXosip_release_aborted_calls ( eXosip_call_t *jc, eXosip_dialog_t *jd )
{
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    tr = eXosip_find_last_out_invite(jc, jd);

  if (tr!=NULL &&
      ( tr->state==IST_TERMINATED || tr->state==ICT_TERMINATED )
      && (jd==NULL || jd->d_dialog == NULL ))
    {
      if (jd!=NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"eXosip: remove a dialog\n"));
	  REMOVE_ELEMENT(jc->c_dialogs, jd);
	  eXosip_dialog_free(jd);
	}
      
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "Release a non established call.\n"));
      return 0;
    }
  return -1;
}


void eXosip_release_terminated_calls ( void )
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;

  int pos;

  for (jc = eXosip.j_calls ; jc != NULL; jc=jc->next)
    {
      /* free call terminated with a BYE */
      for (jd = jc->c_dialogs ; jd != NULL; )
	{
	  if (0==eXosip_pendingosip_transaction_exist(jc, jd))
	    { jd=jd->next; }
	  else if (0==eXosip_release_finished_calls(jc, jd))
	    { jd = NULL; jc = eXosip.j_calls; }
	  else if (0==eXosip_release_aborted_calls(jc, jd))
	    { jd = NULL; jc = eXosip.j_calls; }
	  else
	    { jd=jd->next; }
	}
    }

  for (jc = eXosip.j_calls ; jc != NULL; )
    {
      if (jc->c_dialogs==NULL)
	{
	  if (jc->c_inc_tr!=NULL && jc->c_inc_tr->state!=IST_TERMINATED)
	    { jc=jc->next; }
	  else if (jc->c_out_tr!=NULL && jc->c_out_tr->state!=ICT_TERMINATED)
	    { jc=jc->next; }
	  else /* no active pending transaction */
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "eXosip: remove a call\n"));
	      REMOVE_ELEMENT(eXosip.j_calls, jc);
	      eXosip_call_free(jc);
	      jc = eXosip.j_calls;
	    }
	}
      else
	jc = jc->next;
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
	  osip_transaction_free2(tr);
	}
      else
	pos++;
    }
}


