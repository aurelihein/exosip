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

#include "eXosip2.h"
#include <eXosip/eXosip.h>
#include <eXosip/eXosip_cfg.h>

#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>

#if defined WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif


eXosip_t eXosip;
char    *localip;
char    *localport;
extern char *register_callid_number;


void eXosip_get_localip(char *ip)
{
	eXosip_guess_ip_for_via(ip);
}

int
eXosip_lock()
{
  return osip_mutex_lock((struct osip_mutex*)eXosip.j_mutexlock);
}

int
eXosip_unlock()
{
  return osip_mutex_unlock((struct osip_mutex*)eXosip.j_mutexlock);
}


jfriend_t *jfriend_get()
{
  return eXosip.j_friends;
}

void jfriend_remove(jfriend_t *fr)
{
  REMOVE_ELEMENT(eXosip.j_friends, fr);
}

jsubscriber_t *jsubscriber_get()
{
  return eXosip.j_subscribers;
}

jidentity_t *jidentity_get()
{
  return eXosip.j_identitys;
}

void
eXosip_kill_transaction (osip_list_t * transactions)
{
  osip_transaction_t *transaction;

  if (!osip_list_eol (transactions, 0))
    {
      /* some transaction are still used by osip,
         transaction should be released by modules! */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: _osip_kill_transaction transaction should be released by modules!\n"));
    }

  while (!osip_list_eol (transactions, 0))
    {
      transaction = osip_list_get (transactions, 0);

      __eXosip_delete_jinfo(transaction);
      osip_transaction_free (transaction);
    }
}

void eXosip_quit()
{
  eXosip_call_t *jc;
  eXosip_reg_t  *jreg;
  int i;

  eXosip.j_stop_ua = 1; /* ask to quit the application */
  i = osip_thread_join((struct osip_thread*)eXosip.j_thread);
  if (i!=0)
    fprintf(stderr, "eXosip: can't terminate thread!");
  osip_free((struct osip_thread*)eXosip.j_thread);

  osip_free(localip);
  osip_free(localport);
  osip_free(register_callid_number);

  eXosip.j_input = 0;
  eXosip.j_output = 0;

  for (jc = eXosip.j_calls; jc!=NULL;jc = eXosip.j_calls)
    {
      REMOVE_ELEMENT(eXosip.j_calls, jc);
      eXosip_call_free(jc);
    }
  
  osip_mutex_destroy((struct osip_mutex*)eXosip.j_mutexlock);
  osip_mutex_destroy((struct osip_mutex*)eXosip.j_condmutex);
  osip_cond_destroy((struct osip_cond*)eXosip.j_cond);

  osip_negotiation_free(eXosip.osip_negotiation);  

  if (eXosip.j_input)
    fclose(eXosip.j_input);
  if (eXosip.j_output)
    osip_free(eXosip.j_output);
  if (eXosip.j_socket)
    close(eXosip.j_socket);

  for (jreg = eXosip.j_reg; jreg!=NULL; jreg = eXosip.j_reg)
    {
      REMOVE_ELEMENT(eXosip.j_reg, jreg);
      eXosip_reg_free(jreg);
    }

  /* should be moved to method with an argument */
  jfriend_unload();
  jidentity_unload();
  jsubscriber_unload();

  /*    
  for (jid = eXosip.j_identitys; jid!=NULL; jid = eXosip.j_identitys)
    {
      REMOVE_ELEMENT(eXosip.j_identitys, jid);
      eXosip_friend_free(jid);
    }

  for (jfr = eXosip.j_friends; jfr!=NULL; jfr = eXosip.j_friends)
    {
      REMOVE_ELEMENT(eXosip.j_friends, jfr);
      eXosip_reg_free(jfr);
    }
  */

  while (!osip_list_eol(eXosip.j_transactions, 0))
    {
      osip_transaction_t *tr = (osip_transaction_t*) osip_list_get(eXosip.j_transactions, 0);
      if (tr->state==IST_TERMINATED || tr->state==ICT_TERMINATED
	  || tr->state== NICT_TERMINATED || tr->state==NIST_TERMINATED)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		      "Release a terminated transaction\n"));
	  osip_list_remove(eXosip.j_transactions, 0);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
      else
	{
	  osip_list_remove(eXosip.j_transactions, 0);
	  __eXosip_delete_jinfo(tr);
	  osip_transaction_free(tr);
	}
    }

  osip_free(eXosip.j_transactions);

  eXosip_kill_transaction (eXosip.j_osip->osip_ict_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_nict_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_ist_transactions);
  eXosip_kill_transaction (eXosip.j_osip->osip_nist_transactions);
  osip_release (eXosip.j_osip);

  osip_fifo_free(eXosip.j_events);

  return ;
}

