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


#include "eXosip2.h"

extern eXosip_t eXosip;

static int eXosip_create_transaction(eXosip_call_t * jc,
									 eXosip_dialog_t * jd,
									 osip_message_t * request);
static int eXosip_create_cancel_transaction(eXosip_call_t * jc,
											eXosip_dialog_t * jd,
											osip_message_t * request);

static int _eXosip_call_reuse_contact(osip_message_t * invite,
									  osip_message_t * msg);

static int
eXosip_create_transaction(eXosip_call_t * jc,
						  eXosip_dialog_t * jd, osip_message_t * request)
{
	osip_event_t *sipevent;
	osip_transaction_t *tr;
	int i;

	i = _eXosip_transaction_init(&tr, NICT, eXosip.j_osip, request);
	if (i != 0) {
		/* TODO: release the j_call.. */

		osip_message_free(request);
		return i;
	}

	if (jd != NULL)
		osip_list_add(jd->d_out_trs, tr, 0);

	sipevent = osip_new_outgoing_sipmessage(request);
	sipevent->transactionid = tr->transactionid;

#ifndef MINISIZE
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd));
#endif
	osip_transaction_add_event(tr, sipevent);
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

static int
eXosip_create_cancel_transaction(eXosip_call_t * jc,
								 eXosip_dialog_t * jd, osip_message_t * request)
{
	osip_event_t *sipevent;
	osip_transaction_t *tr;
	int i;

	i = _eXosip_transaction_init(&tr, NICT, eXosip.j_osip, request);
	if (i != 0) {
		/* TODO: release the j_call.. */

		osip_message_free(request);
		return i;
	}

	osip_list_add(&eXosip.j_transactions, tr, 0);

	sipevent = osip_new_outgoing_sipmessage(request);
	sipevent->transactionid = tr->transactionid;

	osip_transaction_add_event(tr, sipevent);
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

static int
_eXosip_call_reuse_contact(osip_message_t * invite, osip_message_t * msg)
{
	osip_contact_t *co_invite = NULL;
	osip_contact_t *co_msg = NULL;
	int i;
	i = osip_message_get_contact(invite, 0, &co_invite);
	if (i < 0 || co_invite == NULL || co_invite->url == NULL) {
		return i;
	}

	i = osip_message_get_contact(msg, 0, &co_msg);
	if (i >= 0 && co_msg != NULL) {
		osip_list_remove(&msg->contacts, 0);
		osip_contact_free(co_msg);
	}

	co_msg = NULL;
	i = osip_contact_clone(co_invite, &co_msg);
	if (i >= 0 && co_msg != NULL) {
		osip_list_add(&msg->contacts, co_msg, 0);
		return OSIP_SUCCESS;
	}
	return i;
}

int
_eXosip_call_transaction_find(int tid, eXosip_call_t ** jc,
							  eXosip_dialog_t ** jd, osip_transaction_t ** tr)
{
	for (*jc = eXosip.j_calls; *jc != NULL; *jc = (*jc)->next) {
		if ((*jc)->c_inc_tr != NULL && (*jc)->c_inc_tr->transactionid == tid) {
			*tr = (*jc)->c_inc_tr;
			*jd = (*jc)->c_dialogs;
			return OSIP_SUCCESS;
		}
		if ((*jc)->c_out_tr != NULL && (*jc)->c_out_tr->transactionid == tid) {
			*tr = (*jc)->c_out_tr;
			*jd = (*jc)->c_dialogs;
			return OSIP_SUCCESS;
		}
		for (*jd = (*jc)->c_dialogs; *jd != NULL; *jd = (*jd)->next) {
			osip_transaction_t *transaction;
			int pos = 0;

			while (!osip_list_eol((*jd)->d_inc_trs, pos)) {
				transaction =
					(osip_transaction_t *) osip_list_get((*jd)->d_inc_trs, pos);
				if (transaction != NULL && transaction->transactionid == tid) {
					*tr = transaction;
					return OSIP_SUCCESS;
				}
				pos++;
			}

			pos = 0;
			while (!osip_list_eol((*jd)->d_out_trs, pos)) {
				transaction =
					(osip_transaction_t *) osip_list_get((*jd)->d_out_trs, pos);
				if (transaction != NULL && transaction->transactionid == tid) {
					*tr = transaction;
					return OSIP_SUCCESS;
				}
				pos++;
			}
		}
	}
	*jd = NULL;
	*jc = NULL;
	return OSIP_NOTFOUND;
}

int eXosip_call_set_reference(int id, void *reference)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;

	if (id > 0) {
		eXosip_call_dialog_find(id, &jc, &jd);
		if (jc == NULL) {
			eXosip_call_find(id, &jc);
		}
	}
	if (jc == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return OSIP_NOTFOUND;
	}
	jc->external_reference = reference;
	return OSIP_SUCCESS;
}

void *eXosip_call_get_reference(int cid)
{
	eXosip_call_t *jc = NULL;

	eXosip_call_find(cid, &jc);
	if (jc == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return NULL;
	}

	return jc->external_reference;
}

/* this method can't be called unless the previous
   INVITE transaction is over. */
