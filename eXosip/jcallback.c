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

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include <eXosip.h>


#ifdef WIN32
#include <winsock.h>
#else 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern eXosip_t eXosip;

#ifdef TEST_AUDIO
static pid_t pid = 0;
#endif

int cb_udp_snd_message(osip_transaction_t *tr, osip_message_t *sip, char *host,
		       int port, int out_socket)
{
  static int num = 0;
  struct sockaddr_in addr;
  unsigned long int  one_inet_addr;
  char *message;
  int i;

  if (eXosip.j_socket==0) return -1;

  if (host==NULL)
    {
      host = sip->rquri->host;
      if (sip->rquri->port!=NULL)
	port = osip_atoi(sip->rquri->port);
      else
	port = 5060;
    }

  if ((int)(one_inet_addr = inet_addr(host)) == -1)
    {
      /* TODO: have to resolv, but it should not be done here! */
      return -1;
    }
  else
    { 
      addr.sin_addr.s_addr = one_inet_addr;
    }

  addr.sin_port        = htons((short)port);
  addr.sin_family      = AF_INET;


  i = msg_to_str(sip, &message);

  if (i!=0) {
    return -1;
  }

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			"Message sent: \n%s\n", message));
  if (0  > sendto (eXosip.j_socket, (const void*) message, strlen (message), 0,
		   (struct sockaddr *) &addr,sizeof(addr))) 
    {
#ifdef WIN32
      if (WSAECONNREFUSED==WSAGetLastError())
#else
	if (ECONNREFUSED==errno)
#endif
	  {
	    /* This can be considered as an error, but for the moment,
	       I prefer that the application continue to try sending
	       message again and again... so we are not in a error case.
	       Nevertheless, this error should be announced!
	       ALSO, UAS may not have any other options than retry always
	       on the same port.
	    */
	    osip_free(message);
	    return 1;
	  }
	else
	  {
	    /* SIP_NETWORK_ERROR; */
	    osip_free(message);
	    return -1;
	  }
    }
  if (strncmp(message, "INVITE", 7)==0)
    {
      num++;
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,"number of message sent: %i\n", num));
    }

  osip_free(message);
  return 0;
  
}

void cb_ict_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ict_kill_transaction (id=%i)\r\n", tr->transactionid));

  i = osip_remove_ict(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_ist_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_ist(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_nict_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nict_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_nict(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_nist_kill_transaction(int type, osip_transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_nist(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}
  
void cb_rcvinvite  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinvite (id=%i)\n", tr->transactionid));
}

void cb_rcvack     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack (id=%i)\n", tr->transactionid));
}

void cb_rcvack2    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack2 (id=%i)\r\n", tr->transactionid));
}
  
void cb_rcvregister(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvbye (id=%i)\r\n", tr->transactionid));
}

void cb_rcvbye     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvbye (id=%i)\r\n", tr->transactionid));
#ifdef TEST_AUDIO
  if (pid!=0)
    {
      kill(pid, SIGINT);
      pid = 0;
    }
#endif
}

void cb_rcvcancel  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvcancel (id=%i)\r\n", tr->transactionid));
}

void cb_rcvinfo    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinfo (id=%i)\r\n", tr->transactionid));
}

void cb_rcvoptions (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvoptions (id=%i)\r\n", tr->transactionid));
}

void cb_rcvnotify  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvnotify (id=%i)\r\n", tr->transactionid));
}

void cb_rcvsubscribe (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvsubscribe (id=%i)\r\n", tr->transactionid));
}

void cb_rcvunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvunkrequest (id=%i)\r\n", tr->transactionid));
}

void cb_sndinvite  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinvite (id=%i)\r\n", tr->transactionid));
}

void cb_sndack     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndack (id=%i)\r\n", tr->transactionid));
}
  
void cb_sndregister(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndregister (id=%i)\r\n", tr->transactionid));
}