int eXosip_execute ( void )
{
  int i;
  i = eXosip_read_message(1, 0, 500000);
  if (i==-2)
    {
      return -2;
    }

  eXosip_lock();
  osip_timers_ict_execute(eXosip.j_osip);
  osip_timers_nict_execute(eXosip.j_osip);
  osip_timers_ist_execute(eXosip.j_osip);
  osip_timers_nist_execute(eXosip.j_osip);
  
  osip_ict_execute(eXosip.j_osip);
  osip_nict_execute(eXosip.j_osip);
  osip_ist_execute(eXosip.j_osip);
  osip_nist_execute(eXosip.j_osip);
  
  /* free all Calls that are in the TERMINATED STATE? */
  eXosip_release_terminated_calls();

  eXosip_unlock();

  return 0;
}

void *eXosip_thread        ( void *arg )
{
  int i;
  while (eXosip.j_stop_ua==0)
    {
      i = eXosip_execute();
      if (i==-2)
	osip_thread_exit();
    }
  osip_thread_exit();
  return NULL;
}

int eXosip_init(FILE *input, FILE *output, int port)
{
  osip_t *osip;
  int i;
  if (port<0)
    {
      fprintf(stderr, "eXosip: port must be higher than 0!\n");
      return -1;
    }
  localip = (char *) osip_malloc(30);
  memset(localip, '\0', 30);
  eXosip_guess_ip_for_via(localip);
  if (localip[0]=='\0')
    {
#ifdef ENABLE_DEBUG
      fprintf(stderr, "eXosip: No ethernet interface found!\n");
      fprintf(stderr, "eXosip: using 127.0.0.1 (debug mode)!\n");
      strcpy(localip, "127.0.0.1");
#else
      fprintf(stderr, "eXosip: No ethernet interface found!\n");
      return -1;
#endif
    }

  eXosip_set_mode(EVENT_MODE);
  eXosip.j_input = input;
  eXosip.j_output = output;
  eXosip.j_calls = NULL;
  eXosip.j_stop_ua = 0;
  eXosip.j_thread = NULL;
  eXosip.j_transactions = (osip_list_t*) osip_malloc(sizeof(osip_list_t));
  osip_list_init(eXosip.j_transactions);
  eXosip.j_reg = NULL;

  eXosip.j_cond      = (struct osip_cond*)osip_cond_init();
  eXosip.j_condmutex = (struct osip_mutex*)osip_mutex_init();

  eXosip.j_mutexlock = (struct osip_mutex*)osip_mutex_init();

  if (-1==osip_init(&osip))
    {
      fprintf(stderr, "eXosip: Cannot initialize osip!\n");
      return -1;
    }

  eXosip_sdp_negotiation_init(&(eXosip.osip_negotiation));

  eXosip_sdp_negotiation_add_codec(osip_strdup("0"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("0 PCMU/8000"));

  eXosip_sdp_negotiation_add_codec(osip_strdup("8"),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup("8 PCMA/8000"));

  osip_set_application_context(osip, &eXosip);
  
  eXosip_set_callbacks(osip);
  
  eXosip.j_osip = osip;

#ifdef WIN32
   /* Initializing windows socket library */
   {
     WORD wVersionRequested;
     WSADATA wsaData;

     wVersionRequested = MAKEWORD(1,1);
     if(i = WSAStartup(wVersionRequested,  &wsaData))
       {
	 fprintf(stderr, "eXosip: Unable to initialize WINSOCK, reason: %d\n",i);
	 /* return -1; It might be already initilized?? */
       }
   }
#endif

  /* open the UDP listener */
          
  eXosip.j_socket = (int)socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (eXosip.j_socket==-1)
    return -1;
  
  {
    struct sockaddr_in  raddr;
    raddr.sin_addr.s_addr = htons(INADDR_ANY);
    raddr.sin_port = htons((short)port);
    raddr.sin_family = AF_INET;
    
    i = bind(eXosip.j_socket, (struct sockaddr *)&raddr, sizeof(raddr));
    if (i < 0)
    {
      fprintf(stderr, "eXosip: Cannot bind on port: %i!\n", i);
      return -1;
    }
  }

  localport = (char*)osip_malloc(10);
  sprintf(localport, "%i", port);

  eXosip.j_thread = (void*) osip_thread_create(20000,eXosip_thread, NULL);
  if (eXosip.j_thread==NULL)
    {
      fprintf(stderr, "eXosip: Cannot start thread!\n");
      return -1;
    }

  /* To be changed in osip! */
  eXosip.j_events = (osip_fifo_t*) osip_malloc(sizeof(osip_fifo_t));
  osip_fifo_init(eXosip.j_events);

  jfriend_load();
  jidentity_load();
  jsubscriber_load();
  return 0;
}

void
eXosip_set_mode(int mode)
{
  eXosip.j_runtime_mode = mode;
}

void
eXosip_update()
{
  static int static_id  = 1;
  eXosip_call_t      *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t *jn;
  eXosip_dialog_t    *jd;
  int now;

  if (static_id>100000)
    static_id = 1; /* loop */

  now = time(NULL);
  for (jc=eXosip.j_calls; jc!=NULL; jc=jc->next)
    {
      if (jc->c_id<1)
	{
	  jc->c_id = static_id;
	  static_id++;
	}
      for (jd=jc->c_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	    }
	  else jd->d_id = -1;
	}
    }

  for (js=eXosip.j_subscribes; js!=NULL; js=js->next)
    {
      if (js->s_id<1)
	{
	  js->s_id = static_id;
	  static_id++;
	}
      for (jd=js->s_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	      if (eXosip_subscribe_need_refresh(js, now)==0)
		{
		  int i;
#define LOW_EXPIRE
#ifdef LOW_EXPIRE
		  i = eXosip_subscribe_send_subscribe(js, jd, "60");
#else
		  i = eXosip_subscribe_send_subscribe(js, jd, "600");
#endif
		}
	    }
	  else jd->d_id = -1;
	}
    }

  for (jn=eXosip.j_notifies; jn!=NULL; jn=jn->next)
    {
      if (jn->n_id<1)
	{
	  jn->n_id = static_id;
	  static_id++;
	}
      for (jd=jn->n_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_dialog!=NULL) /* finished call */
	    {
	      if (jd->d_id<1)
		{
		  jd->d_id = static_id;
		  static_id++;
		}
	    }
	  else jd->d_id = -1;
	}
    }
}