int
eXosip_call_build_initial_invite(osip_message_t ** invite,
								 const char *to,
								 const char *from,
								 const char *route, const char *subject)
{
	int i;
	osip_to_t *_to = NULL;
	osip_header_t *subject_header;

	*invite = NULL;

	if (to != NULL && *to == '\0')
		return OSIP_BADPARAMETER;
	if (route != NULL && *route == '\0')
		route = NULL;
	if (subject != NULL && *subject == '\0')
		subject = NULL;

	i = osip_to_init(&_to);
	if (i != 0)
		return i;

	i = osip_to_parse(_to, to);
	if (i != 0) {
		osip_to_free(_to);
		return i;
	}

	i = generating_request_out_of_dialog(invite, "INVITE", to,
										 eXosip.transport, from, route);
	osip_to_free(_to);
	if (i != 0)
		return i;
	_eXosip_dialog_add_contact(*invite, NULL);

	subject_header = NULL;
	osip_message_get_subject(*invite, 0, &subject_header);
	if (subject_header == NULL && subject != NULL)
		osip_message_set_subject(*invite, subject);

	return OSIP_SUCCESS;
}

int eXosip_call_send_initial_invite(osip_message_t * invite)
{
	eXosip_call_t *jc;
	osip_transaction_t *transaction;
	osip_event_t *sipevent;
	int i;

	if (invite == NULL) {
		osip_message_free(invite);
		return OSIP_BADPARAMETER;
	}

	i = eXosip_call_init(&jc);
	if (i != 0) {
		osip_message_free(invite);
		return i;
	}

	i = _eXosip_transaction_init(&transaction, ICT, eXosip.j_osip, invite);
	if (i != 0) {
		eXosip_call_free(jc);
		osip_message_free(invite);
		return i;
	}

	jc->c_out_tr = transaction;

	sipevent = osip_new_outgoing_sipmessage(invite);
	sipevent->transactionid = transaction->transactionid;

#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, NULL, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, NULL));
#endif
	osip_transaction_add_event(transaction, sipevent);

	jc->external_reference = NULL;
	ADD_ELEMENT(eXosip.j_calls, jc);

	eXosip_update();			/* fixed? */
	__eXosip_wakeup();
	return jc->c_id;
}

int eXosip_call_build_ack(int did, osip_message_t ** _ack)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;

	osip_message_t *ack;
	char *transport;
	int i;

	*_ack = NULL;

	if (did <= 0)
		return OSIP_BADPARAMETER;
	if (did > 0) {
		eXosip_call_dialog_find(did, &jc, &jd);
	}
	if (jc == NULL || jd == NULL || jd->d_dialog == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return OSIP_NOTFOUND;
	}

	tr = eXosip_find_last_invite(jc, jd);

	if (tr == NULL || tr->orig_request == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No transaction for call?\n"));
		return OSIP_NOTFOUND;
	}

	if (0 != osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: ACK are only sent for invite transactions\n"));
		return OSIP_BADPARAMETER;
	}

	transport = NULL;
	transport = _eXosip_transport_protocol(tr->orig_request);
	if (transport == NULL)
		i = _eXosip_build_request_within_dialog(&ack, "ACK", jd->d_dialog, "UDP");
	else
		i = _eXosip_build_request_within_dialog(&ack, "ACK", jd->d_dialog,
												transport);

	if (i != 0) {
		return i;
	}

	_eXosip_call_reuse_contact(tr->orig_request, ack);

	/* Fix CSeq Number when request has been exchanged during INVITE transactions */
	if (tr->orig_request->cseq != NULL && tr->orig_request->cseq->number != NULL) {
		if (ack != NULL && ack->cseq != NULL && ack->cseq->number != NULL) {
			osip_free(ack->cseq->number);
			ack->cseq->number = osip_strdup(tr->orig_request->cseq->number);
		}
	}

	/* copy all credentials from INVITE! */
	{
		int pos = 0;
		osip_proxy_authorization_t *pa = NULL;

		i = osip_message_get_proxy_authorization(tr->orig_request, pos, &pa);
		while (i >= 0 && pa != NULL) {
			osip_proxy_authorization_t *pa2;

			i = osip_proxy_authorization_clone(pa, &pa2);
			if (i != 0) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "Error in credential from INVITE\n"));
				break;
			}
			osip_list_add(&ack->proxy_authorizations, pa2, -1);
			pa = NULL;
			pos++;
			i = osip_message_get_proxy_authorization(tr->orig_request, pos, &pa);
		}
	}

	*_ack = ack;
	return OSIP_SUCCESS;
}

int eXosip_call_send_ack(int did, osip_message_t * ack)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	int i;

	osip_route_t *route;
	char *host=NULL;
	int port;

	if (did <= 0)
		return OSIP_BADPARAMETER;
	if (did > 0) {
		eXosip_call_dialog_find(did, &jc, &jd);
	}

	if (jc == NULL || jd == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		if (ack != NULL)
			osip_message_free(ack);
		return OSIP_NOTFOUND;
	}

	if (ack == NULL) {
		i = eXosip_call_build_ack(did, &ack);
		if (i != 0) {
			return i;
		}
	}

	if (host==NULL)
	{
		osip_message_get_route(ack, 0, &route);
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
			osip_uri_uparam_get_byname(ack->req_uri, "maddr", &maddr_param);
			host = NULL;
			if (maddr_param != NULL && maddr_param->gvalue != NULL)
				host = maddr_param->gvalue;

			port = 5060;
			if (ack->req_uri->port != NULL)
				port = osip_atoi(ack->req_uri->port);

			if (host == NULL)
				host = ack->req_uri->host;
		}
	}

	i = cb_snd_message(NULL, ack, host, port, -1);

	if (jd->d_ack != NULL)
		osip_message_free(jd->d_ack);
	jd->d_ack = ack;
	if (i < 0)
		return i;

	/* TODO: could be 1 for icmp... */
	return OSIP_SUCCESS;
}

