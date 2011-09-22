/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002,2003,2004,2005,2006,2007  Aymeric MOIZARD  - jack@atosc.org
  
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

#ifdef _WIN32_WCE
#include <winsock2.h>
#include "inet_ntop.h"
#elif WIN32
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "inet_ntop.h"

#else
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <eXosip2/eXosip.h>
#include "eXosip2.h"

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif

extern eXosip_t eXosip;

void udp_tl_learn_port_from_via(osip_message_t * sip);

/* Private functions */
static void rcvregister_failure(osip_transaction_t * tr, osip_message_t * sip);
static void cb_xixt_kill_transaction(int type, osip_transaction_t * tr);
#ifndef MINISIZE
static void cb_rcvinvite(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcvack(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcvack2(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcvcancel(int type, osip_transaction_t * tr, osip_message_t * sip);
#endif
static void cb_rcvregister(int type, osip_transaction_t * tr,
						   osip_message_t * sip);
static void cb_rcvrequest(int type, osip_transaction_t * tr, osip_message_t * sip);
#ifndef MINISIZE
static void cb_sndinvite(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndack(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndregister(int type, osip_transaction_t * tr,
						   osip_message_t * sip);
static void cb_sndbye(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndcancel(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndinfo(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndoptions(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndnotify(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_sndsubscribe(int type, osip_transaction_t * tr,
							osip_message_t * sip);
static void cb_sndunkrequest(int type, osip_transaction_t * tr,
							 osip_message_t * sip);
#endif
static void cb_rcv1xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcv2xx_4invite(osip_transaction_t * tr, osip_message_t * sip);
#ifndef MINISIZE
static void cb_rcv2xx_4subscribe(osip_transaction_t * tr, osip_message_t * sip);
#endif
static void cb_rcv2xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcv3xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcv4xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcv5xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_rcv6xx(int type, osip_transaction_t * tr, osip_message_t * sip);
static void cb_snd123456xx(int type, osip_transaction_t * tr,
						   osip_message_t * sip);
#ifndef MINISIZE
static void cb_rcvresp_retransmission(int type, osip_transaction_t * tr,
									  osip_message_t * sip);
static void cb_sndreq_retransmission(int type, osip_transaction_t * tr,
									 osip_message_t * sip);
static void cb_sndresp_retransmission(int type, osip_transaction_t * tr,
									  osip_message_t * sip);
static void cb_rcvreq_retransmission(int type, osip_transaction_t * tr,
									 osip_message_t * sip);
#endif
static void cb_transport_error(int type, osip_transaction_t * tr, int error);

int
cb_snd_message(osip_transaction_t * tr, osip_message_t * sip, char *host,
			   int port, int out_socket)
{
	int i;
	osip_via_t *via;

#ifndef MINISIZE
	if (eXosip.dontsend_101 != 0 && sip->status_code == 101)
		return OSIP_SUCCESS;
#else
	if (sip->status_code == 101)
		return OSIP_SUCCESS;
#endif

	via = (osip_via_t *) osip_list_get(&sip->vias, 0);
	if (via == NULL || via->protocol == NULL)
		return -1;

	if (host == NULL) {
		if (MSG_IS_REQUEST(sip)) {
			osip_route_t *route;

			osip_message_get_route(sip, 0, &route);
			if (route != NULL) {
				osip_uri_param_t *lr_param = NULL;

				osip_uri_uparam_get_byname(route->url, "lr", &lr_param);
				if (lr_param == NULL)
					route = NULL;
			}

			if (route != NULL) {
				port = 5060;
				if (route->url->port != NULL)
					port = osip_atoi(route->url->port);
				host = route->url->host;
			} else {
				/* search for maddr parameter */
				osip_uri_param_t *maddr_param = NULL;
				osip_uri_uparam_get_byname(sip->req_uri, "maddr", &maddr_param);
				host = NULL;
				if (maddr_param != NULL && maddr_param->gvalue != NULL)
					host = maddr_param->gvalue;

				port = 5060;
				if (sip->req_uri->port != NULL)
					port = osip_atoi(sip->req_uri->port);

				if (host == NULL)
					host = sip->req_uri->host;
			}
		} else {
			osip_generic_param_t *maddr;
			osip_generic_param_t *received;
			osip_generic_param_t *rport;

			osip_via_param_get_byname(via, "maddr", &maddr);
			osip_via_param_get_byname(via, "received", &received);
			osip_via_param_get_byname(via, "rport", &rport);
			if (maddr != NULL && maddr->gvalue != NULL)
				host = maddr->gvalue;
			else if (received != NULL && received->gvalue != NULL)
				host = received->gvalue;
			else
				host = via->host;

			if (rport == NULL || rport->gvalue == NULL) {
				if (via->port != NULL)
					port = osip_atoi(via->port);
				else
					port = 5060;
			} else
				port = osip_atoi(rport->gvalue);
		}
	}

	if (eXosip.cbsipCallback != NULL) {
		eXosip.cbsipCallback(sip, 0);
	}

	i = -1;
	if (osip_strcasecmp(via->protocol, "udp") == 0) {
		i = eXtl_udp.tl_send_message(tr, sip, host, port, out_socket);
	} else if (osip_strcasecmp(via->protocol, "tcp") == 0) {
		i = eXtl_tcp.tl_send_message(tr, sip, host, port, out_socket);
	}
#ifdef HAVE_OPENSSL_SSL_H
	else if (osip_strcasecmp(via->protocol, "tls") == 0) {
		i = eXtl_tls.tl_send_message(tr, sip, host, port, out_socket);
	} else if (osip_strcasecmp(via->protocol, "dtls-udp") == 0) {
		i = -1;
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
		i = eXtl_dtls.tl_send_message(tr, sip, host, port, out_socket);
#endif
	}
#endif
	if (i != 0) {
		return i;
	}

	return OSIP_SUCCESS;

}

static void cb_xixt_kill_transaction(int type, osip_transaction_t * tr)
{
	int i;

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_nict_kill_transaction (id=%i)\r\n", tr->transactionid));
	i = osip_remove_transaction(eXosip.j_osip, tr);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_BUG, NULL,
					"cb_nict_kill_transaction Error: Could not remove transaction from the oSIP stack? (id=%i)\r\n",
					tr->transactionid));
	}

	if (MSG_IS_REGISTER(tr->orig_request)
		&& type == OSIP_NICT_KILL_TRANSACTION && tr->last_response == NULL) {
		rcvregister_failure(tr, NULL);
		return;
	}
#ifndef MINISIZE
	if (type == OSIP_NICT_KILL_TRANSACTION) {
		eXosip_dialog_t *jd;
		eXosip_subscribe_t *js;
		eXosip_notify_t *jn;
		eXosip_call_t *jc;
		jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

		if (jinfo == NULL && tr->last_response == NULL) {
			eXosip_event_t *je;
			je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REQUESTFAILURE, tr);
			report_event(je, NULL);
			return;
		}

		if (jinfo == NULL) {
			return;
		}

		jc = jinfo->jc;
		jd = jinfo->jd;
		jn = jinfo->jn;
		js = jinfo->js;

		if (jn == NULL && js == NULL) {
			eXosip_event_t *je;
			if (jc != NULL && tr->last_response == NULL) {
				report_call_event(EXOSIP_CALL_MESSAGE_REQUESTFAILURE, jc, jd, tr);
				return;
			}

			if (jc == NULL && tr->last_response == NULL) {
				je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REQUESTFAILURE,
												   tr);
				report_event(je, NULL);
			}
			return;
		}

		/* no answer to a NOTIFY request! */
		if (MSG_IS_NOTIFY(tr->orig_request) && tr->last_response == NULL) {
			/* delete the dialog! */
			eXosip_event_t *je;

			je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_REQUESTFAILURE,
											  jn, jd, tr);
			report_event(je, NULL);

			REMOVE_ELEMENT(eXosip.j_notifies, jn);
			eXosip_notify_free(jn);
			return;
		}

		if (jn != NULL && MSG_IS_NOTIFY(tr->orig_request)
			&& tr->last_response != NULL && tr->last_response->status_code > 299) {
			/* delete the dialog! */
			if (tr->last_response->status_code != 407
				&& tr->last_response->status_code != 401) {
				REMOVE_ELEMENT(eXosip.j_notifies, jn);
				eXosip_notify_free(jn);
				return;
			}
		}

		if (jn != NULL && MSG_IS_NOTIFY(tr->orig_request)
			&& tr->last_response != NULL
			&& tr->last_response->status_code > 199
			&& tr->last_response->status_code < 300) {
			if (jn->n_ss_status == EXOSIP_SUBCRSTATE_TERMINATED) {
				/* delete the dialog! */
				REMOVE_ELEMENT(eXosip.j_notifies, jn);
				eXosip_notify_free(jn);
				return;
			}
		}

		/* no answer to a SUBSCRIBE request! */
		if (js != NULL && MSG_IS_SUBSCRIBE(tr->orig_request)
			&& (tr->last_response == NULL
				|| tr->last_response->status_code <= 199)) {
			eXosip_event_t *je;

			je = eXosip_event_init_for_subscribe
				(EXOSIP_SUBSCRIPTION_REQUESTFAILURE, js, jd, tr);
			report_event(je, NULL);

			/* delete the dialog! */
			REMOVE_ELEMENT(eXosip.j_subscribes, js);
			eXosip_subscribe_free(js);
			return;
		}

		/* detect SUBSCRIBE request that close the dialogs! */
		if (MSG_IS_SUBSCRIBE(tr->orig_request)
			&& tr->last_response != NULL
			&& (tr->last_response->status_code == 401
				|| tr->last_response->status_code == 407)) {
				/* delete the dialog later because we need to authenticate */
		} else if (js != NULL && MSG_IS_SUBSCRIBE(tr->orig_request)) {
			osip_header_t *expires;

			osip_message_get_expires(tr->orig_request, 0, &expires);
			if (expires == NULL || expires->hvalue == NULL) {
			} else if (0 == strcmp(expires->hvalue, "0")) {
				REMOVE_ELEMENT(eXosip.j_subscribes, js);
				eXosip_subscribe_free(js);
				return;
			}
		}
	}
#endif
}

#ifndef MINISIZE

static void cb_rcvinvite(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcvinvite (id=%i)\n",
				tr->transactionid));
}

static void cb_rcvack(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcvack (id=%i)\n",
				tr->transactionid));
}