void eXosip_message    (char *to, char *from, char *route, char *buff)
{
  /* eXosip_call_t *jc;
     osip_header_t *subject; */
  osip_message_t *message;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;

  i = generating_message(&message, to, from, route, buff);
  if (i!=0) 
    {
      fprintf(stderr, "eXosip: cannot send message (cannot build MESSAGE)! ");
      return;
    }

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       message);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(message);
      return ;
    }
  
  osip_list_add(eXosip.j_transactions, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(message);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
}

int eXosip_info_call(int jid, char *content_type, char *body)
{
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *info;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  int i;
  
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }
  if (jd==NULL || jd->d_dialog==NULL)
    {
      fprintf(stderr, "eXosip: No established dialog!");
      return -1;
    }

  transaction = eXosip_find_last_options(jc, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
    }  
 
  i = generating_info_within_dialog(&info, jd->d_dialog);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot send info message! ");
      return -2;
    }
  
  osip_message_set_content_type(info, content_type);
  osip_message_set_body(info, body);
  
  i = osip_transaction_init(&transaction,
			    NICT,
			    eXosip.j_osip,
			    info);
  if (i!=0)
    {
      osip_message_free(info);
      return -1;
    }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(info);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);

  return 0;
}

extern osip_list_t *supported_codec;