int
eXosip_call_build_request(int jid, const char *method, osip_message_t ** request)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;

	osip_transaction_t *transaction;
	int i;

	*request = NULL;
	if (jid <= 0)
		return OSIP_BADPARAMETER;
	if (method == NULL || method[0] == '\0')
		return OSIP_BADPARAMETER;

	if (jid > 0) {
		eXosip_call_dialog_find(jid, &jc, &jd);
	}
	if (jd == NULL || jd->d_dialog == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return OSIP_NOTFOUND;
	}

	transaction = NULL;
	if (0 == osip_strcasecmp(method, "INVITE")) {
		transaction = eXosip_find_last_invite(jc, jd);
	} else {					/* OPTIONS, UPDATE, INFO, REFER, ?... */

		transaction = eXosip_find_last_transaction(jc, jd, method);
	}

	if (transaction != NULL) {
		if (0 != osip_strcasecmp(method, "INVITE")) {
			if (transaction->state != NICT_TERMINATED &&
				transaction->state != NIST_TERMINATED &&
				transaction->state != NICT_COMPLETED &&
				transaction->state != NIST_COMPLETED)
				return OSIP_WRONG_STATE;
		} else {
			if (transaction->state != ICT_TERMINATED &&
				transaction->state != IST_TERMINATED &&
				transaction->state != IST_CONFIRMED &&
				transaction->state != ICT_COMPLETED)
				return OSIP_WRONG_STATE;
		}
	}

	i = _eXosip_build_request_within_dialog(request, method, jd->d_dialog,
											eXosip.transport);
	if (i != 0)
		return i;

	eXosip_add_authentication_information(*request, NULL);

	return OSIP_SUCCESS;
}

int eXosip_call_send_request(int jid, osip_message_t * request)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;

	osip_transaction_t *transaction;
	osip_event_t *sipevent;

	int i;

	if (request == NULL)
		return OSIP_BADPARAMETER;
	if (jid <= 0) {
		osip_message_free(request);
		return OSIP_BADPARAMETER;
	}

	if (request->sip_method == NULL) {
		osip_message_free(request);
		return OSIP_BADPARAMETER;
	}

	if (jid > 0) {
		eXosip_call_dialog_find(jid, &jc, &jd);
	}
	if (jd == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		osip_message_free(request);
		return OSIP_NOTFOUND;
	}

	transaction = NULL;
	if (0 == osip_strcasecmp(request->sip_method, "INVITE")) {
		transaction = eXosip_find_last_invite(jc, jd);
	} else {					/* OPTIONS, UPDATE, INFO, REFER, ?... */

		transaction = eXosip_find_last_transaction(jc, jd, request->sip_method);
	}

	if (transaction != NULL) {
		if (0 != osip_strcasecmp(request->sip_method, "INVITE")) {
			if (transaction->state != NICT_TERMINATED &&
				transaction->state != NIST_TERMINATED &&
				transaction->state != NICT_COMPLETED &&
				transaction->state != NIST_COMPLETED) {
				osip_message_free(request);
				return OSIP_WRONG_STATE;
			}
		} else {
			if (transaction->state != ICT_TERMINATED &&
				transaction->state != IST_TERMINATED &&
				transaction->state != IST_CONFIRMED &&
				transaction->state != ICT_COMPLETED) {
				osip_message_free(request);
				return OSIP_WRONG_STATE;
			}
		}
	}

	transaction = NULL;
	if (0 != osip_strcasecmp(request->sip_method, "INVITE")) {
		i = _eXosip_transaction_init(&transaction, NICT, eXosip.j_osip, request);
	} else {
		i = _eXosip_transaction_init(&transaction, ICT, eXosip.j_osip, request);
	}

	if (i != 0) {
		osip_message_free(request);
		return i;
	}

	osip_list_add(jd->d_out_trs, transaction, 0);

	sipevent = osip_new_outgoing_sipmessage(request);
	sipevent->transactionid = transaction->transactionid;

#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd));
#endif
	osip_transaction_add_event(transaction, sipevent);
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

#ifndef MINISIZE

int
eXosip_call_build_refer(int did, const char *refer_to, osip_message_t ** request)
{
	int i;

	*request = NULL;
	i = eXosip_call_build_request(did, "REFER", request);
	if (i != 0)
		return i;

	if (refer_to == NULL || refer_to[0] == '\0')
		return OSIP_SUCCESS;

	osip_message_set_header(*request, "Refer-to", refer_to);
	return OSIP_SUCCESS;
}

int eXosip_call_build_options(int did, osip_message_t ** request)
{
	int i;

	*request = NULL;
	i = eXosip_call_build_request(did, "OPTIONS", request);
	if (i != 0)
		return i;

	return OSIP_SUCCESS;
}

int eXosip_call_build_info(int did, osip_message_t ** request)
{
	int i;

	*request = NULL;
	i = eXosip_call_build_request(did, "INFO", request);
	if (i != 0)
		return i;

	return OSIP_SUCCESS;
}

int eXosip_call_build_update(int did, osip_message_t ** request)
{
	int i;

	*request = NULL;
	i = eXosip_call_build_request(did, "UPDATE", request);
	if (i != 0)
		return i;

	return OSIP_SUCCESS;
}