void cb_sndbye     (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndbye (id=%i)\r\n", tr->transactionid));
#ifdef TEST_AUDIO
  if (pid!=0)
    {
      kill(pid, SIGINT);
      pid = 0;
    }
#endif

}

void cb_sndcancel  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndcancel (id=%i)\r\n", tr->transactionid));
}

void cb_sndinfo    (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinfo (id=%i)\r\n", tr->transactionid));
}

void cb_sndoptions (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndoptions (id=%i)\r\n", tr->transactionid));
}

void cb_sndnotify  (int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndnotify (id=%i)\r\n", tr->transactionid));
}

void cb_sndsubscribe(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndsubscibe (id=%i)\r\n", tr->transactionid));
}

void cb_sndunkrequest(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndunkrequest (id=%i)\r\n", tr->transactionid));
}

void __eXosip_delete_jinfo(osip_transaction_t *transaction)
{
  jinfo_t *ji = osip_transaction_get_your_instance(transaction);
  osip_free(ji);
}

jinfo_t *__eXosip_new_jinfo(eXosip_call_t *jc, eXosip_dialog_t *jd)
{
  jinfo_t *ji = (jinfo_t *) osip_malloc(sizeof(jinfo_t));
  if (ji==NULL) return NULL;
  ji->jd = jd;
  ji->jc = jc;
  return ji;
}

void cb_rcv1xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv1xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE") && !MSG_TEST_CODE(sip, 100))
    {
      int i;
      if (jd == NULL) /* This transaction initiate a dialog in the case of
			 INVITE (else it would be attached to a "jd" element. */
	{
	  /* allocate a jd */
	  i = eXosip_dialog_init_as_uac(&jd, sip);
	  if (i!=0)
	    {
	      fprintf(stderr, "eXosip: cannot establish a dialog\n");
	      return;
	    }

	  ADD_ELEMENT(jc->c_dialogs, jd);
	  jinfo->jd = jd;
	  osip_transaction_set_your_instance(tr, jinfo);
	}
      else
	{
	  osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
	}
      if ( jd!=NULL)
	jd->d_STATE = JD_TRYING;

      if (MSG_TEST_CODE(sip, 180) && jd!=NULL)
	{
	  jd->d_STATE = JD_RINGING;
	}
      else if (MSG_TEST_CODE(sip, 183) && jd!=NULL)
	{
	  jd->d_STATE = JD_QUEUED;
	}

    }
}


sdp_message_t *eXosip_get_remote_sdp(osip_transaction_t *transaction)
{
  osip_message_t *message;
  osip_body_t *body;
  sdp_message_t *sdp;
  int pos = 0;
  int i;
  if (transaction->ist_context!=NULL)
    /* remote sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* remote sdp is in response */
    message = transaction->last_response;

  sdp=NULL;
  body = (osip_body_t *)osip_list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_message_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_message_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
      pos++;
      body = (osip_body_t *)osip_list_get(message->bodies,pos);
    }
  return NULL;
}

sdp_message_t *eXosip_get_local_sdp(osip_transaction_t *transaction)
{
  osip_message_t *message;
  osip_body_t *body;
  sdp_message_t *sdp;
  int i;
  int pos = 0;
  if (transaction->ict_context!=NULL)
    /* local sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* local sdp is in response */
    message = transaction->last_response;

  sdp=NULL;
  body = (osip_body_t *)osip_list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_message_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_message_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_message_free(sdp);
      sdp = NULL;
      pos++;
      body = (osip_body_t *)osip_list_get(message->bodies,pos);
    }
  return NULL;
}