int eXosip_initiate_call(osip_message_t *invite, void *reference,
			 void *sdp_context_reference,
			 char *local_sdp_port)
{
  eXosip_call_t *jc;
  osip_header_t *subject;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;
  
  if (local_sdp_port!=NULL)
    {
      osip_negotiation_sdp_build_offer(eXosip.osip_negotiation, NULL, &sdp, local_sdp_port, NULL);
      
      /*
	if speex codec is supported, add bandwith attribute:
	b=AS:110 20
	b=AS:111 20
      */
      if (sdp!=NULL)
	{
	  int pos=0;
	  while (!sdp_message_endof_media (sdp, pos))
	    {
	      int k = 0;
	      char *tmp = sdp_message_m_media_get (sdp, pos);
	      if (0 == strncmp (tmp, "audio", 5))
		{
		  char *payload = NULL;
		  do {
		    payload = sdp_message_m_payload_get (sdp, pos, k);
		    if (payload == NULL)
		      {
		      }
		    else if (0==strcmp("110",payload))
		      {
			sdp_message_a_attribute_add (sdp,
						     pos,
						     osip_strdup ("AS"),
						     osip_strdup ("110 20"));
		      }
		    else if (0==strcmp("111",payload))
		      {
			sdp_message_a_attribute_add (sdp,
						     pos,
						     osip_strdup ("AS"),
						     osip_strdup ("111 20"));
		      }
		    k++;
		  } while (payload != NULL);
		}
	      pos++;
	    }
	}

      i = sdp_message_to_str(sdp, &body);
      if (body!=NULL)
	{
	  size= (char *)osip_malloc(7*sizeof(char));
	  sprintf(size,"%i",strlen(body));
	  osip_message_set_content_length(invite, size);
	  osip_free(size);
	  
	  osip_message_set_body(invite, body);
	  osip_free(body);
	  osip_message_set_content_type(invite, "application/sdp");
	}
      else
	osip_message_set_content_length(invite, "0");
    }

  eXosip_call_init(&jc);
  i = osip_message_get_subject(invite, 0, &subject);
  snprintf(jc->c_subject, 99, "%s", subject->hvalue);
  
  if (sdp_context_reference==NULL)
    osip_negotiation_ctx_set_mycontext(jc->c_ctx, jc);
  else
    osip_negotiation_ctx_set_mycontext(jc->c_ctx, sdp_context_reference);

  if (local_sdp_port!=NULL)
    {
      osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);  
      jc->c_ack_sdp = 0;
    }
  else
    jc->c_ack_sdp = 1;

  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      eXosip_call_free(jc);
      osip_message_free(invite);
      return -1;
    }
  
  jc->c_out_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, NULL, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);

  jc->external_reference = reference;
  ADD_ELEMENT(eXosip.j_calls, jc);

  eXosip_update(); /* fixed? */
  return 0;
}

int eXosip_answer_call   (int jid, int status, char *local_sdp_port)
{
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }
  if (status>100 && status<200)
    {
      i = eXosip_answer_invite_1xx(jc, jd, status);
    }
  else if (status>199 && status<300)
    {
#if 0 /* this seems to be useless?? */
      if (jc->c_ctx!=NULL)
	osip_negotiation_ctx_set_mycontext(jc->c_ctx, jc);
	  else
    osip_negotiation_ctx_set_mycontext(jc->c_ctx, sdp_context_reference);
#endif

      i = eXosip_answer_invite_2xx(jc, jd, status, local_sdp_port);
    }
  else if (status>300 && status<699)
    {
      i = eXosip_answer_invite_3456xx(jc, jd, status);
    }
  else
    {
      fprintf(stderr, "eXosip: wrong status code (101<status<699)\n");
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int eXosip_options_call  (int jid)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *options;
  int i;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }

  transaction = eXosip_find_last_options(jc, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
    }

  i = _eXosip_build_request_within_dialog(&options, "OPTIONS", jd->d_dialog, "UDP");
  if (i!=0)
    return -2;
  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       options);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(options);
      return -2;
    }
  
  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(options);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  return 0;
}

int eXosip_answer_options   (int cid, int jid, int status)
{
  int i = -1;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
      if (jd==NULL)
	{
	  fprintf(stderr, "eXosip: No dialog here?\n");
	  return -1;
	}
    }
  else 
    {
      eXosip_call_find(cid, &jc);
      if (jc==NULL)
	{
	  fprintf(stderr, "eXosip: No call here?\n");
	  return -1;
	}
    }
  if (status>100 && status<200)
    {
      i = eXosip_answer_options_1xx(jc, jd, status);
    }
  else if (status>199 && status<300)
    {
      i = eXosip_answer_options_2xx(jc, jd, status);
    }
  else if (status>300 && status<699)
    {
      i = eXosip_answer_options_3456xx(jc, jd, status);
    }
  else
    {
      fprintf(stderr, "eXosip: wrong status code (101<status<699)\n");
      return -1;
    }
  if (i!=0)
    return -1;
  return 0;
}