int
eXosip_call_build_notify(int did, int subscription_status,
						 osip_message_t ** request)
{
	char subscription_state[50];
	char *tmp;
	int i;

	*request = NULL;
	i = eXosip_call_build_request(did, "NOTIFY", request);
	if (i != 0)
		return i;

	if (subscription_status == EXOSIP_SUBCRSTATE_PENDING)
		osip_strncpy(subscription_state, "pending;expires=", 16);
	else if (subscription_status == EXOSIP_SUBCRSTATE_ACTIVE)
		osip_strncpy(subscription_state, "active;expires=", 15);
	else if (subscription_status == EXOSIP_SUBCRSTATE_TERMINATED) {
#if 0
		int reason = NORESOURCE;
		if (reason == DEACTIVATED)
			osip_strncpy(subscription_state, "terminated;reason=deactivated", 29);
		else if (reason == PROBATION)
			osip_strncpy(subscription_state, "terminated;reason=probation", 27);
		else if (reason == REJECTED)
			osip_strncpy(subscription_state, "terminated;reason=rejected", 26);
		else if (reason == TIMEOUT)
			osip_strncpy(subscription_state, "terminated;reason=timeout", 25);
		else if (reason == GIVEUP)
			osip_strncpy(subscription_state, "terminated;reason=giveup", 24);
		else if (reason == NORESOURCE)
#endif
			osip_strncpy(subscription_state, "terminated;reason=noresource", 29);
	}
	tmp = subscription_state + strlen(subscription_state);
	if (subscription_status != EXOSIP_SUBCRSTATE_TERMINATED)
		snprintf(tmp, 50 - (tmp - subscription_state), "%i", 180);
	osip_message_set_header(*request, "Subscription-State", subscription_state);

	return OSIP_SUCCESS;
}

#endif

int eXosip_call_build_answer(int tid, int status, osip_message_t ** answer)
{
	int i = -1;
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;

	*answer = NULL;

	if (tid < 0)
		return OSIP_BADPARAMETER;
	if (status <= 100)
		return OSIP_BADPARAMETER;
	if (status > 699)
		return OSIP_BADPARAMETER;

	if (tid > 0) {
		_eXosip_call_transaction_find(tid, &jc, &jd, &tr);
	}
	if (tr == NULL || jd == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return OSIP_NOTFOUND;
	}

	if (0 == osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
		i = _eXosip_answer_invite_123456xx(jc, jd, status, answer, 0);
	} else {
		i = _eXosip_build_response_default(answer, jd->d_dialog, status,
											   tr->orig_request);
		if (i != 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "ERROR: Could not create response for %s\n",
								  tr->orig_request->sip_method));
			return i;
		}
		if (status > 100 && status < 300)
			i = complete_answer_that_establish_a_dialog(*answer, tr->orig_request);
	}

	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: Could not create response for %s\n",
							  tr->orig_request->sip_method));
		return i;
	}
	return OSIP_SUCCESS;
}

int eXosip_call_send_answer(int tid, int status, osip_message_t * answer)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;
	osip_event_t *evt_answer;

	if (tid < 0) {
		osip_message_free(answer);
		return OSIP_BADPARAMETER;
	}
	if (status <= 100) {
		osip_message_free(answer);
		return OSIP_BADPARAMETER;
	}
	if (status > 699) {
		osip_message_free(answer);
		return OSIP_BADPARAMETER;
	}

	if (tid > 0) {
		_eXosip_call_transaction_find(tid, &jc, &jd, &tr);
	}
	if (jd == NULL || tr == NULL || tr->orig_request == NULL
		|| tr->orig_request->sip_method == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here or no transaction for call\n"));
		osip_message_free(answer);
		return OSIP_NOTFOUND;
	}

	if (answer == NULL) {
		if (0 == osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
			if (status >= 200 && status <= 299) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Wrong parameter?\n"));
				osip_message_free(answer);
				return OSIP_BADPARAMETER;
			}
		}
	}

	/* is the transaction already answered? */
	if (tr->state == IST_COMPLETED
		|| tr->state == IST_CONFIRMED
		|| tr->state == IST_TERMINATED
		|| tr->state == NIST_COMPLETED || tr->state == NIST_TERMINATED) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: transaction already answered\n"));
		osip_message_free(answer);
		return OSIP_WRONG_STATE;
	}

	if (answer == NULL) {
		if (0 == osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
			osip_message_t *response;
			return _eXosip_answer_invite_123456xx(jc, jd, status, &response, 1);
		}
		osip_message_free(answer);
		return OSIP_BADPARAMETER;
	}

	if (0 == osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
		if (MSG_IS_STATUS_2XX(answer) && jd != NULL) {
			if (status >= 200 && status < 300 && jd != NULL) {
				eXosip_dialog_set_200ok(jd, answer);
				/* wait for a ACK */
				osip_dialog_set_state(jd->d_dialog, DIALOG_CONFIRMED);
			}
		}
	}

	if (0 == osip_strcasecmp(tr->orig_request->sip_method, "INVITE")
		|| 0 == osip_strcasecmp(tr->orig_request->sip_method, "UPDATE")) {
		if (MSG_IS_STATUS_2XX(answer) && jd != NULL) {
			osip_header_t *supported = NULL;
			int i = 0;

			/* look for timer in supported header: must be added by user-application */

			i = osip_message_header_get_byname(answer, "supported", 0, &supported);
			while (i >= 0) {
				if (supported == NULL)
					break;
				if (supported->hvalue != NULL
					&& osip_strcasecmp(supported->hvalue, "timer") == 0) {
					/*found */
					break;
				}
				supported = NULL;
				i = osip_message_header_get_byname(answer, "supported", i + 1,
												   &supported);
			}
			if (supported != NULL) {	/* timer is supported */
				/* copy session-expires */
				/* add refresher=uas, if it's not already there */
				osip_header_t *se_exp = NULL;
				osip_message_header_get_byname(tr->orig_request, "session-expires",
											   0, &se_exp);
				if (se_exp == NULL)
					osip_message_header_get_byname(tr->orig_request, "x", 0,
												   &se_exp);
				if (se_exp != NULL) {
					osip_header_t *cp = NULL;

					i = osip_header_clone(se_exp, &cp);
					if (cp != NULL) {
						osip_content_disposition_t *exp_h = NULL;
						/* syntax of Session-Expires is equivalent to "Content-Disposition" */
						osip_content_disposition_init(&exp_h);
						if (exp_h == NULL) {
							osip_header_free(cp);
						} else {
							osip_content_disposition_parse(exp_h, se_exp->hvalue);
							if (exp_h->element == NULL) {
								osip_content_disposition_free(exp_h);
								osip_header_free(cp);
								exp_h = NULL;
							} else {
								osip_generic_param_t *param = NULL;
								osip_generic_param_get_byname(&exp_h->gen_params,
															  "refresher", &param);
								if (param == NULL) {
									osip_generic_param_add(&exp_h->gen_params,
														   osip_strdup
														   ("refresher"),
														   osip_strdup("uas"));
									osip_free(cp->hvalue);
									cp->hvalue = NULL;
									osip_content_disposition_to_str(exp_h,
																	&cp->hvalue);
									jd->d_refresher = 0;
								} else {
									if (osip_strcasecmp(param->gvalue, "uas") == 0)
										jd->d_refresher = 0;
									else
										jd->d_refresher = 1;
								}
								jd->d_session_timer_start = time(NULL);
								jd->d_session_timer_length = atoi(exp_h->element);
								if (jd->d_session_timer_length <= 90)
									jd->d_session_timer_length = 90;
								osip_list_add(&answer->headers, cp, 0);
							}
						}
						if (exp_h)
							osip_content_disposition_free(exp_h);
						exp_h = NULL;


						/* add Require only if remote UA support "timer" */
						i = osip_message_header_get_byname(tr->orig_request,
														   "supported", 0,
														   &supported);
						while (i >= 0) {
							if (supported == NULL)
								break;
							if (supported->hvalue != NULL
								&& osip_strcasecmp(supported->hvalue,
												   "timer") == 0) {
								/*found */
								break;
							}
							supported = NULL;
							i = osip_message_header_get_byname(tr->orig_request,
															   "supported", i + 1,
															   &supported);
						}
						if (supported != NULL) {	/* timer is supported */
							osip_message_set_header(answer, "Require", "timer");
						}
					}
				}
			}
		}
	}

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = tr->transactionid;

	osip_transaction_add_event(tr, evt_answer);
	eXosip_update();
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