void eXosip_update_audio_session(osip_transaction_t *transaction)
{
  char *remaddr;
  sdp_message_t *remote_sdp;
  sdp_message_t *local_sdp;
  char *remote_port;
  char *local_port;
  char *payload;
  char *media_type;
  int pos;
  /* look for the SDP informations */
  
  remote_sdp = eXosip_get_remote_sdp(transaction);
  if (remote_sdp==NULL)
    return ;
  local_sdp = eXosip_get_local_sdp(transaction);
  if (local_sdp==NULL)
    {
      sdp_message_free(remote_sdp);
      return ;
    }
  remaddr=sdp_message_c_addr_get(remote_sdp,-1,0);
  if (remaddr==NULL){
    remaddr=sdp_message_c_addr_get(remote_sdp,0,0);
  }

  pos=0;
  local_port=sdp_message_m_port_get(local_sdp,pos);
  media_type = sdp_message_m_media_get(local_sdp,pos);
  while (local_port!=NULL && media_type!=NULL)
    { /* If we have refused some media lines, the port is set to 0 */
      if (0!=strncmp(local_port,"0", 1)&&0==strcmp(media_type,"audio"))
	break;
      pos++;
      media_type = sdp_message_m_media_get(local_sdp,pos);
      local_port=sdp_message_m_port_get(local_sdp,pos);
    }

  if (media_type!=NULL && local_port!=NULL)
    {
      remote_port = sdp_message_m_port_get(remote_sdp,pos);
      payload = sdp_message_m_payload_get(local_sdp,pos,0);
    }
  else
    {
      remote_port = NULL;
      payload = NULL;
    }
  if (remote_port!=NULL && media_type!=NULL) /* if codec has been found */
    {
      char tmp[256];
      sprintf(tmp, "mediastream --local %s --remote %s:%s --payload %s" ,
	      local_port, remaddr, remote_port, payload);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command %s\n", tmp));

#ifdef TEST_AUDIO
      if (pid!=0)
	{
	  kill(pid, SIGINT);
	  pid = 0;
	}

      pid = fork();
      if (pid==0)
	{
	  int ret = system(tmp);
	  
	  if (WIFSIGNALED(ret) &&
	      (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
	    {
	      exit(-1);
	    }
	  if (ret==0)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not start audio\n", tmp));
	    }
	  exit(0);
	}
#endif

    }
  else
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not create audio session.\r\n"));
    }
  sdp_message_free(local_sdp);
  sdp_message_free(remote_sdp);
}

void cb_rcv2xx_4invite(osip_transaction_t *tr,osip_message_t *sip)
{
  int i;
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd == NULL) /* This transaction initiate a dialog in the case of
		     INVITE (else it would be attached to a "jd" element. */
    {
      /* allocate a jd */
      i = eXosip_dialog_init_as_uac(&jd, sip);
      if (i!=0)
	{
	  fprintf(stderr, "eXosip: cannot establish a dialog\n");
	  return;
	}
      ADD_ELEMENT(jc->c_dialogs, jd);
      jinfo->jd = jd;
      osip_transaction_set_your_instance(tr, jinfo);
    }
  else
    {
      osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
      osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
    }


  jd->d_STATE = JD_ESTABLISHED;

  eXosip_dialog_set_200ok(jd, sip);

  {
    osip_route_t *route;
    char *host;
    int port;
    osip_message_t *ack;
    i = _eXosip_build_request_within_dialog(&ack, "ACK", jd->d_dialog, "UDP");
    if (i!=0) {
      jd->d_STATE = JD_ESTABLISHED;
      return ;
    }

    osip_parser_get_route(ack, 0, &route);
    if (route!=NULL)
      {
	port = 5060;
	if (route->url->port!=NULL)
	  port = osip_atoi(route->url->port);
	host = route->url->host;
      }
    else
      {
	port = 5060;
	if (ack->rquri->port!=NULL)
	  port = osip_atoi(ack->rquri->port);
	host = ack->rquri->host;
      }

    cb_udp_snd_message(NULL, ack, host, port, eXosip.j_socket);

    jd->d_ack  = ack;

  }


  /* look for the SDP information and decide if this answer was for
     an initial INVITE, an HoldCall, or a RetreiveCall */

  /* don't handle hold/unhold by now... */
  eXosip_update_audio_session(tr);

}