int eXosip_on_hold_call  (int jid)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *invite;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }

  transaction = eXosip_find_last_invite(jc, jd);
  if (transaction==NULL) return -1;
  if (transaction->state!=ICT_TERMINATED &&
      transaction->state!=IST_TERMINATED)
    return -1;

  sdp = eXosip_get_local_sdp_info(transaction);
  if (sdp==NULL)
    return -1;
  i = osip_negotiation_sdp_message_put_on_hold(sdp);
  if (i!=0)
    {
      sdp_message_free(sdp);
      return -2;
    }

  i = _eXosip_build_request_within_dialog(&invite, "INVITE", jd->d_dialog, "UDP");
  if (i!=0) {
    sdp_message_free(sdp);
    return -2;
  }

  i = sdp_message_to_str(sdp, &body);
  if (body!=NULL)
    {
      size= (char *)osip_malloc(7*sizeof(char));
      sprintf(size,"%i",strlen(body));
      osip_message_set_content_length(invite, size);
      osip_free(size);
      
      osip_message_set_body(invite, body);
      osip_free(body);
      osip_message_set_content_type(invite, "application/sdp");
    }
  else
    osip_message_set_content_length(invite, "0");

  if (jc->c_subject!=NULL)
    osip_message_set_subject(invite, jc->c_subject);
  else
    osip_message_set_subject(invite, "New Call");

  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(invite);
      return -2;
    }
  
  {
    sdp_message_t *old_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
    sdp_message_free(old_sdp);
    osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);  
  }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  return 0;
}

int eXosip_off_hold_call (int jid)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *invite;
  int i;
  sdp_message_t *sdp;
  char *body;
  char *size;

  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }

  transaction = eXosip_find_last_invite(jc, jd);
  if (transaction==NULL) return -1;
  if (transaction->state!=ICT_TERMINATED &&
      transaction->state!=IST_TERMINATED)
    return -1;

  sdp = eXosip_get_local_sdp_info(transaction);
  if (sdp==NULL)
    return -1;
  i = osip_negotiation_sdp_message_put_off_hold(sdp);
  if (i!=0)
    {
      sdp_message_free(sdp);
      return -2;
    }

  i = _eXosip_build_request_within_dialog(&invite, "INVITE", jd->d_dialog, "UDP");
  if (i!=0) {
    sdp_message_free(sdp);
    return -2;
  }

  i = sdp_message_to_str(sdp, &body);
  if (body!=NULL)
    {
      size= (char *)osip_malloc(7*sizeof(char));
      sprintf(size,"%i",strlen(body));
      osip_message_set_content_length(invite, size);
      osip_free(size);
      
      osip_message_set_body(invite, body);
      osip_free(body);
      osip_message_set_content_type(invite, "application/sdp");
    }
  else
    osip_message_set_content_length(invite, "0");

  if (jc->c_subject!=NULL)
    osip_message_set_subject(invite, jc->c_subject);
  else
    osip_message_set_subject(invite, "New Call");

  i = osip_transaction_init(&transaction,
		       ICT,
		       eXosip.j_osip,
		       invite);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(invite);
      return -2;
    }
  
  {
    sdp_message_t *old_sdp = osip_negotiation_ctx_get_local_sdp(jc->c_ctx);
    sdp_message_free(old_sdp);
    osip_negotiation_ctx_set_local_sdp(jc->c_ctx, sdp);  
  }

  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(invite);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(transaction, sipevent);
  return 0;
}

int eXosip_create_transaction(eXosip_call_t *jc,
			     eXosip_dialog_t *jd,
			     osip_message_t *request)
{
  osip_event_t *sipevent;
  osip_transaction_t *tr;
  int i;
  i = osip_transaction_init(&tr,
		       NICT,
		       eXosip.j_osip,
		       request);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(request);
      return -1;
    }
  
  if (jd!=NULL)
    osip_list_add(jd->d_out_trs, tr, 0);
  
  sipevent = osip_new_outgoing_sipmessage(request);
  sipevent->transactionid =  tr->transactionid;
  
  osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd, NULL, NULL));
  osip_transaction_add_event(tr, sipevent);
  return 0;
}

int eXosip_transfer_call(int jid, char *refer_to)
{
  int i;
  osip_message_t *request;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid<=0)
    return -1;

  eXosip_call_dialog_find(jid, &jc, &jd);
  if (jd==NULL || jd->d_dialog==NULL || jd->d_dialog->state==DIALOG_EARLY)
    {
      fprintf(stderr, "eXosip: No established call here!");
      return -1;
    }

  i = generating_refer(&request, jd->d_dialog, refer_to);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot generate REFER for call!");
      return -2;
    }

  i = eXosip_create_transaction(jc, jd, request);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot initiate SIP transfer transaction!");
      return i;
    }
  return 0;
}

