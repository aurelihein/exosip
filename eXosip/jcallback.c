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

int cb_udp_snd_message(transaction_t *tr, sip_t *sip, char *host,
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
      host = sip->strtline->rquri->host;
      if (sip->strtline->rquri->port!=NULL)
	port = satoi(sip->strtline->rquri->port);
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


  i = msg_2char(sip, &message);

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
	    sfree(message);
	    return 1;
	  }
	else
	  {
	    /* SIP_NETWORK_ERROR; */
	    sfree(message);
	    return -1;
	  }
    }
  if (strncmp(message, "INVITE", 7)==0)
    {
      num++;
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,"number of message sent: %i\n", num));
    }

  sfree(message);
  return 0;
  
}

void cb_ict_kill_transaction(transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ict_kill_transaction (id=%i)\r\n", tr->transactionid));

  i = osip_remove_ict(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_ist_kill_transaction(transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_ist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_ist(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_ist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_nict_kill_transaction(transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nict_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_nict(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}

void cb_nist_kill_transaction(transaction_t *tr)
{
  int i;
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_nist_kill_transaction (id=%i)\r\n", tr->transactionid));
  i = osip_remove_nist(eXosip.j_osip, tr);
  if (i!=0)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,"cb_nist_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n", tr->transactionid));
    }
}
  
void cb_rcvinvite  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinvite (id=%i)\n", tr->transactionid));
}

void cb_rcvack     (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack (id=%i)\n", tr->transactionid));
}

void cb_rcvack2    (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvack2 (id=%i)\r\n", tr->transactionid));
}
  
void cb_rcvregister(transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvbye (id=%i)\r\n", tr->transactionid));
}

void cb_rcvbye     (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvbye (id=%i)\r\n", tr->transactionid));
}

void cb_rcvcancel  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvcancel (id=%i)\r\n", tr->transactionid));
}

void cb_rcvinfo    (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvinfo (id=%i)\r\n", tr->transactionid));
}

void cb_rcvoptions (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvoptions (id=%i)\r\n", tr->transactionid));
}

void cb_rcvnotify  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvnotify (id=%i)\r\n", tr->transactionid));
}

void cb_rcvsubscribe (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvsubscribe (id=%i)\r\n", tr->transactionid));
}

void cb_rcvunkrequest(transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvunkrequest (id=%i)\r\n", tr->transactionid));
}

void cb_sndinvite  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinvite (id=%i)\r\n", tr->transactionid));
}

void cb_sndack     (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndack (id=%i)\r\n", tr->transactionid));
}
  
void cb_sndregister(transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndregister (id=%i)\r\n", tr->transactionid));
}

void cb_sndbye     (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndbye (id=%i)\r\n", tr->transactionid));
}

void cb_sndcancel  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndcancel (id=%i)\r\n", tr->transactionid));
}

void cb_sndinfo    (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndinfo (id=%i)\r\n", tr->transactionid));
}

void cb_sndoptions (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndoptions (id=%i)\r\n", tr->transactionid));
}

void cb_sndnotify  (transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndnotify (id=%i)\r\n", tr->transactionid));
}

void cb_sndsubscribe(transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndsubscibe (id=%i)\r\n", tr->transactionid));
}

void cb_sndunkrequest(transaction_t *tr,sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndunkrequest (id=%i)\r\n", tr->transactionid));
}

jinfo_t *new_jinfo(eXosip_call_t *jc, eXosip_dialog_t *jd)
{
  jinfo_t *ji = (jinfo_t *) smalloc(sizeof(jinfo_t));
  if (ji==NULL) return NULL;
  ji->jd = jd;
  ji->jc = jc;
  return ji;
}