static void cb_rcvack2(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcvack2 (id=%i)\r\n",
				tr->transactionid));
}

static void cb_rcvcancel(int type, osip_transaction_t * tr, osip_message_t * sip)
{
        jinfo_t* jinfo;
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_rcvcancel (id=%i)\r\n", tr->transactionid));

	jinfo = (jinfo_t *) osip_transaction_get_your_instance (tr);
	if (jinfo == NULL)
	  {
	    return;
	  }

	if ((jinfo->jd != NULL) && (jinfo->jc != NULL))
	  {
		  /* do propagate CANCEL to the application */
		  report_call_event (EXOSIP_CALL_CANCELLED, jinfo->jc, jinfo->jd, tr);
	  }
}

#endif

static void cb_rcvregister(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_event_t *je;

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_rcvregister (id=%i)\r\n", tr->transactionid));

	je = eXosip_event_init_for_message(EXOSIP_MESSAGE_NEW, tr);
	eXosip_event_add(je);
	return;
}

static void cb_rcvrequest(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
#ifndef MINISIZE
	eXosip_notify_t *jn;
	eXosip_subscribe_t *js;
#endif

	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_rcvunkrequest (id=%i)\r\n", tr->transactionid));

	if (jinfo == NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_NEW, tr);
		eXosip_event_add(je);
		return;
	}

	jd = jinfo->jd;
	jc = jinfo->jc;
#ifndef MINISIZE
	jn = jinfo->jn;
	js = jinfo->js;
	if (jc == NULL && jn == NULL && js == NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_NEW, tr);
		eXosip_event_add(je);
		return;
	}
#else
	if (jc == NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_NEW, tr);
		eXosip_event_add(je);
		return;
	}
#endif
	else if (jc != NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO3, NULL,
					"cb_rcv? (id=%i)\r\n", tr->transactionid));

		if (MSG_IS_BYE(sip)) {
			/* already sent */
		} else
			report_call_event(EXOSIP_CALL_MESSAGE_NEW, jc, jd, tr);
		return;
	}
#ifndef MINISIZE
	else if (jn != NULL) {
		if (MSG_IS_SUBSCRIBE(sip)) {
			eXosip_event_t *je;
			je = eXosip_event_init_for_notify(EXOSIP_IN_SUBSCRIPTION_NEW, jn, jd,
											  tr);
			report_event(je, NULL);
			return;
		}
		return;
	} else if (js != NULL) {
		if (MSG_IS_NOTIFY(sip)) {
			eXosip_event_t *je;
			je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_NOTIFY, js,
												 jd, tr);
			report_event(je, NULL);
			return;
		}
		return;
	}
#endif
}

#ifndef MINISIZE

static void cb_sndinvite(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndinvite (id=%i)\r\n", tr->transactionid));
}

static void cb_sndack(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_sndack (id=%i)\r\n",
				tr->transactionid));
}

static void cb_sndregister(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndregister (id=%i)\r\n", tr->transactionid));
}

static void cb_sndbye(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_sndbye (id=%i)\r\n",
				tr->transactionid));
}