int eXosip_create_cancel_transaction(eXosip_call_t *jc,
				    eXosip_dialog_t *jd, osip_message_t *request)
{
  osip_event_t *sipevent;
  osip_transaction_t *tr;
  int i;
  i = osip_transaction_init(&tr,
		       NICT,
		       eXosip.j_osip,
		       request);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(request);
      return -2;
    }
  
  osip_list_add(eXosip.j_transactions, tr, 0);
  
  sipevent = osip_new_outgoing_sipmessage(request);
  sipevent->transactionid =  tr->transactionid;
  
  osip_transaction_add_event(tr, sipevent);
  return 0;
}

int eXosip_terminate_call(int cid, int jid)
{
  int i;
  osip_message_t *request;
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  if (jid>0)
    {
      eXosip_call_dialog_find(jid, &jc, &jd);
      if (jd==NULL)
	{
	  fprintf(stderr, "eXosip: No call here? ");
	  return -1;
	}
    }
  else
    {
      eXosip_call_find(cid, &jc);      
    }

  if (jc==NULL)
    {
      return -1;
    }
  if (jd==NULL || jd->d_dialog==NULL)
    {
      fprintf(stderr, "eXosip: No established dialog!");
      return -1;
    }
  if (jd!=NULL && jd->d_dialog!=NULL)
    {
      osip_transaction_t *tr;
      tr=eXosip_find_last_out_invite(jc, jd);
      if (tr!=NULL && tr->last_response!=NULL && MSG_IS_STATUS_1XX(tr->last_response))
	{
	  i = generating_cancel(&request, tr->orig_request);
	  if (i!=0)
	    {
	      fprintf(stderr, "eXosip: cannot terminate this call! ");
	      return -2;
	    }
	  i = eXosip_create_cancel_transaction(jc, jd, request);
	  if (i!=0)
	    {
	      fprintf(stderr, "eXosip: cannot initiate SIP transaction! ");
	      return i;
	    }
	  return 0;
	}
      if (tr==NULL)
	{
	  /*this may not be enough if it's a re-INVITE! */
	  tr = eXosip_find_last_inc_invite(jc, jd);
	  if (tr!=NULL && tr->last_response!=NULL &&
	      MSG_IS_STATUS_1XX(tr->last_response))
	    { /* answer with 603 */
	      i = eXosip_answer_call(jid, 603, 0);
	      return i;
	    }
	}
    }
  i = generating_bye(&request, jd->d_dialog);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot terminate this call! ");
      return -2;
    }

  i = eXosip_create_transaction(jc, jd, request);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot initiate SIP transaction! ");
      return -2;
    }

  osip_dialog_free(jd->d_dialog);
  jd->d_dialog = NULL;
  return 0;
}

int eXosip_register      (int rid, int registration_period)
{
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  osip_message_t *reg;
  int i;

  eXosip_reg_t *jr;

  jr = eXosip.j_reg;
  if (jr==NULL)
    {
      /* fprintf(stderr, "eXosip: no registration info saved!\n"); */
      return -1;
    }
  jr->r_reg_period = registration_period;
  if (jr->r_reg_period>3600)
    jr->r_reg_period = 3600;
  else if (jr->r_reg_period<200) /* too low */
    jr->r_reg_period = 200;

  reg = NULL;
  if (jr->r_last_tr!=NULL)
    {
      if (jr->r_last_tr->state!=NICT_TERMINATED)
	{
	  /* fprintf(stderr, "eXosip: a registration is already pending!\n"); */
	  return -1;
	}
      else
	{
	  reg = jr->r_last_tr->orig_request;
	  jr->r_last_tr->orig_request = NULL;
	  __eXosip_delete_jinfo(jr->r_last_tr);
	  osip_transaction_free(jr->r_last_tr);
	  jr->r_last_tr = NULL;

	  /* modify the REGISTER request */
	  {
	    int osip_cseq_num = osip_atoi(reg->cseq->number);
	    int length   = strlen(reg->cseq->number);
	    char *tmp    = (char *)osip_malloc(90*sizeof(char));
	    osip_via_t *via   = (osip_via_t *) osip_list_get (reg->vias, 0);
	    osip_list_remove(reg->vias, 0);
	    osip_via_free(via);
	    sprintf(tmp, "SIP/2.0/UDP %s:%s;branch=z9hG4bK%u",
		    localip,
		    localport,
		    via_branch_new_random());
	    osip_via_init(&via);
	    osip_via_parse(via, tmp);
	    osip_list_add(reg->vias, via, 0);
	    osip_free(tmp);

	    osip_cseq_num++;
	    osip_free(reg->cseq->number);
	    reg->cseq->number = (char*)osip_malloc(length+2); /* +2 like for 9 to 10 */
	    sprintf(reg->cseq->number, "%i", osip_cseq_num);

	    {
	      osip_header_t *exp;
	      osip_message_header_get_byname(reg, "expires", 0, &exp);
	      osip_free(exp->hvalue);
	      exp->hvalue = (char*)malloc(10);
	      snprintf(exp->hvalue, 9, "%i", jr->r_reg_period);
	    }

	    osip_message_force_update(reg);
	  }
	}
    }
  if (reg==NULL)
    {
      i = generating_register(&reg, jr->r_aor, jr->r_registrar, jr->r_contact, jr->r_reg_period);
      if (i!=0) 
	{
	  /* fprintf(stderr, "eXosip: cannot register (cannot build REGISTER)! "); */
	  return -2;
	}
    }

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       reg);
  if (i!=0)
    {
      /* TODO: release the j_call.. */

      osip_message_free(reg);
      return -2;
    }

  jr->r_last_tr = transaction;

  /* send REGISTER */
  sipevent = osip_new_outgoing_sipmessage(reg);
  sipevent->transactionid =  transaction->transactionid;
  osip_message_force_update(reg);
  
  osip_transaction_add_event(transaction, sipevent);
  return 0;
}