int eXosip_call_terminate(int cid, int did)
{
	int i;
	osip_transaction_t *tr;
	osip_message_t *request = NULL;
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;

	if (did <= 0 && cid <= 0)
		return OSIP_BADPARAMETER;
	if (did > 0) {
		eXosip_call_dialog_find(did, &jc, &jd);
		if (jd == NULL) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: No call here?\n"));
			return OSIP_NOTFOUND;
		}
	} else {
		eXosip_call_find(cid, &jc);
	}

	if (jc == NULL) {
		return OSIP_NOTFOUND;
	}

	tr = eXosip_find_last_out_invite(jc, jd);
	if (jd != NULL && jd->d_dialog != NULL
		&& jd->d_dialog->state == DIALOG_CONFIRMED) {
		/* don't send CANCEL on re-INVITE: send BYE instead */
	} else if (tr != NULL && tr->last_response != NULL
			   && MSG_IS_STATUS_1XX(tr->last_response)) {
		i = generating_cancel(&request, tr->orig_request);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot terminate this call!\n"));
			return i;
		}
		i = eXosip_create_cancel_transaction(jc, jd, request);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot initiate SIP transaction!\n"));
			return i;
		}
		if (jd != NULL) {
			/*Fix: keep dialog opened after the CANCEL.
			  osip_dialog_free(jd->d_dialog);
			  jd->d_dialog = NULL;
			  eXosip_update(); */
		}
		return OSIP_SUCCESS+1;
	}

	if (jd == NULL || jd->d_dialog == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No established dialog!\n"));
		return OSIP_WRONG_STATE;
	}

	if (tr == NULL) {
		/*this may not be enough if it's a re-INVITE! */
		tr = eXosip_find_last_inc_invite(jc, jd);
		if (tr != NULL && tr->last_response != NULL && MSG_IS_STATUS_1XX(tr->last_response)) {	/* answer with 603 */
			osip_generic_param_t *to_tag;
			osip_from_param_get_byname(tr->orig_request->to, "tag", &to_tag);

			i = eXosip_call_send_answer(tr->transactionid, 603, NULL);

			if (to_tag == NULL)
				return i;
		}
	}

	if (jd->d_dialog == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot terminate this call!\n"));
		return OSIP_WRONG_STATE;
	}

	i = generating_bye(&request, jd->d_dialog, eXosip.transport);

	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot terminate this call!\n"));
		return i;
	}

	eXosip_add_authentication_information(request, NULL);

	i = eXosip_create_transaction(jc, jd, request);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot initiate SIP transaction!\n"));
		return i;
	}

	osip_dialog_free(jd->d_dialog);
	jd->d_dialog = NULL;
	eXosip_update();			/* AMD 30/09/05 */
	return OSIP_SUCCESS;
}

#ifndef MINISIZE