static void cb_sndcancel(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndcancel (id=%i)\r\n", tr->transactionid));
}

static void cb_sndinfo(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_sndinfo (id=%i)\r\n",
				tr->transactionid));
}

static void cb_sndoptions(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndoptions (id=%i)\r\n", tr->transactionid));
}

static void cb_sndnotify(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndnotify (id=%i)\r\n", tr->transactionid));
}

static void
cb_sndsubscribe(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndsubscribe (id=%i)\r\n", tr->transactionid));
}

static void
cb_sndunkrequest(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL,
				"cb_sndunkrequest (id=%i)\r\n", tr->transactionid));
}

#endif

void __eXosip_delete_jinfo(osip_transaction_t * transaction)
{
	jinfo_t *ji;

	if (transaction == NULL)
		return;
	ji = osip_transaction_get_your_instance(transaction);
	osip_free(ji);
	osip_transaction_set_your_instance(transaction, NULL);
}

#ifndef MINISIZE
jinfo_t *__eXosip_new_jinfo(eXosip_call_t * jc, eXosip_dialog_t * jd,
							eXosip_subscribe_t * js, eXosip_notify_t * jn)
#else
jinfo_t *__eXosip_new_jinfo(eXosip_call_t * jc, eXosip_dialog_t * jd)
#endif
{
	jinfo_t *ji = (jinfo_t *) osip_malloc(sizeof(jinfo_t));

	if (ji == NULL)
		return NULL;
	ji->jd = jd;
	ji->jc = jc;
#ifndef MINISIZE
	ji->js = js;
	ji->jn = jn;
#endif
	return ji;
}


static void cb_rcv1xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
#ifndef MINISIZE
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
#endif
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv1xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
#ifndef MINISIZE
	jn = jinfo->jn;
	js = jinfo->js;
#endif

	if (MSG_IS_RESPONSE_FOR(sip, "OPTIONS")) {
		if (jc == NULL) {
			eXosip_event_t *je;

			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO3, NULL,
						"cb_rcv1xx (id=%i) OPTIONS outside of any call\r\n",
						tr->transactionid));

			je = eXosip_event_init_for_message(EXOSIP_MESSAGE_PROCEEDING, tr);
			eXosip_event_add(je);
			return;
		}
		report_call_event(EXOSIP_CALL_MESSAGE_PROCEEDING, jc, jd, tr);
		return;
	}

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE") && MSG_TEST_CODE(sip, 100)) {
		report_call_event(EXOSIP_CALL_PROCEEDING, jc, jd, tr);
	}

	if ((MSG_IS_RESPONSE_FOR(sip, "INVITE")
#ifndef MINISIZE
		 || MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
#endif
		) && !MSG_TEST_CODE(sip, 100)) {
		int i;

#ifndef MINISIZE
		/* for SUBSCRIBE, test if the dialog has been already created
		   with a previous NOTIFY */
		if (jd == NULL && js != NULL && js->s_dialogs != NULL
			&& MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
			/* find if existing dialog match the to tag */
			osip_generic_param_t *tag;
			int i;

			i = osip_to_get_tag(sip->to, &tag);
			if (i == 0 && tag != NULL && tag->gvalue != NULL) {
				for (jd = js->s_dialogs; jd != NULL; jd = jd->next) {
					if (0 == strcmp(jd->d_dialog->remote_tag, tag->gvalue)) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO1, NULL,
									"eXosip: found established early dialog for this subscribe\n"));
						jinfo->jd = jd;
						break;
					}
				}
			}
		}
#endif

		if (jd == NULL) {		/* This transaction initiate a dialog in the case of
								   INVITE (else it would be attached to a "jd" element. */
			/* allocate a jd */

			i = eXosip_dialog_init_as_uac(&jd, sip);
			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: cannot establish a dialog\n"));
				return;
			}
			if (jc != NULL) {
				ADD_ELEMENT(jc->c_dialogs, jd);
				jinfo->jd = jd;
				eXosip_update();
			}
#ifndef MINISIZE
			else if (js != NULL) {
				ADD_ELEMENT(js->s_dialogs, jd);
				jinfo->jd = jd;
				eXosip_update();
			} else if (jn != NULL) {
				ADD_ELEMENT(jn->n_dialogs, jd);
				jinfo->jd = jd;
				eXosip_update();
			}
#endif
			else {
			}
			osip_transaction_set_your_instance(tr, jinfo);
		} else {
			if (jd->d_dialog == NULL) {
			} else if (jd->d_dialog->remote_tag == NULL) {
				osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
				osip_dialog_update_tag_as_uac(jd->d_dialog, sip);
			} else {
				osip_generic_param_t *tag;
				int i;

				i = osip_to_get_tag(sip->to, &tag);
				if (tag != NULL && tag->gvalue != NULL
					&& 0 == strcmp(jd->d_dialog->remote_tag, tag->gvalue)) {
					/* Update only if it is the same dialog */
					osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
				}
#if 1							/* to be tested */
				else {
					/* the best thing is to replace the current dialog
					   information... Much easier than creating a useless dialog! */
					osip_dialog_free(jd->d_dialog);
					i = osip_dialog_init_as_uac(&(jd->d_dialog), sip);
					if (i != 0) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_ERROR, NULL,
									"Cannot replace the dialog.\r\n"));
					} else {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING, NULL,
									"The dialog has been replaced with the new one from 1xx.\r\n"));
					}
				}
#endif
			}
		}

		if (jd != NULL)
			jd->d_STATE = JD_TRYING;
		if (jd != NULL && MSG_IS_RESPONSE_FOR(sip, "INVITE")
			&& sip->status_code < 180) {
			report_call_event(EXOSIP_CALL_PROCEEDING, jc, jd, tr);
		} else if (jd != NULL && MSG_IS_RESPONSE_FOR(sip, "INVITE")
				   && sip->status_code >= 180) {
			report_call_event(EXOSIP_CALL_RINGING, jc, jd, tr);
		}
#ifndef MINISIZE
		else if (jd != NULL && MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
			eXosip_event_t *je;

			je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_PROCEEDING,
												 js, jd, tr);
			report_event(je, sip);
		}
#endif
		if (MSG_TEST_CODE(sip, 180) && jd != NULL) {
			jd->d_STATE = JD_RINGING;
		} else if (MSG_TEST_CODE(sip, 183) && jd != NULL) {
			jd->d_STATE = JD_QUEUED;
		}
		if (jc != NULL && MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
			eXosip_call_renew_expire_time(jc);
        }
	}
}