void cb_rcv2xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv2xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      cb_rcv2xx_4invite(tr, sip);
    }
  else if (MSG_IS_RESPONSEFOR(sip, "BYE"))
    {
      if (jd!=NULL)
	jd->d_STATE = JD_TERMINATED;
    }
}

void eXosip_delete_early_dialog(eXosip_dialog_t *jd)
{
  if (jd == NULL) /* bug? */
      return;

  /* an early dialog was created, but the call is not established */
  if (jd->d_dialog!=NULL && jd->d_dialog->state==DIALOG_EARLY)
    {
      osip_dialog_free(jd->d_dialog);
      jd->d_dialog = NULL;
      eXosip_dialog_set_state(jd, JD_TERMINATED);
    }    
}

void cb_rcv3xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv3xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL) return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
      if (jd->d_dialog==NULL)
	jd->d_STATE = JD_REDIRECTED;
    }
}

void cb_rcv4xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv4xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
      if (MSG_TEST_CODE(sip, 401) || MSG_TEST_CODE(sip, 407))
	jd->d_STATE = JD_AUTH_REQUIRED;
      else
	jd->d_STATE = JD_CLIENTERROR;
    }
}

void cb_rcv5xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv5xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
      jd->d_STATE = JD_SERVERERROR;
    }

}

void cb_rcv6xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcv6xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
      jd->d_STATE = JD_GLOBALFAILURE;
    }

}

void cb_snd1xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd1xx (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  jd->d_STATE = JD_TRYING;
}

void cb_snd2xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd2xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      jd->d_STATE = JD_ESTABLISHED;
      return;
    }
  jd->d_STATE = JD_ESTABLISHED;
}

void cb_snd3xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd3xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_REDIRECTED;
}

void cb_snd4xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd4xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_CLIENTERROR;
}

void cb_snd5xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd5xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_SERVERERROR;
}

void cb_snd6xx(int type, osip_transaction_t *tr,osip_message_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)osip_transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd6xx (id=%i)\r\n", tr->transactionid));
  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  if (MSG_IS_RESPONSEFOR(sip, "INVITE"))
    {
      eXosip_delete_early_dialog(jd);
    }
  jd->d_STATE = JD_GLOBALFAILURE;
}

void cb_rcvresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvresp_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_sndreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndreq_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_sndresp_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndresp_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_rcvreq_retransmission(int type, osip_transaction_t *tr, osip_message_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvreq_retransmission (id=%i)\r\n", tr->transactionid));
}
  
void cb_killtransaction(int type, osip_transaction_t *tr)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_killtransaction (id=%i)\r\n", tr->transactionid));
}

void cb_endoftransaction(int type, osip_transaction_t *tr)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_endoftransaction (id=%i)\r\n", tr->transactionid));
}
  
void cb_transport_error(int type, osip_transaction_t *tr, int error)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_transport_error (id=%i)\r\n", tr->transactionid));
}