int eXosip_call_build_prack(int tid, osip_message_t ** prack)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;

	osip_header_t *rseq;
	char *transport;
	int i;

	*prack = NULL;

	if (tid < 0)
		return OSIP_BADPARAMETER;

	if (tid > 0) {
		_eXosip_call_transaction_find(tid, &jc, &jd, &tr);
	}
	if (jc == NULL || jd == NULL || jd->d_dialog == NULL
		|| tr == NULL || tr->orig_request == NULL
		|| tr->orig_request->sip_method == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here or no transaction for call\n"));
		return OSIP_NOTFOUND;
	}

	if (0 != osip_strcasecmp(tr->orig_request->sip_method, "INVITE"))
		return OSIP_BADPARAMETER;

	/* PRACK are only send in the PROCEEDING state */
	if (tr->state != ICT_PROCEEDING)
		return OSIP_WRONG_STATE;

	if (tr->orig_request->cseq == NULL
		|| tr->orig_request->cseq->number == NULL
		|| tr->orig_request->cseq->method == NULL)
		return OSIP_SYNTAXERROR;

	transport = NULL;
	if (tr != NULL && tr->orig_request != NULL)
		transport = _eXosip_transport_protocol(tr->orig_request);

	if (transport == NULL)
		i = _eXosip_build_request_within_dialog(prack, "PRACK", jd->d_dialog,
												"UDP");
	else
		i = _eXosip_build_request_within_dialog(prack, "PRACK", jd->d_dialog,
												transport);

	if (i != 0)
		return i;

	osip_message_header_get_byname(tr->last_response, "RSeq", 0, &rseq);
	if (rseq != NULL && rseq->hvalue != NULL) {
		char tmp[128];

		memset(tmp, '\0', sizeof(tmp));
		snprintf(tmp, 127, "%s %s %s", rseq->hvalue,
				 tr->orig_request->cseq->number, tr->orig_request->cseq->method);
		osip_message_set_header(*prack, "RAck", tmp);
	}

	return OSIP_SUCCESS;
}

int eXosip_call_send_prack(int tid, osip_message_t * prack)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;

	osip_event_t *sipevent;
	int i;

	if (tid < 0)
		return OSIP_BADPARAMETER;
	if (prack == NULL)
		return OSIP_BADPARAMETER;

	if (tid > 0) {
		_eXosip_call_transaction_find(tid, &jc, &jd, &tr);
	}
	if (jc == NULL || jd == NULL || jd->d_dialog == NULL
		|| tr == NULL || tr->orig_request == NULL
		|| tr->orig_request->sip_method == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here or no transaction for call\n"));
		osip_message_free(prack);
		return OSIP_NOTFOUND;
	}

	if (0 != osip_strcasecmp(tr->orig_request->sip_method, "INVITE")) {
		osip_message_free(prack);
		return OSIP_BADPARAMETER;
	}

	/* PRACK are only send in the PROCEEDING state */
	if (tr->state != ICT_PROCEEDING) {
		osip_message_free(prack);
		return OSIP_WRONG_STATE;
	}

	tr = NULL;
	i = _eXosip_transaction_init(&tr, NICT, eXosip.j_osip, prack);

	if (i != 0) {
		osip_message_free(prack);
		return i;
	}

	jd->d_mincseq++;

	osip_list_add(jd->d_out_trs, tr, 0);

	sipevent = osip_new_outgoing_sipmessage(prack);
	sipevent->transactionid = tr->transactionid;

#ifndef MINISIZE
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd));
#endif
	osip_transaction_add_event(tr, sipevent);
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

#endif

