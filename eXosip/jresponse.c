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

extern char *localip;
extern char *localport;

int
eXosip_build_response_default(int jid, int status)
{
  return -1;
}

int
_eXosip_build_response_default(osip_message_t **dest, osip_dialog_t *dialog,
			     int status, osip_message_t *request)
{
  osip_generic_param_t *tag;
  osip_message_t *response;
  int pos;
  int i;

  i = msg_init(&response);
  /* initialise osip_message_t structure */
  /* yet done... */

  response->sipversion = (char *)osip_malloc(8*sizeof(char));
  sprintf(response->sipversion,"SIP/2.0");
  response->statuscode = (char *)osip_malloc(5*sizeof(char));
  sprintf(response->statuscode,"%i",status);
  response->reasonphrase = osip_parser_get_reason(status);
  if (response->reasonphrase==NULL)
    {
      if (0==strcmp(response->statuscode, "101"))
	response->reasonphrase = osip_strdup("Dialog Establishement");
      else
	response->reasonphrase = osip_strdup("Unknown code");
    }
  response->rquri     = NULL;
  response->sipmethod = NULL;

  i = osip_to_clone(request->to, &(response->to));
  if (i!=0) goto grd_error_1;


  i = osip_to_get_tag(response->to,&tag);
  if (i!=0)
    {  /* we only add a tag if it does not already contains one! */
      if ((dialog!=NULL) && (dialog->local_tag!=NULL))
	/* it should contain the local TAG we created */
	{ osip_to_set_tag(response->to, osip_strdup(dialog->local_tag)); }
      else
	{
	  if (status!=100)
	    osip_to_set_tag(response->to, osip_to_tag_new_random());
	}
    }

  i = osip_from_clone(request->from, &(response->from));
  if (i!=0) goto grd_error_1;

  pos = 0;
  while (!osip_list_eol(request->vias,pos))
    {
      osip_via_t *via;
      osip_via_t *via2;
      via = (osip_via_t *)osip_list_get(request->vias,pos);
      i = osip_via_clone(via, &via2);
      if (i!=-0) goto grd_error_1;
      osip_list_add(response->vias, via2, -1);
      pos++;
    }

  i = osip_call_id_clone(request->call_id, &(response->call_id));
  if (i!=0) goto grd_error_1;
  i = osip_cseq_clone(request->cseq, &(response->cseq));
  if (i!=0) goto grd_error_1;
    
  *dest = response;
  return 0;

 grd_error_1:
  msg_free(response);
  return -1;
}

int
complete_answer_that_establish_a_dialog(osip_message_t *response, osip_message_t *request, char *contact)
{
  int i;
  int pos=0;
  /* 12.1.1:
     copy all record-route in response
     add a contact with global scope
  */
  while (!osip_list_eol(request->record_routes, pos))
    {
      osip_record_route_t *rr;
      osip_record_route_t *rr2;
      rr = osip_list_get(request->record_routes, pos);
      i = osip_record_route_clone(rr, &rr2);
      if (i!=0) return -1;
      osip_list_add(response->record_routes, rr2, -1);
      pos++;
    }
  osip_parser_set_contact(response, contact);
  return 0;
}