static void cb_rcv2xx_4invite(osip_transaction_t * tr, osip_message_t * sip)
{
	int i;
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	if (jd == NULL) {			/* This transaction initiate a dialog in the case of
								   INVITE (else it would be attached to a "jd" element. */
		/* allocate a jd */
		i = eXosip_dialog_init_as_uac(&jd, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot establish a dialog\n"));
			return;
		}
		ADD_ELEMENT(jc->c_dialogs, jd);
		jinfo->jd = jd;
		eXosip_update();
		osip_transaction_set_your_instance(tr, jinfo);
	} else {
		/* Here is a special case:
		   We have initiated a dialog and we have received informationnal
		   answers from 2 or more remote SIP UA. Those answer can be
		   differentiated with the "To" header's tag.

		   We have used the first informationnal answer to create a
		   dialog, but we now want to be sure the 200ok received is
		   for the dialog this dialog.

		   We have to check the To tag and if it does not match, we
		   just have to modify the existing dialog and replace it. */
		osip_generic_param_t *tag;
		int i;

		i = osip_to_get_tag(sip->to, &tag);
		i = 1;					/* default is the same dialog */

		if (jd->d_dialog == NULL) {
			/* There are real use-case where a BYE is received/processed before
			   the 200ok of the previous INVITE. In this case, jd->d_dialog is
			   empty and the transaction should be silently discarded. */
			/* a ACK should still be sent... -but there is no dialog built- */
			return;
		}

		if (jd->d_dialog->remote_tag == NULL && tag == NULL) {
		} /* non compliant remote UA -> assume it is the same dialog */
		else if (jd->d_dialog->remote_tag != NULL && tag == NULL) {
			i = 0;
		} /* different dialog! */
		else if (jd->d_dialog->remote_tag == NULL && tag != NULL) {
			i = 0;
		} /* different dialog! */
		else if (jd->d_dialog->remote_tag != NULL && tag != NULL
				 && tag->gvalue != NULL
				 && 0 != strcmp(jd->d_dialog->remote_tag, tag->gvalue)) {
			i = 0;
		}
		/* different dialog! */
		if (i == 1) {			/* just update the dialog */
			osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
			if (jd->d_dialog->remote_tag == NULL)
				osip_dialog_update_tag_as_uac(jd->d_dialog, sip);
			osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
		} else {
			/* the best thing is to replace the current dialog
			   information... Much easier than creating a useless dialog! */
			osip_dialog_free(jd->d_dialog);
			i = osip_dialog_init_as_uac(&(jd->d_dialog), sip);
			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Cannot replace the dialog.\r\n"));
			} else {
				jd->d_dialog->local_cseq =
					jd->d_dialog->local_cseq + jd->d_mincseq;
				jd->d_mincseq = 0;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"The dialog has been replaced with the new one from 200ok.\r\n"));
			}
		}
	}

	if (jd != NULL) {
		osip_header_t *se_exp = NULL;
		osip_header_t *se_exp_answer = NULL;
		osip_message_header_get_byname(tr->orig_request, "session-expires", 0,
									   &se_exp);
		if (se_exp == NULL)
			osip_message_header_get_byname(tr->orig_request, "x", 0, &se_exp);
		osip_message_header_get_byname(sip, "session-expires", 0, &se_exp_answer);
		if (se_exp_answer == NULL)
			osip_message_header_get_byname(sip, "x", 0, &se_exp_answer);

		if (se_exp != NULL && se_exp_answer != NULL) {
			osip_content_disposition_t *exp_h = NULL;
			/* syntax of Session-Expires is equivalent to "Content-Disposition" */
			osip_content_disposition_init(&exp_h);
			if (exp_h != NULL) {
				osip_content_disposition_parse(exp_h, se_exp_answer->hvalue);
				if (exp_h->element != NULL) {
					osip_generic_param_t *param = NULL;
					osip_generic_param_get_byname(&exp_h->gen_params, "refresher",
												  &param);
					if (param == NULL) {
						jd->d_refresher = 0;	/* me? in which case? */
					} else {
						if (osip_strcasecmp(param->gvalue, "uac") == 0)
							jd->d_refresher = 0;
						else
							jd->d_refresher = 1;
					}
					jd->d_session_timer_start = time(NULL);
					jd->d_session_timer_length = atoi(exp_h->element);
					if (jd->d_session_timer_length <= 90)
						jd->d_session_timer_length = 90;
				}
				osip_content_disposition_free(exp_h);
				exp_h = NULL;
			}
		}
	}

	jd->d_STATE = JD_ESTABLISHED;

	/* eXosip_dialog_set_200ok (jd, sip); */

	report_call_event(EXOSIP_CALL_ANSWERED, jc, jd, tr);

	/* look for the SDP information and decide if this answer was for
	   an initial INVITE, an HoldCall, or a RetreiveCall */

	/* don't handle hold/unhold by now... */
	/* eXosip_update_audio_session(tr); */
}

#ifndef MINISIZE
static void cb_rcv2xx_4subscribe(osip_transaction_t * tr, osip_message_t * sip)
{
	int i;
	eXosip_dialog_t *jd;
	eXosip_subscribe_t *js;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	js = jinfo->js;
	_eXosip_subscribe_set_refresh_interval(js, sip);


	/* for SUBSCRIBE, test if the dialog has been already created
	   with a previous NOTIFY */
	if (jd == NULL && js != NULL && js->s_dialogs != NULL
		&& MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		/* find if existing dialog match the to tag */
		osip_generic_param_t *tag;
		int i;

		i = osip_to_get_tag(sip->to, &tag);
		if (i == 0 && tag != NULL && tag->gvalue != NULL) {
			for (jd = js->s_dialogs; jd != NULL; jd = jd->next) {
				if (0 == strcmp(jd->d_dialog->remote_tag, tag->gvalue)) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"eXosip: found established early dialog for this subscribe\n"));
					jinfo->jd = jd;
					break;
				}
			}
		}
	}

	if (jd == NULL) {			/* This transaction initiate a dialog in the case of
								   SUBSCRIBE (else it would be attached to a "jd" element. */
		/* allocate a jd */
		i = eXosip_dialog_init_as_uac(&jd, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot establish a dialog\n"));
			return;
		}
		ADD_ELEMENT(js->s_dialogs, jd);
		jinfo->jd = jd;
		eXosip_update();
		osip_transaction_set_your_instance(tr, jinfo);
	} else {
		osip_dialog_update_route_set_as_uac(jd->d_dialog, sip);
		if (jd->d_dialog->remote_tag == NULL)
			osip_dialog_update_tag_as_uac(jd->d_dialog, sip);
		osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
	}

	jd->d_STATE = JD_ESTABLISHED;
	/* look for the body information */

	{
		eXosip_event_t *je;

		je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_ANSWERED, js, jd,
											 tr);
		report_event(je, sip);
	}

}

#endif