int
_eXosip_call_retry_request(eXosip_call_t * jc,
						   eXosip_dialog_t * jd, osip_transaction_t * out_tr)
{
	osip_transaction_t *tr = NULL;
	osip_message_t *msg = NULL;
	osip_event_t *sipevent;

	int cseq;
	osip_via_t *via;
	osip_contact_t *co;
	int pos;
	int i;

	if (jc == NULL)
		return OSIP_BADPARAMETER;
	if (jd != NULL) {
		if (jd->d_out_trs == NULL)
			return OSIP_BADPARAMETER;
	}
	if (out_tr == NULL
		|| out_tr->orig_request == NULL || out_tr->last_response == NULL)
		return OSIP_BADPARAMETER;

	i = osip_message_clone(out_tr->orig_request, &msg);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: could not clone msg for authentication\n"));
		return i;
	}

	via = (osip_via_t *) osip_list_get(&msg->vias, 0);
	if (via == NULL || msg->cseq == NULL || msg->cseq->number == NULL) {
		osip_message_free(msg);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: missing via or cseq header\n"));
		return OSIP_SYNTAXERROR;
	}

	if (MSG_IS_STATUS_3XX(out_tr->last_response)) {
		co = NULL;
		pos = 0;
		while (!osip_list_eol(&out_tr->last_response->contacts, pos)) {
			co = (osip_contact_t *) osip_list_get(&out_tr->last_response->contacts,
												  pos);
			if (co != NULL && co->url != NULL) {
				/* check tranport? */
				osip_uri_param_t *u_param;

				u_param = NULL;
				osip_uri_uparam_get_byname(co->url, "transport", &u_param);
				if (u_param == NULL || u_param->gname == NULL
					|| u_param->gvalue == NULL) {
					if (0 == osip_strcasecmp(eXosip.transport, "udp"))
						break;	/* no transport param in uri & we want udp */
				} else if (0 == osip_strcasecmp(u_param->gvalue, eXosip.transport)) {
					break;		/* transport param in uri & match our protocol */
				}
			}
			pos++;
			co = NULL;
		}

		if (co == NULL || co->url == NULL) {
			osip_message_free(msg);
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: contact header\n"));
			return OSIP_SYNTAXERROR;
		}

		/* TODO:
		   remove extra parameter from new request-uri
		   check usual parameter like "transport"
		 */

		if (msg->req_uri != NULL && msg->req_uri->host != NULL
			&& co->url->host != NULL
			&& 0 == osip_strcasecmp(co->url->host, msg->req_uri->host)) {
			osip_uri_param_t *maddr_param = NULL;
			osip_uri_uparam_get_byname(co->url, "maddr", &maddr_param);
			if (maddr_param != NULL && maddr_param->gvalue != NULL) {
				/* This is a redirect server, the route should probably be removed? */
				osip_route_t *route = NULL;
				osip_generic_param_t *tag = NULL;
				osip_message_get_route(msg, 0, &route);
				if (route != NULL) {
					osip_to_get_tag(msg->to, &tag);
					if (tag == NULL && route != NULL && route->url != NULL) {
						osip_list_remove(&msg->routes, 0);
						osip_route_free(route);
					}
				}
			}
		}

		/* replace request-uri with NEW contact address */
		osip_uri_free(msg->req_uri);
		msg->req_uri = NULL;
		i = osip_uri_clone(co->url, &msg->req_uri);
		if (i != 0) {
			osip_message_free(msg);
			return i;
		}

		/* support for diversions headers/draft! */
		{
			int count = 0;
			pos = 0;
			while (!osip_list_eol(&out_tr->last_response->headers, pos)) {
				osip_header_t *copy = NULL;
				osip_header_t *head =
					osip_list_get(&out_tr->last_response->headers, pos);
				if (head != NULL && 0 == osip_strcasecmp(head->hname, "diversion")) {
					i = osip_header_clone(head, &copy);
					if (i == 0) {
						osip_list_add(&msg->headers, copy, count);
						count++;
					}
				}
				pos++;
			}
		}

	}
	/* remove all previous authentication headers */
	osip_list_special_free(&msg->authorizations,
						   (void (*)(void *)) &osip_authorization_free);
	osip_list_special_free(&msg->proxy_authorizations,
						   (void (*)(void *)) &osip_proxy_authorization_free);

	/* increment cseq */
	cseq = atoi(msg->cseq->number);
	osip_free(msg->cseq->number);
	msg->cseq->number = strdup_printf("%i", cseq + 1);
	if (jd != NULL && jd->d_dialog != NULL) {
		jd->d_dialog->local_cseq++;
	}

	i = eXosip_update_top_via(msg);
	if (i != 0) {
		osip_message_free(msg);
		return i;
	}

	if (out_tr->last_response->status_code == 422) {
		/* increase expires value to "min-se" value */
		osip_header_t *exp;
		osip_header_t *min_se;
		/* syntax of Min-SE & Session-Expires are equivalent to "Content-Disposition" */
		osip_content_disposition_t *exp_h = NULL;
		osip_content_disposition_t *min_se_h = NULL;

		osip_message_header_get_byname(msg, "session-expires", 0, &exp);
		if (exp == NULL)
			osip_message_header_get_byname(msg, "x", 0, &exp);
		osip_message_header_get_byname(out_tr->last_response, "min-se", 0,
									   &min_se);
		if (exp != NULL && exp->hvalue != NULL && min_se != NULL
			&& min_se->hvalue != NULL) {
			osip_content_disposition_init(&exp_h);
			osip_content_disposition_init(&min_se_h);
			if (exp_h == NULL || min_se_h == NULL) {
				osip_content_disposition_free(exp_h);
				osip_content_disposition_free(min_se_h);
				exp_h = NULL;
				min_se_h = NULL;
			} else {
				osip_content_disposition_parse(exp_h, exp->hvalue);
				osip_content_disposition_parse(min_se_h, min_se->hvalue);
				if (exp_h->element == NULL || min_se_h->element == NULL) {
					osip_content_disposition_free(exp_h);
					osip_content_disposition_free(min_se_h);
					exp_h = NULL;
					min_se_h = NULL;
				}
			}
		}

		if (exp_h != NULL && exp_h->element != NULL && min_se_h != NULL
			&& min_se_h->element != NULL) {
			osip_header_t *min_se_new = NULL;
			char minse_new[32];
			memset(minse_new, 0, sizeof(minse_new));

			osip_free(exp_h->element);
			exp_h->element = osip_strdup(min_se_h->element);

			/* rebuild session-expires with new value/same paramters */
			osip_free(exp->hvalue);
			exp->hvalue = NULL;
			osip_content_disposition_to_str(exp_h, &exp->hvalue);

			/* add or update Min-SE in INVITE: */
			osip_message_header_get_byname(msg, "min-se", 0, &min_se_new);
			if (min_se_new != NULL && min_se_new->hvalue != NULL) {
				osip_free(min_se_new->hvalue);
				min_se_new->hvalue = osip_strdup(min_se_h->element);
			} else {
				osip_message_set_header(msg, "Min-SE", min_se_h->element);
			}
		} else {
			osip_message_free(msg);
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: missing Min-SE or Session-Expires in dialog\n"));
			return OSIP_SYNTAXERROR;
		}

		osip_content_disposition_free(exp_h);
		osip_content_disposition_free(min_se_h);

	} else {
		osip_header_t *exp;

		osip_message_header_get_byname(msg, "session-expires", 0, &exp);
		if (exp == NULL) {
			osip_message_header_get_byname(msg, "x", 0, &exp);
		}
		if (exp == NULL) {
			/* add missing one? */
		}
	}

	if (out_tr->last_response->status_code == 401
		|| out_tr->last_response->status_code == 407)
		eXosip_add_authentication_information(msg, out_tr->last_response);
	else
		eXosip_add_authentication_information(msg, NULL);
	osip_message_force_update(msg);

	if (0 != osip_strcasecmp(msg->sip_method, "INVITE")) {
		i = _eXosip_transaction_init(&tr, NICT, eXosip.j_osip, msg);
	} else {
		i = _eXosip_transaction_init(&tr, ICT, eXosip.j_osip, msg);
	}

	if (i != 0) {
		osip_message_free(msg);
		return i;
	}

	if (out_tr == jc->c_out_tr) {
		/* replace with the new tr */
		osip_list_add(&eXosip.j_transactions, jc->c_out_tr, 0);
		jc->c_out_tr = tr;

		/* fix dialog issue */
		if (jd != NULL) {
			REMOVE_ELEMENT(jc->c_dialogs, jd);
			eXosip_dialog_free(jd);
			jd = NULL;
		}
	} else {
		/* add the new tr for the current dialog */
		osip_list_add(jd->d_out_trs, tr, 0);
	}

	sipevent = osip_new_outgoing_sipmessage(msg);