int
eXosip_set_callbacks(osip_t *osip)
{
  // register all callbacks

  osip_set_cb_send_message(osip, &cb_udp_snd_message);
  
  osip_set_kill_transaction_callback(osip ,OSIP_ICT_KILL_TRANSACTION,
				 &cb_ict_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_NIST_KILL_TRANSACTION,
				 &cb_ist_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_NICT_KILL_TRANSACTION,
				 &cb_nict_kill_transaction);
  osip_set_kill_transaction_callback(osip ,OSIP_NIST_KILL_TRANSACTION,
				 &cb_nist_kill_transaction);
          
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_ICT_INVITE_SENT_AGAIN,
			&cb_sndreq_retransmission);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_2XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_3456XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_IST_INVITE_RECEIVED_AGAIN,
			&cb_rcvreq_retransmission);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN,
			&cb_rcvresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_NICT_REQUEST_SENT_AGAIN,
			&cb_sndreq_retransmission);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_2XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_3456XX_SENT_AGAIN,
			&cb_sndresp_retransmission);
  osip_set_msg_callback(osip ,OSIP_NIST_REQUEST_RECEIVED_AGAIN,
			&cb_rcvreq_retransmission);
          
  osip_set_transport_error_callback(osip ,OSIP_ICT_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_IST_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_NICT_TRANSPORT_ERROR,
				    &cb_transport_error);
  osip_set_transport_error_callback(osip ,OSIP_NIST_TRANSPORT_ERROR,
				    &cb_transport_error);
  
  osip_set_msg_callback(osip ,OSIP_ICT_INVITE_SENT,     &cb_sndinvite);
  osip_set_msg_callback(osip ,OSIP_ICT_ACK_SENT,        &cb_sndack);
  osip_set_msg_callback(osip ,OSIP_NICT_REGISTER_SENT,  &cb_sndregister);
  osip_set_msg_callback(osip ,OSIP_NICT_BYE_SENT,       &cb_sndbye);
  osip_set_msg_callback(osip ,OSIP_NICT_CANCEL_SENT,    &cb_sndcancel);
  osip_set_msg_callback(osip ,OSIP_NICT_INFO_SENT,      &cb_sndinfo);
  osip_set_msg_callback(osip ,OSIP_NICT_OPTIONS_SENT,   &cb_sndoptions);
  osip_set_msg_callback(osip ,OSIP_NICT_SUBSCRIBE_SENT, &cb_sndsubscribe);
  osip_set_msg_callback(osip ,OSIP_NICT_NOTIFY_SENT,    &cb_sndnotify);
  /*  osip_set_cb_nict_sndprack   (osip,&cb_sndprack); */
  osip_set_msg_callback(osip ,OSIP_NICT_UNKNOWN_REQUEST_SENT, &cb_sndunkrequest);

  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
  osip_set_msg_callback(osip ,OSIP_ICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);
  
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_1XX_SENT, &cb_snd1xx);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_2XX_SENT, &cb_snd2xx);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_3XX_SENT, &cb_snd3xx);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_4XX_SENT, &cb_snd4xx);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_5XX_SENT, &cb_snd5xx);
  osip_set_msg_callback(osip ,OSIP_IST_STATUS_6XX_SENT, &cb_snd6xx);
  
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
  osip_set_msg_callback(osip ,OSIP_NICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);
      
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_1XX_SENT, &cb_snd1xx);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_2XX_SENT, &cb_snd2xx);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_3XX_SENT, &cb_snd3xx);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_4XX_SENT, &cb_snd4xx);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_5XX_SENT, &cb_snd5xx);
  osip_set_msg_callback(osip ,OSIP_NIST_STATUS_6XX_SENT, &cb_snd6xx);
  
  osip_set_msg_callback(osip ,OSIP_IST_INVITE_RECEIVED,     &cb_rcvinvite);
  osip_set_msg_callback(osip ,OSIP_IST_ACK_RECEIVED,        &cb_rcvack);
  osip_set_msg_callback(osip ,OSIP_IST_ACK_RECEIVED_AGAIN,  &cb_rcvack2);
  osip_set_msg_callback(osip ,OSIP_NIST_REGISTER_RECEIVED,  &cb_rcvregister);
  osip_set_msg_callback(osip ,OSIP_NIST_BYE_RECEIVED,       &cb_rcvbye);
  osip_set_msg_callback(osip ,OSIP_NIST_CANCEL_RECEIVED,    &cb_rcvcancel);
  osip_set_msg_callback(osip ,OSIP_NIST_INFO_RECEIVED,      &cb_rcvinfo);
  osip_set_msg_callback(osip ,OSIP_NIST_OPTIONS_RECEIVED,   &cb_rcvoptions);
  osip_set_msg_callback(osip ,OSIP_NIST_SUBSCRIBE_RECEIVED, &cb_rcvsubscribe);
  osip_set_msg_callback(osip ,OSIP_NIST_NOTIFY_RECEIVED,    &cb_rcvnotify);
  osip_set_msg_callback(osip ,OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, &cb_sndunkrequest);


  return 0;
}