static int
_eXosip_update_expires_according_to_contact(eXosip_reg_t * jreg,
											osip_transaction_t * tr,
											osip_message_t * sip)
{
	osip_contact_t *co_register;
	int pos;
	int maxval = 0;
	if (jreg == NULL)
		return OSIP_BADPARAMETER;

	/* only update if only one Contact was in INVITE */
	if (tr->orig_request == NULL)
		return OSIP_BADPARAMETER;

	if (osip_list_size(&tr->orig_request->contacts) != 1)
		return OSIP_SUCCESS;


	/* search for matching contact (line parameter must be equal) */
	pos = 0;
	co_register = (osip_contact_t *) osip_list_get(&sip->contacts, pos);
	while (co_register != NULL) {
		osip_uri_param_t *line_param = NULL;
		if (co_register->url != NULL)
			osip_uri_uparam_get_byname(co_register->url, "line", &line_param);

		if (line_param != NULL && line_param->gvalue != NULL) {
			if (osip_strcasecmp(jreg->r_line, line_param->gvalue) == 0) {
				/* found contact */
				int val;
				osip_generic_param_t *exp_param = NULL;
				osip_contact_param_get_byname(co_register, "expires", &exp_param);
				if (exp_param != NULL && exp_param->gvalue != NULL) {
					val = atoi(exp_param->gvalue);
					if (val>maxval)
					{
						maxval=val;
					}
				}
			}
		}

		pos++;
		co_register = (osip_contact_t *) osip_list_get(&sip->contacts, pos);
	}

    if (maxval==0)
       return OSIP_NOTFOUND;

	/* update only if expires value has REALLY be
	decreased (more than one minutes):
	In many cases value is decreased because a few seconds has
	elapsed when server send the 200ok. */
	if (maxval < jreg->r_reg_period - 15)
	{
		jreg->r_reg_period = maxval;
	}
	else if (maxval>jreg->r_reg_period) 
	{
		jreg->r_reg_period = maxval;
	}
	return OSIP_SUCCESS;
}


static void cb_rcv2xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
#ifndef MINISIZE
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
#endif
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv2xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

#ifndef MINISIZE
	if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH")) {
		eXosip_pub_t *pub = NULL;
		eXosip_event_t *je;
		int i;

		i = _eXosip_pub_update(&pub, tr, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"cb_rcv2xx (id=%i) No publication to update\r\n",
						tr->transactionid));
		}
		if (pub != NULL) {
			/* update registration interval */
			osip_header_t *exp;

			osip_message_header_get_byname(sip, "expires", 0, &exp);
			if (exp != NULL && exp->hvalue != NULL) {
				int val = atoi(exp->hvalue);
				if (val > 0) {
					/* update only if expires value has REALLY be
					   decreased (more than one minutes):
					   In many cases value is decreased because a few seconds has
					   elapsed when server send the 200ok. */
					if (val < pub->p_period - 15) {
						pub->p_period = val;
					}
				}
			}
			pub->p_retry = 0;	/* reset value */
		}

		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_ANSWERED, tr);
		report_event(je, sip);
		return;
	} else
#endif
	if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		eXosip_event_t *je;
		eXosip_reg_t *jreg = NULL;

		/* find matching j_reg */
		_eXosip_reg_find(&jreg, tr);
		if (jreg != NULL) {
			/* update registration interval */
			osip_header_t *exp;

			osip_message_header_get_byname(sip, "expires", 0, &exp);
			if (exp != NULL && exp->hvalue != NULL) {
				int val = atoi(exp->hvalue);
				if (val > 0) {
					/* update only if expires value has REALLY be
					   decreased (more than one minutes):
					   In many cases value is decreased because a few seconds has
					   elapsed when server send the 200ok. */
					if (val < jreg->r_reg_period - 15) {
						jreg->r_reg_period = val;
					}
				}
			}

			_eXosip_update_expires_according_to_contact(jreg, tr, sip);

			je = eXosip_event_init_for_reg(EXOSIP_REGISTRATION_SUCCESS, jreg, tr);
			report_event(je, sip);
			jreg->r_retry = 0;	/* reset value */
		}

		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
#ifndef MINISIZE
	jn = jinfo->jn;
	js = jinfo->js;
#endif

	if (jd != NULL)
		jd->d_retry = 0;		/* reset marker for authentication */
	if (jc != NULL)
		jc->c_retry = 0;		/* reset marker for authentication */
#ifndef MINISIZE
	if (js != NULL)
		js->s_retry = 0;		/* reset marker for authentication */
#endif

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		cb_rcv2xx_4invite(tr, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "BYE")) {
		if (jd != NULL)
			jd->d_STATE = JD_TERMINATED;
		report_call_event(EXOSIP_CALL_MESSAGE_ANSWERED, jc, jd, tr);
	}
#ifndef MINISIZE
	else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		cb_rcv2xx_4subscribe(tr, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY") && jn != NULL) {
		eXosip_event_t *je;
		osip_header_t *sub_state;

		je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_ANSWERED, jn, jd,
										  tr);
		report_event(je, sip);

		osip_message_header_get_byname(tr->orig_request, "subscription-state",
									   0, &sub_state);
		if (sub_state == NULL || sub_state->hvalue == NULL) {
			/* UNCOMPLIANT UA without a subscription-state header */
		} else if (0 == osip_strncasecmp(sub_state->hvalue, "terminated", 10)) {
			/* delete the dialog! */
			if (jn != NULL) {
				REMOVE_ELEMENT(eXosip.j_notifies, jn);
				eXosip_notify_free(jn);
			}
		}
	}
#endif
	else if (jc != NULL) {
		if (MSG_IS_RESPONSE_FOR(sip, "UPDATE")) {
			if (jd != NULL) {
				osip_header_t *se_exp = NULL;
				osip_header_t *se_exp_answer = NULL;
				osip_message_header_get_byname(tr->orig_request, "session-expires",
											   0, &se_exp);
				if (se_exp == NULL)
					osip_message_header_get_byname(tr->orig_request, "x", 0,
												   &se_exp);
				osip_message_header_get_byname(sip, "session-expires", 0,
											   &se_exp_answer);
				if (se_exp_answer == NULL)
					osip_message_header_get_byname(sip, "x", 0, &se_exp_answer);

				if (se_exp != NULL && se_exp_answer != NULL) {
					osip_content_disposition_t *exp_h = NULL;
					/* syntax of Session-Expires is equivalent to "Content-Disposition" */
					osip_content_disposition_init(&exp_h);
					if (exp_h != NULL) {
						osip_content_disposition_parse(exp_h,
													   se_exp_answer->hvalue);
						if (exp_h->element != NULL) {
							osip_generic_param_t *param = NULL;
							osip_generic_param_get_byname(&exp_h->gen_params,
														  "refresher", &param);
							if (param == NULL) {
								jd->d_refresher = 0;	/* me? in which case? */
							} else {
								if (osip_strcasecmp(param->gvalue, "uac") == 0)
									jd->d_refresher = 0;
								else
									jd->d_refresher = 1;
							}
							jd->d_session_timer_start = time(NULL);
							jd->d_session_timer_length = atoi(exp_h->element);
							if (jd->d_session_timer_length <= 90)
								jd->d_session_timer_length = 90;
						}
						osip_content_disposition_free(exp_h);
						exp_h = NULL;
					}
				}
			}
		}
		report_call_event(EXOSIP_CALL_MESSAGE_ANSWERED, jc, jd, tr);
		return;
	}