void cb_rcv1xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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
	  transaction_set_your_instance(tr, jinfo);
	}
      else
	{
	  dialog_update_route_set_as_uac(jd->d_dialog, sip);
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


sdp_t *eXosip_get_remote_sdp(transaction_t *transaction)
{
  sip_t *message;
  body_t *body;
  sdp_t *sdp;
  int pos = 0;
  int i;
  if (transaction->ist_context!=NULL)
    /* remote sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* remote sdp is in response */
    message = transaction->last_response;

  sdp=NULL;
  body = (body_t *)list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_free(sdp);
      sfree(sdp);
      sdp = NULL;
      pos++;
      body = (body_t *)list_get(message->bodies,pos);
    }
  return NULL;
}

sdp_t *eXosip_get_local_sdp(transaction_t *transaction)
{
  sip_t *message;
  body_t *body;
  sdp_t *sdp;
  int i;
  int pos = 0;
  if (transaction->ict_context!=NULL)
    /* local sdp is in INVITE (or ACK!) */
    message = transaction->orig_request;
  else
    /* local sdp is in response */
    message = transaction->last_response;

  sdp=NULL;
  body = (body_t *)list_get(message->bodies,0);
  while (body!=NULL)
    {
      i = sdp_init(&sdp);
      if (i!=0)
	{ sdp = NULL; break; }
      i = sdp_parse(sdp,body->body);
      if (i==0)
	return sdp;
      sdp_free(sdp);
      sfree(sdp);
      sdp = NULL;
      pos++;
      body = (body_t *)list_get(message->bodies,pos);
    }
  return NULL;
}

void eXosip_update_audio_session(transaction_t *transaction)
{
  char *remaddr;
  sdp_t *remote_sdp;
  sdp_t *local_sdp;
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
      sdp_free(remote_sdp);
      sfree(remote_sdp);
      return ;
    }
  remaddr=sdp_c_addr_get(remote_sdp,-1,0);
  if (remaddr==NULL){
    remaddr=sdp_c_addr_get(remote_sdp,0,0);
  }

  pos=0;
  local_port=sdp_m_port_get(local_sdp,pos);
  media_type = sdp_m_media_get(local_sdp,pos);
  while ((local_port!=NULL&&media_type!=NULL)&&
	 (0==strncmp(local_port,"0", 1)||0!=strcmp(media_type,"audio")))
    { /* If we have refused some media lines, the port is set to 0 */
      pos++;
      media_type = sdp_m_media_get(local_sdp,pos);
      local_port=sdp_m_port_get(local_sdp,pos);
    }
  remote_port = sdp_m_port_get(remote_sdp,pos);
  payload = sdp_m_payload_get(local_sdp,pos,0);
  if (remote_port!=NULL && media_type!=NULL) /* if codec has been found */
    {
      int i;
      char tmp[256];
      sprintf(tmp, "mediastream --local %s --remote %s:%s --payload %s" ,
	      local_port, remaddr, remote_port, payload);
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"audio command %s\n", tmp));
      i = system(tmp);
      if (i==0)
	{
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not start audio\n", tmp));
	}
    }
  else
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"Could not create audio session.\r\n"));
    }
  sdp_free(local_sdp);
  sfree(local_sdp);
  sdp_free(remote_sdp);
  sfree(remote_sdp);
}

void cb_rcv2xx_4invite(transaction_t *tr,sip_t *sip)
{
  int i;
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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
      transaction_set_your_instance(tr, jinfo);
    }
  else
    {
      dialog_update_route_set_as_uac(jd->d_dialog, sip);
      dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
    }


  jd->d_STATE = JD_ESTABLISHED;

  eXosip_dialog_set_200ok(jd, sip);

  {
    route_t *route;
    char *host;
    int port;
    sip_t *ack;
    i = _eXosip_build_request_within_dialog(&ack, "ACK", jd->d_dialog, "UDP");
    if (i!=0) {
      jd->d_STATE = JD_ESTABLISHED;
      return ;
    }

    msg_getroute(ack, 0, &route);
    if (route!=NULL)
      {
	port = 5060;
	if (route->url->port!=NULL)
	  port = satoi(route->url->port);
	host = route->url->host;
      }
    else
      {
	port = 5060;
	if (ack->strtline->rquri->port!=NULL)
	  port = satoi(ack->strtline->rquri->port);
	host = ack->strtline->rquri->host;
      }

    cb_udp_snd_message(NULL, ack, host, port, eXosip.j_socket);

    jd->d_ack  = ack;

  }


  /* look for the SDP information and decide if this answer was for
     an initial INVITE, an HoldCall, or a RetreiveCall */

  /* don't handle hold/unhold by now... */
  eXosip_update_audio_session(tr);

}