char *
generating_sdp_answer(osip_message_t *request)
{
  sdp_negotiation_ctx_t *context;
  sdp_message_t *remote_sdp;
  sdp_message_t *local_sdp = NULL;
  int i;
  char *local_body;
  local_body = NULL;
  if (MSG_IS_INVITE(request)||MSG_IS_OPTIONS(request))
    {
      osip_body_t *body;
      body = (osip_body_t *)osip_list_get(request->bodies,0);
      
      /* remote_sdp = (sdp_message_t *) osip_malloc(sizeof(sdp_message_t)); */
      i = sdp_message_init(&remote_sdp);
      if (i!=0) return NULL;
      
      /* WE ASSUME IT IS A SDP BODY AND THAT    */
      /* IT IS THE ONLY ONE, OF COURSE, THIS IS */
      /* NOT TRUE */
      i = sdp_message_parse(remote_sdp,body->body);
      if (i!=0) return NULL;      

      i = sdp_negotiation_ctx_init(&context);
      if (i!=0)
	{
	  sdp_message_free(remote_sdp);
	  return NULL;
	}
      i = sdp_negotiation_ctx_set_remote_sdp(context, remote_sdp);

      i = sdp_negotiation_ctx_execute_negotiation(context);
      if (i==200)
	{
	  local_sdp = sdp_negotiation_ctx_get_local_sdp(context);
	  i = sdp_message_to_str(local_sdp, &local_body);

	  sdp_negotiation_ctx_free(context);
	  if (i!=0) {
	    OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not parse local SDP answer %i\n",i));
	    return NULL;
	  }
	  return local_body;
	}
      else if (i==415)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"WARNING: Unsupported media %i\n",i));
	}
      else
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: while building answer to SDP (%i)\n",i));
	}
      sdp_negotiation_ctx_free(context);
    } 
  return NULL;
}

void
generating_1xx_answer_osip_to_options(osip_dialog_t *dialog, osip_transaction_t *tr, int code)
{
  osip_message_t *response;
  int i;

  i = _eXosip_build_response_default(&response, dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }

  osip_parser_set_content_length(response, "0");

  return ;
}

void
generating_2xx_answer_osip_to_options(osip_dialog_t *dialog, osip_transaction_t *tr, int code)
{
  osip_message_t *response;
  int i;
  char *size;
  char *body;
  body = generating_sdp_answer(tr->orig_request);
  if (body==NULL)
    {
      code = 488;
    }
  i = _eXosip_build_response_default(&response, dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for options\n"));
      code = 500; /* ? which code to use? */
      osip_free(body); /* not used */
      return;
    }

  if (code==488)
    {
      osip_parser_set_content_length(response, "0");
      /*  send message to transaction layer */
      osip_free(body);      
      return;
    }

  i = osip_parser_set_body(response, body);
  if (i!=0) {
    goto g2atii_error_1;
  }
  size = (char *) osip_malloc(6*sizeof(char));
  sprintf(size,"%i",strlen(body));
  i = osip_parser_set_content_length(response, size);
  osip_free(size);
  if (i!=0) goto g2atii_error_1;
  i = osip_parser_set_header(response, "content-type", "application/sdp");
  if (i!=0) goto g2atii_error_1;

  /* response should contains the allow and supported headers */
  osip_parser_set_allow(response, "INVITE");
  osip_parser_set_allow(response, "ACK");
  osip_parser_set_allow(response, "OPTIONS");
  osip_parser_set_allow(response, "CANCEL");
  osip_parser_set_allow(response, "BYE");


  osip_free(body);
  return ;

 g2atii_error_1:
  osip_free(body);
  msg_free(response);
  return ;
}

void
generating_3456xx_answer_osip_to_options(osip_dialog_t *dialog, osip_transaction_t *tr, int code)
{
  osip_message_t *response;
  int i;
  i = _eXosip_build_response_default(&response, dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for options\n"));
      return;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
    }

  osip_parser_set_content_length(response, "0");
  /*  send message to transaction layer */

  return ;
}

void
eXosip_answer_invite_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      fprintf(stderr, "eXosip: cannot find transaction to answer");
      return;
    }

  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }

  osip_parser_set_content_length(response, "0");
  /*  send message to transaction layer */

  if (code>100)
    {
      /* request that estabish a dialog: */
      /* 12.1.1 UAS Behavior */
      char *contact;
      contact = (char *) osip_malloc(50);
      sprintf(contact, "<sip:%s@%s:%s>", tr->orig_request->to->url->username,
	      localip,
	      localport);
      i = complete_answer_that_establish_a_dialog(response, tr->orig_request, contact);
      osip_free(contact);

      if (jd==NULL)
	{
	  i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
	  if (i!=0)
	    {
	      fprintf(stderr, "eXosip: cannot create dialog!\n");
	    }
	  ADD_ELEMENT(jc->c_dialogs, jd);
	}
    }

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  
  return ;
}