#ifndef MINISIZE
	else if (jc == NULL && js == NULL && jn == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_ANSWERED, tr);
		report_event(je, sip);
		return;
	}
#else
	else if (jc == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_ANSWERED, tr);
		report_event(je, sip);
		return;
	}
#endif
}

void eXosip_delete_early_dialog(eXosip_dialog_t * jd)
{
	if (jd == NULL)				/* bug? */
		return;

	/* an early dialog was created, but the call is not established */
	if (jd->d_dialog != NULL && jd->d_dialog->state == DIALOG_EARLY) {
		osip_dialog_free(jd->d_dialog);
		jd->d_dialog = NULL;
		eXosip_update();		/* AMD 30/09/05 */
		eXosip_dialog_set_state(jd, JD_TERMINATED);
	}
}

static void rcvregister_failure(osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_event_t *je;
	eXosip_reg_t *jreg = NULL;

	/* find matching j_reg */
	_eXosip_reg_find(&jreg, tr);
	if (jreg != NULL) {
		je = eXosip_event_init_for_reg(EXOSIP_REGISTRATION_FAILURE, jreg, tr);
		report_event(je, sip);
	}
}

#ifndef MINISIZE

static void cb_rcv3xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv3xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

	if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH")) {
		eXosip_event_t *je;
		eXosip_pub_t *pub;
		int i;

		i = _eXosip_pub_update(&pub, tr, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"cb_rcv3xx (id=%i) No publication to update\r\n",
						tr->transactionid));
		}
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REDIRECTED, tr);
		report_event(je, sip);
		return;
	} else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		rcvregister_failure(tr, sip);
		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	jn = jinfo->jn;
	js = jinfo->js;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		report_call_event(EXOSIP_CALL_REDIRECTED, jc, jd, tr);
	} else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY") && jn != NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_REDIRECTED, jn, jd,
										  tr);
		report_event(je, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_REDIRECTED, js,
											 jd, tr);
		report_event(je, sip);
	} else if (jc != NULL) {
		report_call_event(EXOSIP_CALL_MESSAGE_REDIRECTED, jc, jd, tr);
		return;
	} else if (jc == NULL && js == NULL && jn == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REDIRECTED, tr);
		report_event(je, sip);
		return;
	}

	if (jd == NULL)
		return;
	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
		|| MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_delete_early_dialog(jd);
		if (jd->d_dialog == NULL)
			jd->d_STATE = JD_REDIRECTED;
	}

}

static void cb_rcv4xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv4xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

	if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH")) {
		eXosip_pub_t *pub;
		eXosip_event_t *je;
		int i;

		i = _eXosip_pub_update(&pub, tr, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"cb_rcv4xx (id=%i) No publication to update\r\n",
						tr->transactionid));
		}
		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REQUESTFAILURE, tr);
		report_event(je, sip);
		return;
	} else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		rcvregister_failure(tr, sip);
		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	jn = jinfo->jn;
	js = jinfo->js;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		report_call_event(EXOSIP_CALL_REQUESTFAILURE, jc, jd, tr);
	} else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY") && jn != NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_REQUESTFAILURE,
										  jn, jd, tr);
		report_event(je, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_REQUESTFAILURE,
											 js, jd, tr);
		report_event(je, sip);
	} else if (jc != NULL) {
		report_call_event(EXOSIP_CALL_MESSAGE_REQUESTFAILURE, jc, jd, tr);
		return;
	} else if (jc == NULL && js == NULL && jn == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_REQUESTFAILURE, tr);
		report_event(je, sip);
		return;
	}

	if (jd == NULL)
		return;
	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
		|| MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_delete_early_dialog(jd);
		if (MSG_TEST_CODE(sip, 401) || MSG_TEST_CODE(sip, 407))
			jd->d_STATE = JD_AUTH_REQUIRED;
		else
			jd->d_STATE = JD_CLIENTERROR;
	}

}

static void cb_rcv5xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv5xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

	if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH")) {
		eXosip_pub_t *pub;
		eXosip_event_t *je;
		int i;

		i = _eXosip_pub_update(&pub, tr, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"cb_rcv3xx (id=%i) No publication to update\r\n",
						tr->transactionid));
		}
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_SERVERFAILURE, tr);
		report_event(je, sip);
		return;
	} else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		rcvregister_failure(tr, sip);
		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	jn = jinfo->jn;
	js = jinfo->js;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		report_call_event(EXOSIP_CALL_SERVERFAILURE, jc, jd, tr);
	} else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY") && jn != NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_SERVERFAILURE,
										  jn, jd, tr);
		report_event(je, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_SERVERFAILURE,
											 js, jd, tr);
		report_event(je, sip);
	} else if (jc != NULL) {
		report_call_event(EXOSIP_CALL_MESSAGE_SERVERFAILURE, jc, jd, tr);
		return;
	} else if (jc == NULL && js == NULL && jn == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_SERVERFAILURE, tr);
		report_event(je, sip);
		return;
	}

	if (jd == NULL)
		return;
	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
		|| MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_delete_early_dialog(jd);
		jd->d_STATE = JD_SERVERERROR;
	}

}

static void cb_rcv6xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	udp_tl_learn_port_from_via(sip);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv6xx (id=%i)\r\n",
				tr->transactionid));

	if (MSG_IS_RESPONSE_FOR(sip, "PUBLISH")) {
		eXosip_pub_t *pub;
		eXosip_event_t *je;
		int i;

		i = _eXosip_pub_update(&pub, tr, sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"cb_rcv6xx (id=%i) No publication to update\r\n",
						tr->transactionid));
		}
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_GLOBALFAILURE, tr);
		report_event(je, sip);
		return;
	} else if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		rcvregister_failure(tr, sip);
		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	jn = jinfo->jn;
	js = jinfo->js;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		report_call_event(EXOSIP_CALL_GLOBALFAILURE, jc, jd, tr);
	} else if (MSG_IS_RESPONSE_FOR(sip, "NOTIFY") && jn != NULL) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_notify(EXOSIP_NOTIFICATION_GLOBALFAILURE,
										  jn, jd, tr);
		report_event(je, sip);
	} else if (MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_event_t *je;

		je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_GLOBALFAILURE,
											 js, jd, tr);
		report_event(je, sip);
	} else if (jc != NULL) {
		report_call_event(EXOSIP_CALL_MESSAGE_GLOBALFAILURE, jc, jd, tr);
		return;
	} else if (jc == NULL && js == NULL && jn == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(EXOSIP_MESSAGE_GLOBALFAILURE, tr);
		report_event(je, sip);
		return;
	}

	if (jd == NULL)
		return;
	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
		|| MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")) {
		eXosip_delete_early_dialog(jd);
		jd->d_STATE = JD_GLOBALFAILURE;
	}

}