int
eXosip_register_init(char *from, char *proxy, char *contact)
{
  eXosip_reg_t *jr;
  int i;

  i = eXosip_reg_init(&jr, from, proxy, contact);
  if (i!=0) 
    {
      fprintf(stderr, "eXosip: cannot register! ");
      return i;
    }
  eXosip.j_reg = jr;
  return 0;
}


int eXosip_subscribe    (char *to, char *from, char *route)
{
  eXosip_subscribe_t *js;
  osip_message_t *subscribe;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;
    
  i = generating_initial_subscribe(&subscribe, to, from, route);
  if (i!=0) 
    {
      fprintf(stderr, "eXosip: cannot subscribe (cannot build SUBSCRIBE)! ");
      return -1;
    }

  i = eXosip_subscribe_init(&js, to);
  if (i!=0)
    {
      fprintf(stderr, "eXosip: cannot subscribe.");
      return -1;
    }
  
  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       subscribe);
  if (i!=0)
    {
      osip_message_free(subscribe);
      return -1;
    }

  _eXosip_subscribe_set_refresh_interval(js, subscribe);
  js->s_out_tr = transaction;
  
  sipevent = osip_new_outgoing_sipmessage(subscribe);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, NULL, js, NULL));
  osip_transaction_add_event(transaction, sipevent);

  ADD_ELEMENT(eXosip.j_subscribes, js);
  return 0;
}


int eXosip_subscribe_refresh  (int sid, char *expires)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  if (sid>0)
    {
      eXosip_subscribe_dialog_find(sid, &js, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No subscribe dialog here?\n");
      return -1;
    }

#define LOW_EXPIRE
#ifdef LOW_EXPIRE
  if (expires==NULL)
    i = eXosip_subscribe_send_subscribe(js, jd, "60");
  else
    i = eXosip_subscribe_send_subscribe(js, jd, expires);
#else
  if (expires==NULL)
    i = eXosip_subscribe_send_subscribe(js, jd, "600");
  else
    i = eXosip_subscribe_send_subscribe(js, jd, expires);
#endif
  return i;
}

int eXosip_subscribe_close(int sid)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  if (sid>0)
    {
      eXosip_subscribe_dialog_find(sid, &js, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No subscribe dialog here?\n");
      return -1;
    }

  i = eXosip_subscribe_send_subscribe(js, jd, "0");
  return i;
}