void
eXosip_answer_invite_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  char *size;
  char *body;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);

  if (tr==NULL || tr->orig_request==NULL)
    {
      fprintf(stderr, "eXosip: cannot find transaction to answer\n");
      return;
    }

  if (jd!=NULL && jd->d_dialog==NULL)
    {  /* element previously removed, this is a no hop! */
      fprintf(stderr, "eXosip: cannot answer this closed transaction\n");
      return ;
    }

  body = generating_sdp_answer(tr->orig_request);
  if (body==NULL)
    {
      code = 488;
    }
  if (jd==NULL)
    i = _eXosip_build_response_default(&response, NULL, code, tr->orig_request);
  else
    i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);

  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      code = 500; /* ? which code to use? */
      osip_free(body); /* not used */
      return;
    }

  if (code==488)
    {
      osip_parser_set_content_length(response, "0");
      /*  TODO: send message to transaction layer */
      osip_free(body);      
      evt_answer = osip_new_outgoing_sipmessage(response);
      evt_answer->transactionid = tr->transactionid;
      osip_transaction_add_event(tr, evt_answer);
      return;
    }

  i = osip_parser_set_body(response, body);
  if (i!=0) {
    goto g2atii_error_1;
  }
  size = (char *) osip_malloc(6*sizeof(char));
  sprintf(size,"%i",strlen(body));
  i = osip_parser_set_content_length(response, size);
  osip_free(size);
  if (i!=0) goto g2atii_error_1;
  i = osip_parser_set_header(response, "content-type", "application/sdp");
  if (i!=0) goto g2atii_error_1;

  /* request that estabish a dialog: */
  /* 12.1.1 UAS Behavior */
  {
    char *contact;
    contact = (char *) osip_malloc(50);
    sprintf(contact, "<sip:%s@%s:%s>", tr->orig_request->to->url->username,
	    localip,
	    localport);
    i = complete_answer_that_establish_a_dialog(response, tr->orig_request,
						contact);
    osip_free(contact);
    if (i!=0) goto g2atii_error_1;; /* ?? */
  }
  /* response should contains the allow and supported headers */
  osip_parser_set_allow(response, "INVITE");
  osip_parser_set_allow(response, "ACK");
  osip_parser_set_allow(response, "OPTIONS");
  osip_parser_set_allow(response, "CANCEL");
  osip_parser_set_allow(response, "BYE");

  osip_free(body);
  /* THIS RESPONSE MUST BE SENT RELIABILY until the final ACK is received !! */
  /* this response must be stored at the upper layer!!! (it will be destroyed*/
  /* right after being sent! */

  if (jd==NULL)
    {
      i = eXosip_dialog_init_as_uas(&jd, tr->orig_request, response);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot create dialog!\n");
	  return;
	}
      ADD_ELEMENT(jc->c_dialogs, jd);
    }

  eXosip_dialog_set_200ok(jd, response);
  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);

  osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
  return ;

 g2atii_error_1:
  osip_free(body);
  msg_free(response);
  return ;
}

void
eXosip_answer_invite_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code)
{
  osip_event_t *evt_answer;
  osip_message_t *response;
  int i;
  osip_transaction_t *tr;
  tr = eXosip_find_last_inc_invite(jc, jd);
  if (tr==NULL)
    {
      fprintf(stderr, "eXosip: cannot find transaction to answer");
      return;
    }
  i = _eXosip_build_response_default(&response, jd->d_dialog, code, tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"ERROR: Could not create response for invite\n"));
      return;
    }

  if (300<=code<=399)
    {
      /* Should add contact fields */
      /* ... */
    }

  osip_parser_set_content_length(response, "0");
  /*  send message to transaction layer */

  evt_answer = osip_new_outgoing_sipmessage(response);
  evt_answer->transactionid = tr->transactionid;

  osip_transaction_add_event(tr, evt_answer);
  return ;
}