void cb_rcv2xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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
      dialog_free(jd->d_dialog);
      sfree(jd->d_dialog);
      jd->d_dialog = NULL;
      eXosip_dialog_set_state(jd, JD_TERMINATED);
    }    
}

void cb_rcv3xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_rcv4xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_rcv5xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_rcv6xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_snd1xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_snd1xx (id=%i)\r\n", tr->transactionid));

  if (jinfo==NULL)
    return;
  jd = jinfo->jd;
  jc = jinfo->jc;
  if (jd==NULL) return;
  jd->d_STATE = JD_TRYING;
}

void cb_snd2xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_snd3xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_snd4xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_snd5xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_snd6xx(transaction_t *tr,sip_t *sip)
{
  eXosip_dialog_t *jd;
  eXosip_call_t *jc;
  jinfo_t *jinfo =  (jinfo_t *)transaction_get_your_instance(tr);
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

void cb_rcvresp_retransmission(transaction_t *tr, sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvresp_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_sndreq_retransmission(transaction_t *tr, sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndreq_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_sndresp_retransmission(transaction_t *tr, sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_sndresp_retransmission (id=%i)\r\n", tr->transactionid));
}

void cb_rcvreq_retransmission(transaction_t *tr, sip_t *sip)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_rcvreq_retransmission (id=%i)\r\n", tr->transactionid));
}
  
void cb_killtransaction(transaction_t *tr)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_killtransaction (id=%i)\r\n", tr->transactionid));
}

void cb_endoftransaction(transaction_t *tr)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_endoftransaction (id=%i)\r\n", tr->transactionid));
}
  
void cb_transport_error(transaction_t *tr, int error)
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,"cb_transport_error (id=%i)\r\n", tr->transactionid));
}