#ifndef MINISIZE
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(tr, __eXosip_new_jinfo(jc, jd));
#endif
	osip_transaction_add_event(tr, sipevent);

	eXosip_update();			/* fixed? */
	__eXosip_wakeup();
	return OSIP_SUCCESS;
}

#ifndef MINISIZE

int eXosip_call_get_referto(int did, char *refer_to, size_t refer_to_len)
{
	eXosip_dialog_t *jd = NULL;
	eXosip_call_t *jc = NULL;
	osip_transaction_t *tr = NULL;
	osip_uri_t *referto_uri;
	char atmp[256];
	char *referto_tmp = NULL;
	int i;

	if (did <= 0)
		return OSIP_BADPARAMETER;

	eXosip_call_dialog_find(did, &jc, &jd);
	if (jc == NULL || jd == NULL || jd->d_dialog == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No call here?\n"));
		return OSIP_NOTFOUND;
	}

	tr = eXosip_find_last_invite(jc, jd);

	if (tr == NULL || tr->orig_request == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: No transaction for call?\n"));
		return OSIP_NOTFOUND;
	}

	i = osip_uri_clone(jd->d_dialog->remote_uri->url, &referto_uri);
	if (i != 0)
		return i;

	snprintf(atmp, sizeof(atmp), "%s;to-tag=%s;from-tag=%s",
			 jd->d_dialog->call_id,
			 jd->d_dialog->remote_tag, jd->d_dialog->local_tag);

	osip_uri_uheader_add(referto_uri, osip_strdup("Replaces"), osip_strdup(atmp));
	i = osip_uri_to_str(referto_uri, &referto_tmp);
	if (i != 0) {
		osip_uri_free(referto_uri);
		return i;
	}

	snprintf(refer_to, refer_to_len, "%s", referto_tmp);
	osip_uri_free(referto_uri);

	return OSIP_SUCCESS;
}

int eXosip_call_find_by_replaces(char *replaces_str)
{
	eXosip_call_t *jc = NULL;
	eXosip_dialog_t *jd = NULL;
	char *call_id;
	char *to_tag;
	char *from_tag;
	char *early_flag;
	char *semicolon;
	char *totag_str = (char *) "to-tag=";
	char *fromtag_str = (char *) "from-tag=";
	char *earlyonly_str = (char *) "early-only";

	/* copy replaces string */
	if (replaces_str == NULL)
		return OSIP_SYNTAXERROR;
	call_id = osip_strdup(replaces_str);
	if (call_id == NULL)
		return OSIP_NOMEM;

	/* parse replaces string */
	to_tag = strstr(call_id, totag_str);
	from_tag = strstr(call_id, fromtag_str);
	early_flag = strstr(call_id, earlyonly_str);

	if ((to_tag == NULL) || (from_tag == NULL)) {
		osip_free(call_id);
		return OSIP_SYNTAXERROR;
	}
	to_tag += strlen(totag_str);
	from_tag += strlen(fromtag_str);

	while ((semicolon = strrchr(call_id, ';')) != NULL) {
		*semicolon = '\0';
	}

	for (jc = eXosip.j_calls; jc != NULL; jc = jc->next) {
		for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
			if (jd->d_dialog == NULL) {
				/* skip */
			} else if (((0 == strcmp(jd->d_dialog->call_id, call_id)) &&
						(0 == strcmp(jd->d_dialog->remote_tag, to_tag)) &&
						(0 == strcmp(jd->d_dialog->local_tag, from_tag)))
					   ||
					   ((0 == strcmp(jd->d_dialog->call_id, call_id)) &&
						(0 == strcmp(jd->d_dialog->local_tag, to_tag)) &&
						(0 == strcmp(jd->d_dialog->remote_tag, from_tag)))) {
				/* This dialog match! */
				if (jd->d_dialog->state == DIALOG_CONFIRMED && early_flag != NULL) {
					osip_free(call_id);
					return OSIP_WRONG_STATE;	/* confirmed dialog but already answered with 486 */
				}
				if (jd->d_dialog->state == DIALOG_EARLY
					&& jd->d_dialog->type == CALLEE) {
					osip_free(call_id);
					return OSIP_BADPARAMETER;	/* confirmed dialog but already answered with 481 */
				}

				osip_free(call_id);
				return jc->c_id;	/* match anyway */
			}
		}
	}
	osip_free(call_id);
	return OSIP_NOTFOUND;		/* answer with 481 */
}

#endif