#else

static void
cb_rcv3456xx(int type, osip_transaction_t * tr, osip_message_t * sip,
			 int invite_event, int call_event, int outcall_event, int state);

static void
cb_rcv3456xx(int type, osip_transaction_t * tr, osip_message_t * sip,
			 int invite_event, int call_event, int outcall_event, int state)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_rcv3456xx (id=%i)\r\n",
				tr->transactionid));

	udp_tl_learn_port_from_via(sip);

	if (MSG_IS_RESPONSE_FOR(sip, "REGISTER")) {
		rcvregister_failure(tr, sip);
		return;
	}

	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		report_call_event(invite_event, jc, jd, tr);
	} else if (jc != NULL) {
		report_call_event(call_event, jc, jd, tr);
		return;
	} else if (jc == NULL) {
		eXosip_event_t *je;

		/* For all requests outside of calls */
		je = eXosip_event_init_for_message(outcall_event, tr);
		report_event(je, sip);
		return;
	}

	if (jd == NULL)
		return;
	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		eXosip_delete_early_dialog(jd);
		if (MSG_TEST_CODE(sip, 401) || MSG_TEST_CODE(sip, 407))
			jd->d_STATE = JD_AUTH_REQUIRED;
		else
			jd->d_STATE = state;
	}
}

static void cb_rcv3xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	cb_rcv3456xx(type, tr, sip,
				 EXOSIP_CALL_REDIRECTED,
				 EXOSIP_CALL_MESSAGE_REDIRECTED,
				 EXOSIP_MESSAGE_REDIRECTED, JD_REDIRECTED);
}

static void cb_rcv4xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	cb_rcv3456xx(type, tr, sip,
				 EXOSIP_CALL_REQUESTFAILURE,
				 EXOSIP_CALL_MESSAGE_REQUESTFAILURE,
				 EXOSIP_MESSAGE_REQUESTFAILURE, JD_CLIENTERROR);
}

static void cb_rcv5xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	cb_rcv3456xx(type, tr, sip,
				 EXOSIP_CALL_SERVERFAILURE,
				 EXOSIP_CALL_MESSAGE_SERVERFAILURE,
				 EXOSIP_MESSAGE_SERVERFAILURE, JD_SERVERERROR);
}

static void cb_rcv6xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	cb_rcv3456xx(type, tr, sip,
				 EXOSIP_CALL_GLOBALFAILURE,
				 EXOSIP_CALL_MESSAGE_GLOBALFAILURE,
				 EXOSIP_MESSAGE_GLOBALFAILURE, JD_REDIRECTED);
}

#endif

static void cb_snd123456xx(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO3, NULL, "cb_snd123456xx (id=%i)\r\n",
				tr->transactionid));
	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	if (jd == NULL)
		return;
	if (type == OSIP_IST_STATUS_1XX_SENT || type == OSIP_NIST_STATUS_1XX_SENT) {
		jd->d_STATE = JD_TRYING;
		return;
	}
	if (type == OSIP_IST_STATUS_2XX_SENT || type == OSIP_NIST_STATUS_2XX_SENT) {
		jd->d_STATE = JD_ESTABLISHED;
		return;
	}

	if (type == OSIP_IST_STATUS_3XX_SENT || type == OSIP_NIST_STATUS_3XX_SENT)
		jd->d_STATE = JD_REDIRECTED;
	else if (type == OSIP_IST_STATUS_4XX_SENT || type == OSIP_NIST_STATUS_4XX_SENT)
		jd->d_STATE = JD_CLIENTERROR;
	else if (type == OSIP_IST_STATUS_5XX_SENT || type == OSIP_NIST_STATUS_5XX_SENT)
		jd->d_STATE = JD_SERVERERROR;
	else if (type == OSIP_IST_STATUS_6XX_SENT || type == OSIP_NIST_STATUS_6XX_SENT)
		jd->d_STATE = JD_GLOBALFAILURE;

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")
#ifndef MINISIZE
		|| MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
#endif
		) {
		eXosip_delete_early_dialog(jd);
	}

	if (MSG_IS_RESPONSE_FOR(sip, "INVITE")) {
		/* only close calls if this is the initial INVITE */
		if (jc != NULL && tr == jc->c_inc_tr) {
			report_call_event(EXOSIP_CALL_CLOSED, jc, jd, tr);
		}
	}

}

#ifndef MINISIZE

static void
cb_rcvresp_retransmission(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_rcvresp_retransmission (id=%i)\r\n", tr->transactionid));
}

static void
cb_sndreq_retransmission(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_sndreq_retransmission (id=%i)\r\n", tr->transactionid));
}

static void
cb_sndresp_retransmission(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_sndresp_retransmission (id=%i)\r\n", tr->transactionid));
}

static void
cb_rcvreq_retransmission(int type, osip_transaction_t * tr, osip_message_t * sip)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_rcvreq_retransmission (id=%i)\r\n", tr->transactionid));
}

#endif

static void cb_transport_error(int type, osip_transaction_t * tr, int error)
{
#ifndef MINISIZE
	eXosip_dialog_t *jd;
	eXosip_call_t *jc;
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
	jinfo_t *jinfo = (jinfo_t *) osip_transaction_get_your_instance(tr);
#endif

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"cb_transport_error (id=%i)\r\n", tr->transactionid));

#ifndef MINISIZE
	if (jinfo == NULL)
		return;
	jd = jinfo->jd;
	jc = jinfo->jc;
	jn = jinfo->jn;
	js = jinfo->js;

	if (jn == NULL && js == NULL)
		return;

	if (jn != NULL && MSG_IS_NOTIFY(tr->orig_request)
		&& type == OSIP_NICT_TRANSPORT_ERROR) {
		/* delete the dialog! */
		REMOVE_ELEMENT(eXosip.j_notifies, jn);
		eXosip_notify_free(jn);
	}

	if (js != NULL && MSG_IS_SUBSCRIBE(tr->orig_request)
		&& type == OSIP_NICT_TRANSPORT_ERROR) {
		/* delete the dialog! */
		REMOVE_ELEMENT(eXosip.j_subscribes, js);
		eXosip_subscribe_free(js);
	}