int
eXosip_set_callbacks(osip_t *osip)
{
  // register all callbacks
  osip_setcb_send_message(osip, &cb_udp_snd_message);
  
  osip_setcb_ict_kill_transaction(osip,&cb_ict_kill_transaction);
  osip_setcb_ist_kill_transaction(osip,&cb_ist_kill_transaction);
  osip_setcb_nict_kill_transaction(osip,&cb_nict_kill_transaction);
  osip_setcb_nist_kill_transaction(osip,&cb_nist_kill_transaction);
          
  osip_setcb_ict_2xx_received2(osip,&cb_rcvresp_retransmission);
  osip_setcb_ict_3456xx_received2(osip,&cb_rcvresp_retransmission);
  osip_setcb_ict_invite_sent2(osip,&cb_sndreq_retransmission);
  osip_setcb_ist_2xx_sent2(osip,&cb_sndresp_retransmission);
  osip_setcb_ist_3456xx_sent2(osip,&cb_sndresp_retransmission);
  osip_setcb_ist_invite_received2(osip,&cb_rcvreq_retransmission);
  osip_setcb_nict_2xx_received2(osip,&cb_rcvresp_retransmission);
  osip_setcb_nict_3456xx_received2(osip,&cb_rcvresp_retransmission);
  osip_setcb_nict_request_sent2(osip,&cb_sndreq_retransmission);
  osip_setcb_nist_2xx_sent2(osip,&cb_sndresp_retransmission);
  osip_setcb_nist_3456xx_sent2(osip,&cb_sndresp_retransmission);
  osip_setcb_nist_request_received2(osip,&cb_rcvreq_retransmission);
          
  osip_setcb_ict_transport_error(osip,&cb_transport_error);
  osip_setcb_ist_transport_error(osip,&cb_transport_error);
  osip_setcb_nict_transport_error(osip,&cb_transport_error);
  osip_setcb_nist_transport_error(osip,&cb_transport_error);
  
  osip_setcb_ict_invite_sent  (osip,&cb_sndinvite);
  osip_setcb_ict_ack_sent     (osip,&cb_sndack);
  osip_setcb_nict_register_sent(osip,&cb_sndregister);
  osip_setcb_nict_bye_sent     (osip,&cb_sndbye);
  osip_setcb_nict_cancel_sent  (osip,&cb_sndcancel);
  osip_setcb_nict_info_sent    (osip,&cb_sndinfo);
  osip_setcb_nict_options_sent (osip,&cb_sndoptions);
  osip_setcb_nict_subscribe_sent (osip,&cb_sndoptions);
  osip_setcb_nict_notify_sent (osip,&cb_sndoptions);
  /*  osip_setcb_nict_sndprack   (osip,&cb_sndprack); */
  osip_setcb_nict_unknown_sent(osip,&cb_sndunkrequest);

  osip_setcb_ict_1xx_received(osip,&cb_rcv1xx);
  osip_setcb_ict_2xx_received(osip,&cb_rcv2xx);
  osip_setcb_ict_3xx_received(osip,&cb_rcv3xx);
  osip_setcb_ict_4xx_received(osip,&cb_rcv4xx);
  osip_setcb_ict_5xx_received(osip,&cb_rcv5xx);
  osip_setcb_ict_6xx_received(osip,&cb_rcv6xx);
  
  osip_setcb_ist_1xx_sent(osip,&cb_snd1xx);
  osip_setcb_ist_2xx_sent(osip,&cb_snd2xx);
  osip_setcb_ist_3xx_sent(osip,&cb_snd3xx);
  osip_setcb_ist_4xx_sent(osip,&cb_snd4xx);
  osip_setcb_ist_5xx_sent(osip,&cb_snd5xx);
  osip_setcb_ist_6xx_sent(osip,&cb_snd6xx);
  
  osip_setcb_nict_1xx_received(osip,&cb_rcv1xx);
  osip_setcb_nict_2xx_received(osip,&cb_rcv2xx);
  osip_setcb_nict_3xx_received(osip,&cb_rcv3xx);
  osip_setcb_nict_4xx_received(osip,&cb_rcv4xx);
  osip_setcb_nict_5xx_received(osip,&cb_rcv5xx);
  osip_setcb_nict_6xx_received(osip,&cb_rcv6xx);
      

  osip_setcb_nist_1xx_sent(osip,&cb_snd1xx);
  osip_setcb_nist_2xx_sent(osip,&cb_snd2xx);
  osip_setcb_nist_3xx_sent(osip,&cb_snd3xx);
  osip_setcb_nist_4xx_sent(osip,&cb_snd4xx);
  osip_setcb_nist_5xx_sent(osip,&cb_snd5xx);
  osip_setcb_nist_6xx_sent(osip,&cb_snd6xx);
  
  osip_setcb_ist_invite_received  (osip,&cb_rcvinvite);
  osip_setcb_ist_ack_received     (osip,&cb_rcvack);
  osip_setcb_ist_ack_received2    (osip,&cb_rcvack2);
  osip_setcb_nist_register_received(osip,&cb_rcvregister);
  osip_setcb_nist_bye_received     (osip,&cb_rcvbye);
  osip_setcb_nist_cancel_received  (osip,&cb_rcvcancel);
  osip_setcb_nist_info_received    (osip,&cb_rcvinfo);
  osip_setcb_nist_options_received (osip,&cb_rcvoptions);
  osip_setcb_nist_subscribe_received (osip,&cb_rcvoptions);
  osip_setcb_nist_notify_received (osip,&cb_rcvoptions);
  osip_setcb_nist_unknown_received(osip,&cb_rcvunkrequest);

  return 0;
}