int eXosip_notify_send_notify(eXosip_notify_t *jn,
			      eXosip_dialog_t *jd,
			      int subscription_status,
			      int online_status)
{
  osip_transaction_t *transaction;
  osip_message_t *notify;
  osip_event_t *sipevent;
  int   i;
  char  subscription_state[50];
  char *tmp;
  int   now = time(NULL);
  transaction = eXosip_find_last_out_notify(jn, jd);
  if (transaction!=NULL)
    {
      if (transaction->state!=NICT_TERMINATED &&
	  transaction->state!=NIST_TERMINATED)
	return -1;
    }

#ifndef SUPPORT_MSN

#else

  /* DO NOT SEND ANY NOTIFY when the status
     is not active (or terminated?) */
  if (subscription_status!=EXOSIP_SUBCRSTATE_ACTIVE
      && subscription_status!=EXOSIP_SUBCRSTATE_TERMINATED)
    {
      /* set the new state anyway! */
      jn->n_online_status = online_status;
      jn->n_ss_status = subscription_status;
      return -1;
    }

#endif

  i = _eXosip_build_request_within_dialog(&notify, "NOTIFY", jd->d_dialog, "UDP");
  if (i!=0)
    return -2;

  jn->n_online_status = online_status;
  jn->n_ss_status = subscription_status;

  /* add the notifications info */
  if (jn->n_ss_status==EXOSIP_SUBCRSTATE_UNKNOWN)
    jn->n_online_status=EXOSIP_SUBCRSTATE_PENDING;

#ifndef SUPPORT_MSN
  if (jn->n_ss_status==EXOSIP_SUBCRSTATE_PENDING)
    osip_strncpy(subscription_state, "pending;expires=", 16);
  else if (jn->n_ss_status==EXOSIP_SUBCRSTATE_ACTIVE)
    osip_strncpy(subscription_state, "active;expires=", 15);
  else if (jn->n_ss_status==EXOSIP_SUBCRSTATE_TERMINATED)
    {
      if (jn->n_ss_reason==DEACTIVATED)
	osip_strncpy(subscription_state, "terminated;reason=deactivated", 29);
      else if (jn->n_ss_reason==PROBATION)
	osip_strncpy(subscription_state, "terminated;reason=probation", 27);
      else if (jn->n_ss_reason==REJECTED)
	osip_strncpy(subscription_state, "terminated;reason=rejected", 26);
      else if (jn->n_ss_reason==TIMEOUT)
	osip_strncpy(subscription_state, "terminated;reason=timeout", 25);
      else if (jn->n_ss_reason==GIVEUP)
	osip_strncpy(subscription_state, "terminated;reason=giveup", 24);
      else if (jn->n_ss_reason==NORESSOURCE)
	osip_strncpy(subscription_state, "terminated;reason=noressource", 29);
    }
  tmp = subscription_state + strlen(subscription_state);
  if (jn->n_ss_status!=EXOSIP_SUBCRSTATE_TERMINATED)
    sprintf(tmp, "%i", jn->n_ss_expires-now);
  osip_message_set_header(notify, "Subscription-State",
			 subscription_state);
#endif

  /* add a body */
  i = _eXosip_notify_add_body(jn, notify);
  if (i!=0)
    {

    }

  i = osip_transaction_init(&transaction,
		       NICT,
		       eXosip.j_osip,
		       notify);
  if (i!=0)
    {
      /* TODO: release the j_call.. */
      osip_message_free(notify);
      return -1;
    }
  
  osip_list_add(jd->d_out_trs, transaction, 0);
  
  sipevent = osip_new_outgoing_sipmessage(notify);
  sipevent->transactionid =  transaction->transactionid;
  
  osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(NULL, jd, NULL, jn));
  osip_transaction_add_event(transaction, sipevent);
  return 0;
}

int eXosip_notify  (int nid, int subscription_status, int online_status)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_notify_t *jn = NULL;

  if (nid>0)
    {
      eXosip_notify_dialog_find(nid, &jn, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No subscribe dialog here?\n");
      return -1;
    }

  i = eXosip_notify_send_notify(jn, jd, subscription_status, online_status);
  return i;
}


int eXosip_notify_accept_subscribe(int nid, int code,
				   int subscription_status,
				   int online_status)
{
  int i;
  eXosip_dialog_t *jd = NULL;
  eXosip_notify_t *jn = NULL;
  if (nid>0)
    {
      eXosip_notify_dialog_find(nid, &jn, &jd);
    }
  if (jd==NULL)
    {
      fprintf(stderr, "eXosip: No call here?\n");
      return -1;
    }
  if (code>100 && code<200)
    {
      eXosip_notify_answer_subscribe_1xx(jn, jd, code);
    }
  else if (code>199 && code<300)
    {
      eXosip_notify_answer_subscribe_2xx(jn, jd, code);
    }
  else if (code>300 && code<699)
    {
      eXosip_notify_answer_subscribe_3456xx(jn, jd, code);
    }
  else
    {
      fprintf(stderr, "eXosip: wrong status code (101<code<699)\n");
      return -1;
    }

  i = eXosip_notify(nid, subscription_status, online_status);
  return i;
}