#endif

#if 0
	if (jc->c_dialogs == NULL && type == OSIP_NICT_TRANSPORT_ERROR) {
		/* delete the dialog! */
		REMOVE_ELEMENT(eXosip.j_calls, jc);
		eXosip_call_free(jc);
	}
#endif

}



int eXosip_set_callbacks(osip_t * osip)
{
	/* register all callbacks */

	osip_set_cb_send_message(osip, &cb_snd_message);

	osip_set_kill_transaction_callback(osip, OSIP_ICT_KILL_TRANSACTION,
									   &cb_xixt_kill_transaction);
	osip_set_kill_transaction_callback(osip, OSIP_IST_KILL_TRANSACTION,
									   &cb_xixt_kill_transaction);
	osip_set_kill_transaction_callback(osip, OSIP_NICT_KILL_TRANSACTION,
									   &cb_xixt_kill_transaction);
	osip_set_kill_transaction_callback(osip, OSIP_NIST_KILL_TRANSACTION,
									   &cb_xixt_kill_transaction);

#ifndef MINISIZE
	osip_set_message_callback(osip, OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN,
							  &cb_rcvresp_retransmission);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN,
							  &cb_rcvresp_retransmission);
	osip_set_message_callback(osip, OSIP_ICT_INVITE_SENT_AGAIN,
							  &cb_sndreq_retransmission);
	osip_set_message_callback(osip, OSIP_IST_STATUS_2XX_SENT_AGAIN,
							  &cb_sndresp_retransmission);
	osip_set_message_callback(osip, OSIP_IST_STATUS_3456XX_SENT_AGAIN,
							  &cb_sndresp_retransmission);
	osip_set_message_callback(osip, OSIP_IST_INVITE_RECEIVED_AGAIN,
							  &cb_rcvreq_retransmission);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN,
							  &cb_rcvresp_retransmission);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN,
							  &cb_rcvresp_retransmission);
	osip_set_message_callback(osip, OSIP_NICT_REQUEST_SENT_AGAIN,
							  &cb_sndreq_retransmission);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_2XX_SENT_AGAIN,
							  &cb_sndresp_retransmission);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_3456XX_SENT_AGAIN,
							  &cb_sndresp_retransmission);
	osip_set_message_callback(osip, OSIP_NIST_REQUEST_RECEIVED_AGAIN,
							  &cb_rcvreq_retransmission);
#endif

	osip_set_transport_error_callback(osip, OSIP_ICT_TRANSPORT_ERROR,
									  &cb_transport_error);
	osip_set_transport_error_callback(osip, OSIP_IST_TRANSPORT_ERROR,
									  &cb_transport_error);
	osip_set_transport_error_callback(osip, OSIP_NICT_TRANSPORT_ERROR,
									  &cb_transport_error);
	osip_set_transport_error_callback(osip, OSIP_NIST_TRANSPORT_ERROR,
									  &cb_transport_error);

#ifndef MINISIZE
	osip_set_message_callback(osip, OSIP_ICT_INVITE_SENT, &cb_sndinvite);
	osip_set_message_callback(osip, OSIP_ICT_ACK_SENT, &cb_sndack);
	osip_set_message_callback(osip, OSIP_NICT_REGISTER_SENT, &cb_sndregister);
	osip_set_message_callback(osip, OSIP_NICT_BYE_SENT, &cb_sndbye);
	osip_set_message_callback(osip, OSIP_NICT_CANCEL_SENT, &cb_sndcancel);
	osip_set_message_callback(osip, OSIP_NICT_INFO_SENT, &cb_sndinfo);
	osip_set_message_callback(osip, OSIP_NICT_OPTIONS_SENT, &cb_sndoptions);
	osip_set_message_callback(osip, OSIP_NICT_SUBSCRIBE_SENT, &cb_sndsubscribe);
	osip_set_message_callback(osip, OSIP_NICT_NOTIFY_SENT, &cb_sndnotify);
	/*  osip_set_cb_nict_sndprack   (osip,&cb_sndprack); */
	osip_set_message_callback(osip, OSIP_NICT_UNKNOWN_REQUEST_SENT,
							  &cb_sndunkrequest);
#endif

	osip_set_message_callback(osip, OSIP_ICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
	osip_set_message_callback(osip, OSIP_ICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);

	osip_set_message_callback(osip, OSIP_IST_STATUS_1XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_IST_STATUS_2XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_IST_STATUS_3XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_IST_STATUS_4XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_IST_STATUS_5XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_IST_STATUS_6XX_SENT, &cb_snd123456xx);

	osip_set_message_callback(osip, OSIP_NICT_STATUS_1XX_RECEIVED, &cb_rcv1xx);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_2XX_RECEIVED, &cb_rcv2xx);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_3XX_RECEIVED, &cb_rcv3xx);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_4XX_RECEIVED, &cb_rcv4xx);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_5XX_RECEIVED, &cb_rcv5xx);
	osip_set_message_callback(osip, OSIP_NICT_STATUS_6XX_RECEIVED, &cb_rcv6xx);

	osip_set_message_callback(osip, OSIP_NIST_STATUS_1XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_2XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_3XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_4XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_5XX_SENT, &cb_snd123456xx);
	osip_set_message_callback(osip, OSIP_NIST_STATUS_6XX_SENT, &cb_snd123456xx);

#ifndef MINISIZE
	osip_set_message_callback(osip, OSIP_IST_INVITE_RECEIVED, &cb_rcvinvite);
	osip_set_message_callback(osip, OSIP_IST_ACK_RECEIVED, &cb_rcvack);
	osip_set_message_callback(osip, OSIP_IST_ACK_RECEIVED_AGAIN, &cb_rcvack2);
	osip_set_message_callback(osip, OSIP_NIST_CANCEL_RECEIVED, &cb_rcvcancel);
#endif
	osip_set_message_callback(osip, OSIP_NIST_REGISTER_RECEIVED, &cb_rcvregister);
	osip_set_message_callback(osip, OSIP_NIST_BYE_RECEIVED, &cb_rcvrequest);
	osip_set_message_callback(osip, OSIP_NIST_INFO_RECEIVED, &cb_rcvrequest);
	osip_set_message_callback(osip, OSIP_NIST_OPTIONS_RECEIVED, &cb_rcvrequest);
	osip_set_message_callback(osip, OSIP_NIST_SUBSCRIBE_RECEIVED, &cb_rcvrequest);
	osip_set_message_callback(osip, OSIP_NIST_NOTIFY_RECEIVED, &cb_rcvrequest);
	osip_set_message_callback(osip, OSIP_NIST_UNKNOWN_REQUEST_RECEIVED,
							  &cb_rcvrequest);

	return OSIP_SUCCESS;
}
