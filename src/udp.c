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
#include <eXosip2/eXosip.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __APPLE_CC__
#include <unistd.h>
#endif
#else
#include <windows.h>
#endif

extern eXosip_t eXosip;

/* Private functions */
static void eXosip_send_default_answer(eXosip_dialog_t * jd,
									   osip_transaction_t * transaction,
									   osip_event_t * evt,
									   int status,
									   char *reason_phrase,
									   char *warning, int line);
static void eXosip_process_bye(eXosip_call_t * jc, eXosip_dialog_t * jd,
							   osip_transaction_t * transaction,
							   osip_event_t * evt);
static void eXosip_process_ack(eXosip_call_t * jc, eXosip_dialog_t * jd,
							   osip_event_t * evt);
static void eXosip_process_prack(eXosip_call_t * jc, eXosip_dialog_t * jd,
								 osip_transaction_t * transaction,
								 osip_event_t * evt);
static int cancel_match_invite(osip_transaction_t * invite,
							   osip_message_t * cancel);
static void eXosip_process_cancel(osip_transaction_t * transaction,
								  osip_event_t * evt);
static void eXosip_process_reinvite(eXosip_call_t * jc,
									eXosip_dialog_t * jd,
									osip_transaction_t *
									transaction, osip_event_t * evt);
static void eXosip_process_new_invite(osip_transaction_t * transaction,
									  osip_event_t * evt);
#ifndef MINISIZE
static void eXosip_process_new_subscribe(osip_transaction_t * transaction,
										 osip_event_t * evt);
static void eXosip_process_subscribe_within_call(eXosip_notify_t * jn,
												 eXosip_dialog_t * jd,
												 osip_transaction_t *
												 transaction, osip_event_t * evt);
static void eXosip_process_notify_within_dialog(eXosip_subscribe_t * js,
												eXosip_dialog_t * jd,
												osip_transaction_t *
												transaction, osip_event_t * evt);
static int eXosip_match_notify_for_subscribe(eXosip_subscribe_t * js,
											 osip_message_t * notify);
#endif
static void eXosip_process_message_within_dialog(eXosip_call_t * jc,
												 eXosip_dialog_t * jd,
												 osip_transaction_t *
												 transaction, osip_event_t * evt);
static void eXosip_process_newrequest(osip_event_t * evt, int socket);
static void eXosip_process_response_out_of_transaction(osip_event_t * evt);
static int eXosip_pendingosip_transaction_exist(eXosip_call_t * jc,
												eXosip_dialog_t * jd);
static int eXosip_release_finished_calls(eXosip_call_t * jc, eXosip_dialog_t * jd);
static int eXosip_release_aborted_calls(eXosip_call_t * jc, eXosip_dialog_t * jd);

static int eXosip_release_finished_transactions(eXosip_call_t * jc,
												eXosip_dialog_t * jd);
#ifndef MINISIZE
static int eXosip_release_finished_transactions_for_subscription(eXosip_dialog_t *
																 jd);
#endif

static void
eXosip_send_default_answer(eXosip_dialog_t * jd,
						   osip_transaction_t * transaction,
						   osip_event_t * evt,
						   int status,
						   char *reason_phrase, char *warning, int line)
{
	osip_event_t *evt_answer;
	osip_message_t *answer;
	int i;

	/* osip_list_add(&eXosip.j_transactions, transaction, 0); */
	osip_transaction_set_your_instance(transaction, NULL);

	/* THIS METHOD DOES NOT ACCEPT STATUS CODE BETWEEN 101 and 299 */
	if (status > 100 && status < 299 && MSG_IS_INVITE(evt->sip))
		return;

	if (jd != NULL)
		i = _eXosip_build_response_default(&answer, jd->d_dialog, status,
										   evt->sip);
	else
		i = _eXosip_build_response_default(&answer, NULL, status, evt->sip);

	if (i != 0 || answer == NULL) {
		return;
	}

	if (reason_phrase != NULL) {
		char *_reason;

		_reason = osip_message_get_reason_phrase(answer);
		if (_reason != NULL)
			osip_free(_reason);
		_reason = osip_strdup(reason_phrase);
		osip_message_set_reason_phrase(answer, _reason);
	}

	osip_message_set_content_length(answer, "0");

	if (status == 500)
		osip_message_set_retry_after(answer, "10");

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = transaction->transactionid;
	osip_transaction_add_event(transaction, evt_answer);
	__eXosip_wakeup();

}

static void
eXosip_process_bye(eXosip_call_t * jc, eXosip_dialog_t * jd,
				   osip_transaction_t * transaction, osip_event_t * evt)
{
	osip_event_t *evt_answer;
	osip_message_t *answer;
	int i;

#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, NULL /*jd */ ,
														  NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, NULL /*jd */ ));
#endif

	i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
	if (i != 0) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		return;
	}
	osip_message_set_content_length(answer, "0");

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = transaction->transactionid;

	osip_list_add(jd->d_inc_trs, transaction, 0);

	/* Release the eXosip_dialog */
	osip_dialog_free(jd->d_dialog);
	jd->d_dialog = NULL;

	osip_transaction_add_event(transaction, evt_answer);

	osip_nist_execute(eXosip.j_osip);
	report_call_event(EXOSIP_CALL_MESSAGE_NEW, jc, jd, transaction);
	report_call_event(EXOSIP_CALL_CLOSED, jc, jd, transaction);
	eXosip_update();			/* AMD 30/09/05 */

	__eXosip_wakeup();
}

static void
eXosip_process_ack(eXosip_call_t * jc, eXosip_dialog_t * jd, osip_event_t * evt)
{
	/* TODO: We should find the matching transaction for this ACK
	   and also add the ACK in the event. */
	eXosip_event_t *je;
	int i;


	je = eXosip_event_init_for_call(EXOSIP_CALL_ACK, jc, jd, NULL);
	if (je != NULL) {
		osip_transaction_t *tr;
		tr = eXosip_find_last_inc_invite(jc, jd);
		if (tr != NULL) {
			je->tid = tr->transactionid;
			/* fill request and answer */
			if (tr->orig_request != NULL) {
				i = osip_message_clone(tr->orig_request, &je->request);
				if (i != 0) {
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
										  "failed to clone request for event\n"));
				}
			}
			if (tr->last_response != NULL) {
				i = osip_message_clone(tr->last_response, &je->response);
				if (i != 0) {
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
										  "failed to clone response for event\n"));
				}
			}
		}

		i = osip_message_clone(evt->sip, &je->ack);
		if (i != 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "failed to clone ACK for event\n"));
		}
	}

	/* stop ACK retransmission, in case there is any */
	jd->d_count = 0;
	osip_message_free(jd->d_200Ok);
	jd->d_200Ok = NULL;

	if (je != NULL)
		report_event(je, NULL);

	osip_event_free(evt);
}

static void
eXosip_process_prack(eXosip_call_t * jc, eXosip_dialog_t * jd,
					 osip_transaction_t * transaction, osip_event_t * evt)
{
	osip_event_t *evt_answer;
	osip_message_t *answer;
	int i;

#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd));
#endif

	i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
	if (i != 0) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		return;
	}

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = transaction->transactionid;

	osip_list_add(jd->d_inc_trs, transaction, 0);

	osip_transaction_add_event(transaction, evt_answer);
	__eXosip_wakeup();
}

static int
cancel_match_invite(osip_transaction_t * invite, osip_message_t * cancel)
{
	osip_generic_param_t *br;
	osip_generic_param_t *br2;
	osip_via_t *via;

	osip_via_param_get_byname(invite->topvia, "branch", &br);
	via = osip_list_get(&cancel->vias, 0);
	if (via == NULL)
		return OSIP_SYNTAXERROR;	/* request without via??? */
	osip_via_param_get_byname(via, "branch", &br2);
	if (br != NULL && br2 == NULL)
		return OSIP_UNDEFINED_ERROR;
	if (br2 != NULL && br == NULL)
		return OSIP_UNDEFINED_ERROR;
	if (br2 != NULL && br != NULL) {	/* compliant UA  :) */
		if (br->gvalue != NULL && br2->gvalue != NULL &&
			0 == strcmp(br->gvalue, br2->gvalue))
			return OSIP_SUCCESS;
		return OSIP_UNDEFINED_ERROR;
	}
	/* old backward compatibility mechanism */
	if (0 != osip_call_id_match(invite->callid, cancel->call_id))
		return OSIP_UNDEFINED_ERROR;
	if (0 != osip_to_tag_match(invite->to, cancel->to))
		return OSIP_UNDEFINED_ERROR;
	if (0 != osip_from_tag_match(invite->from, cancel->from))
		return OSIP_UNDEFINED_ERROR;
	if (0 != osip_via_match(invite->topvia, via))
		return OSIP_UNDEFINED_ERROR;
	return OSIP_SUCCESS;
}

static void
eXosip_process_cancel(osip_transaction_t * transaction, osip_event_t * evt)
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
	for (jc = eXosip.j_calls; jc != NULL; jc = jc->next) {
		if (jc->c_inc_tr != NULL) {
			i = cancel_match_invite(jc->c_inc_tr, evt->sip);
			if (i == 0) {
				tr = jc->c_inc_tr;
				/* fixed */
				if (jc->c_dialogs != NULL)
					jd = jc->c_dialogs;
				break;
			}
		}
		tr = NULL;
		for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
			int pos = 0;

			while (!osip_list_eol(jd->d_inc_trs, pos)) {
				tr = osip_list_get(jd->d_inc_trs, pos);
				i = cancel_match_invite(tr, evt->sip);
				if (i == 0)
					break;
				tr = NULL;
				pos++;
			}
			if (tr!=NULL)
				break;
		}
		if (jd != NULL)
			break;				/* tr has just been found! */
	}

	if (tr == NULL) {			/* we didn't found the transaction to cancel */
		i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot cancel transaction.\n"));
			/*BUG fixed 32/12/2010
			  osip_list_add(&eXosip.j_transactions, tr, 0);
			  osip_transaction_set_your_instance(tr, NULL);
			  replaced with */
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			osip_transaction_set_your_instance(transaction, NULL);
			return;
		}
		osip_message_set_content_length(answer, "0");
		evt_answer = osip_new_outgoing_sipmessage(answer);
		evt_answer->transactionid = transaction->transactionid;
		osip_transaction_add_event(transaction, evt_answer);

		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		__eXosip_wakeup();
		return;
	}

	if (tr->state == IST_TERMINATED || tr->state == IST_CONFIRMED
		|| tr->state == IST_COMPLETED) {
		/* I can't find the status code in the rfc?
		   (I read I must answer 200? wich I found strange)
		   I probably misunderstood it... and prefer to send 481
		   as the transaction has been answered. */
		if (jd == NULL)
			i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
		else
			i = _eXosip_build_response_default(&answer, jd->d_dialog, 481,
											   evt->sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot cancel transaction.\n"));
			/*BUG fixed 32/12/2010
			  osip_list_add(&eXosip.j_transactions, tr, 0);
			  osip_transaction_set_your_instance(tr, NULL);
			  replaced with */
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			osip_transaction_set_your_instance(transaction, NULL);
			return;
		}
		osip_message_set_content_length(answer, "0");
		evt_answer = osip_new_outgoing_sipmessage(answer);
		evt_answer->transactionid = transaction->transactionid;
		osip_transaction_add_event(transaction, evt_answer);

		if (jd != NULL)
			osip_list_add(jd->d_inc_trs, transaction, 0);
		else
			osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		__eXosip_wakeup();

		return;
	}

	{
		if (jd == NULL)
			i = _eXosip_build_response_default(&answer, NULL, 200, evt->sip);
		else
			i = _eXosip_build_response_default(&answer, jd->d_dialog, 200,
											   evt->sip);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot cancel transaction.\n"));
			/*BUG fixed 32/12/2010
			  osip_list_add(&eXosip.j_transactions, tr, 0);
			  osip_transaction_set_your_instance(tr, NULL);
			  replaced with */
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			osip_transaction_set_your_instance(transaction, NULL);
			return;
		}
		osip_message_set_content_length(answer, "0");
		evt_answer = osip_new_outgoing_sipmessage(answer);
		evt_answer->transactionid = transaction->transactionid;
		osip_transaction_add_event(transaction, evt_answer);
		__eXosip_wakeup();

		if (jd != NULL)
			osip_list_add(jd->d_inc_trs, transaction, 0);
		else
			osip_list_add(&eXosip.j_transactions, transaction, 0);
#ifndef MINISIZE
		osip_transaction_set_your_instance(transaction,
										   __eXosip_new_jinfo(jc, jd ,
															  NULL, NULL));
#else
		osip_transaction_set_your_instance(transaction,
										   __eXosip_new_jinfo(jc, jd ));
#endif

		/* answer transaction to cancel */
		if (jd == NULL)
			i = _eXosip_build_response_default(&answer, NULL, 487,
											   tr->orig_request);
		else
			i = _eXosip_build_response_default(&answer, jd->d_dialog, 487,
											   tr->orig_request);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot cancel transaction.\n"));
			/*BUG fixed 32/12/2010
			  osip_list_add(&eXosip.j_transactions, tr, 0);
			  osip_transaction_set_your_instance(tr, NULL); */
			return;
		}
		osip_message_set_content_length(answer, "0");
		evt_answer = osip_new_outgoing_sipmessage(answer);
		evt_answer->transactionid = tr->transactionid;
		osip_transaction_add_event(tr, evt_answer);
		__eXosip_wakeup();
	}
}

static void
eXosip_process_reinvite(eXosip_call_t * jc, eXosip_dialog_t * jd,
						osip_transaction_t * transaction, osip_event_t * evt)
{
#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd));
#endif

	osip_list_add(jd->d_inc_trs, transaction, 0);
	osip_ist_execute(eXosip.j_osip);
	report_call_event(EXOSIP_CALL_REINVITE, jc, jd, transaction);
}

static void
eXosip_process_new_invite(osip_transaction_t * transaction, osip_event_t * evt)
{
	osip_event_t *evt_answer;
	int i;
	eXosip_call_t *jc;
	eXosip_dialog_t *jd;
	osip_message_t *answer;
	osip_generic_param_t *to_tag = NULL;
	if (evt->sip != NULL && evt->sip->to != NULL)
		osip_from_param_get_byname(evt->sip->to, "tag", &to_tag);

	if (to_tag != NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: Existing To-Tag in new INVITE -> reject with 481\n"));
		i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
		if (i == 0) {
			evt_answer = osip_new_outgoing_sipmessage(answer);
			evt_answer->transactionid = transaction->transactionid;
			eXosip_update();
			osip_transaction_add_event(transaction, evt_answer);
			return;
		}
		osip_message_set_content_length(answer, "0");
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		return;
	}

	eXosip_call_init(&jc);

	ADD_ELEMENT(eXosip.j_calls, jc);

	i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot create dialog."));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: Could not create response for invite\n"));
		return;
	}
	osip_message_set_content_length(answer, "0");
	i = complete_answer_that_establish_a_dialog(answer, evt->sip);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot complete answer!\n"));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		osip_message_free(answer);
		return;
	}

	i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot create dialog!\n"));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		osip_message_free(answer);
		return;
	}
	ADD_ELEMENT(jc->c_dialogs, jd);

#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd));
#endif

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = transaction->transactionid;

	eXosip_update();
	jc->c_inc_tr = transaction;
	osip_transaction_add_event(transaction, evt_answer);

	/* be sure the invite will be processed
	   before any API call on this dialog */
	osip_ist_execute(eXosip.j_osip);

	if (transaction->orig_request != NULL) {
		report_call_event(EXOSIP_CALL_INVITE, jc, jd, transaction);
	}

	__eXosip_wakeup();

}

#ifndef MINISIZE

static void
eXosip_process_new_subscribe(osip_transaction_t * transaction, osip_event_t * evt)
{
	osip_event_t *evt_answer;
	eXosip_notify_t *jn;
	eXosip_dialog_t *jd;
	osip_message_t *answer;
	int i;
	osip_generic_param_t *to_tag = NULL;

	if (evt->sip != NULL && evt->sip->to != NULL)
		osip_from_param_get_byname(evt->sip->to, "tag", &to_tag);

	if (to_tag != NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: Existing To-Tag in new SUBSCRIBE -> reject with 481\n"));
		i = _eXosip_build_response_default(&answer, NULL, 481, evt->sip);
		if (i == 0) {
			evt_answer = osip_new_outgoing_sipmessage(answer);
			evt_answer->transactionid = transaction->transactionid;
			eXosip_update();
			osip_message_set_content_length(answer, "0");
			osip_transaction_add_event(transaction, evt_answer);
		}
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		return;
	}

	i = eXosip_notify_init(&jn, evt->sip);
	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: missing contact or memory\n"));
		i = _eXosip_build_response_default(&answer, NULL, 400, evt->sip);
		if (i == 0) {
			evt_answer = osip_new_outgoing_sipmessage(answer);
			evt_answer->transactionid = transaction->transactionid;
			eXosip_update();
			osip_message_set_content_length(answer, "0");
			osip_transaction_add_event(transaction, evt_answer);
		}
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		osip_transaction_set_your_instance(transaction, NULL);
		return;
	}
	_eXosip_notify_set_refresh_interval(jn, evt->sip);

	i = _eXosip_build_response_default(&answer, NULL, 101, evt->sip);
	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "ERROR: Could not create response for invite\n"));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_notify_free(jn);
		return;
	}
	i = complete_answer_that_establish_a_dialog(answer, evt->sip);
	if (i != 0) {
		osip_message_free(answer);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot complete answer!\n"));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_notify_free(jn);
		return;
	}

	i = eXosip_dialog_init_as_uas(&jd, evt->sip, answer);
	if (i != 0) {
		osip_message_free(answer);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot create dialog!\n"));
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_notify_free(jn);
		return;
	}
	ADD_ELEMENT(jn->n_dialogs, jd);

	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(NULL, jd, NULL, jn));

	evt_answer = osip_new_outgoing_sipmessage(answer);
	evt_answer->transactionid = transaction->transactionid;
	osip_transaction_add_event(transaction, evt_answer);

	ADD_ELEMENT(eXosip.j_notifies, jn);
	__eXosip_wakeup();

	jn->n_inc_tr = transaction;

	eXosip_update();
	__eXosip_wakeup();
}

static void
eXosip_process_subscribe_within_call(eXosip_notify_t * jn,
									 eXosip_dialog_t * jd,
									 osip_transaction_t * transaction,
									 osip_event_t * evt)
{
	_eXosip_notify_set_refresh_interval(jn, evt->sip);
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(NULL, jd, NULL, jn));

	/* if subscribe request contains expires="0", close the subscription */
	{
		time_t now = time(NULL);

		if (jn->n_ss_expires - now <= 0) {
			jn->n_ss_status = EXOSIP_SUBCRSTATE_TERMINATED;
			jn->n_ss_reason = TIMEOUT;
		}
	}

	osip_list_add(jd->d_inc_trs, transaction, 0);
	__eXosip_wakeup();
	return;
}

static void
eXosip_process_notify_within_dialog(eXosip_subscribe_t * js,
									eXosip_dialog_t * jd,
									osip_transaction_t * transaction,
									osip_event_t * evt)
{
	osip_message_t *answer;
	osip_event_t *sipevent;
	osip_header_t *sub_state;

#ifdef SUPPORT_MSN
	osip_header_t *expires;
#endif
	int i;

	if (jd == NULL) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_send_default_answer(jd, transaction, evt, 500,
								   "Internal SIP Error",
								   "No dialog for this NOTIFY", __LINE__);
		return;
	}

	/* if subscription-state has a reason state set to terminated,
	   we close the dialog */
#ifndef SUPPORT_MSN
	osip_message_header_get_byname(evt->sip, "subscription-state", 0, &sub_state);
	if (sub_state == NULL || sub_state->hvalue == NULL) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_send_default_answer(jd, transaction, evt, 400, NULL, NULL,
								   __LINE__);
		return;
	}
#endif

	i = _eXosip_build_response_default(&answer, jd->d_dialog, 200, evt->sip);
	if (i != 0) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_send_default_answer(jd, transaction, evt, 500,
								   "Internal SIP Error",
								   "Failed to build Answer for NOTIFY", __LINE__);
		return;
	}
#ifdef SUPPORT_MSN
	osip_message_header_get_byname(evt->sip, "expires", 0, &expires);
	if (expires != NULL && expires->hvalue != NULL
		&& 0 == osip_strcasecmp(expires->hvalue, "0")) {
		/* delete the dialog! */
		js->s_ss_status = EXOSIP_SUBCRSTATE_TERMINATED;
		{
			eXosip_event_t *je;

			je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_NOTIFY, js,
												 jd);
			eXosip_event_add(je);
		}

		sipevent = osip_new_outgoing_sipmessage(answer);
		sipevent->transactionid = transaction->transactionid;
		osip_transaction_add_event(transaction, sipevent);

		osip_list_add(&eXosip.j_transactions, transaction, 0);

		REMOVE_ELEMENT(eXosip.j_subscribes, js);
		eXosip_subscribe_free(js);
		__eXosip_wakeup();

		return;
	} else {
		osip_transaction_set_your_instance(transaction,
										   __eXosip_new_jinfo(NULL, jd, js, NULL));
		js->s_ss_status = EXOSIP_SUBCRSTATE_ACTIVE;
	}
#else
	/* modify the status of user */
	if (0 == osip_strncasecmp(sub_state->hvalue, "active", 6)) {
		js->s_ss_status = EXOSIP_SUBCRSTATE_ACTIVE;
	} else if (0 == osip_strncasecmp(sub_state->hvalue, "pending", 7)) {
		js->s_ss_status = EXOSIP_SUBCRSTATE_PENDING;
	}

	if (0 == osip_strncasecmp(sub_state->hvalue, "terminated", 10)) {
		/* delete the dialog! */
		js->s_ss_status = EXOSIP_SUBCRSTATE_TERMINATED;

		{
			eXosip_event_t *je;

			je = eXosip_event_init_for_subscribe(EXOSIP_SUBSCRIPTION_NOTIFY, js,
												 jd, transaction);
			if (je->request == NULL && evt->sip != NULL) {
				i = osip_message_clone(evt->sip, &je->request);
				if (i != 0) {
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
										  "failed to clone request for event\n"));
				}
			}

			eXosip_event_add(je);
		}

		sipevent = osip_new_outgoing_sipmessage(answer);
		sipevent->transactionid = transaction->transactionid;
		osip_transaction_add_event(transaction, sipevent);

		osip_list_add(&eXosip.j_transactions, transaction, 0);

		REMOVE_ELEMENT(eXosip.j_subscribes, js);
		eXosip_subscribe_free(js);
		__eXosip_wakeup();
		return;
	} else {
		osip_transaction_set_your_instance(transaction,
										   __eXosip_new_jinfo(NULL, jd, js, NULL));
	}
#endif

	osip_list_add(jd->d_inc_trs, transaction, 0);

	sipevent = osip_new_outgoing_sipmessage(answer);
	sipevent->transactionid = transaction->transactionid;
	osip_transaction_add_event(transaction, sipevent);

	__eXosip_wakeup();
	return;
}

static int
eXosip_match_notify_for_subscribe(eXosip_subscribe_t * js, osip_message_t * notify)
{
	osip_transaction_t *out_sub;

	if (js == NULL)
		return OSIP_BADPARAMETER;
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "Trying to match notify with subscribe\n"));

	out_sub = eXosip_find_last_out_subscribe(js, NULL);
	if (out_sub == NULL || out_sub->orig_request == NULL)
		return OSIP_NOTFOUND;
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "subscribe transaction found\n"));

	/* some checks to avoid crashing on bad requests */
	if (notify == NULL)
		return OSIP_BADPARAMETER;

	if (notify->cseq == NULL || notify->cseq->method == NULL || notify->to == NULL)
		return OSIP_SYNTAXERROR;

	if (0 != osip_call_id_match(out_sub->callid, notify->call_id))
		return OSIP_UNDEFINED_ERROR;

	{
		/* The From tag of outgoing request must match
		   the To tag of incoming notify:
		 */
		osip_generic_param_t *tag_from;
		osip_generic_param_t *tag_to;

		osip_from_param_get_byname(out_sub->from, "tag", &tag_from);
		osip_from_param_get_byname(notify->to, "tag", &tag_to);
		if (tag_to == NULL || tag_to->gvalue == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "Uncompliant user agent: no tag in from of outgoing request\n"));
			return OSIP_SYNTAXERROR;
		}
		if (tag_from == NULL || tag_to->gvalue == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "Uncompliant user agent: no tag in to of incoming request\n"));
			return OSIP_SYNTAXERROR;
		}

		if (0 != strcmp(tag_from->gvalue, tag_to->gvalue))
			return OSIP_UNDEFINED_ERROR;
	}

	return OSIP_SUCCESS;
}

#endif


static void
eXosip_process_message_within_dialog(eXosip_call_t * jc,
									 eXosip_dialog_t * jd,
									 osip_transaction_t * transaction,
									 osip_event_t * evt)
{
	osip_list_add(jd->d_inc_trs, transaction, 0);
#ifndef MINISIZE
	osip_transaction_set_your_instance(transaction,
									   __eXosip_new_jinfo(jc, jd, NULL, NULL));
#else
	osip_transaction_set_your_instance(transaction, __eXosip_new_jinfo(jc, jd));
#endif
	__eXosip_wakeup();
	return;
}


static void eXosip_process_newrequest(osip_event_t * evt, int socket)
{
	osip_transaction_t *transaction;
#ifndef MINISIZE
	osip_event_t *evt_answer;
	osip_message_t *answer;
#endif
	int i;
	int ctx_type;
	eXosip_call_t *jc;
#ifndef MINISIZE
	eXosip_subscribe_t *js;
	eXosip_notify_t *jn;
#endif
	eXosip_dialog_t *jd;

	if (MSG_IS_INVITE(evt->sip)) {
		ctx_type = IST;
	} else if (MSG_IS_ACK(evt->sip)) {	/* this should be a ACK for 2xx (but could be a late ACK!) */
		ctx_type = -1;
	} else if (MSG_IS_REQUEST(evt->sip)) {
		ctx_type = NIST;
	} else {					/* We should handle late response and 200 OK before coming here. */
		ctx_type = -1;
		osip_event_free(evt);
		return;
	}

	transaction = NULL;
	if (ctx_type != -1) {
		i = _eXosip_transaction_init(&transaction,
									 (osip_fsm_type_t) ctx_type,
									 eXosip.j_osip, evt->sip);
		if (i != 0) {
			osip_event_free(evt);
			return;
		}

		osip_transaction_set_in_socket(transaction, socket);
		osip_transaction_set_out_socket(transaction, socket);

		evt->transactionid = transaction->transactionid;
		osip_transaction_set_your_instance(transaction, NULL);

		osip_transaction_add_event(transaction, evt);
	}

	if (MSG_IS_CANCEL(evt->sip)) {
		/* special handling for CANCEL */
		/* in the new spec, if the CANCEL has a Via branch, then it
		   is the same as the one in the original INVITE */
		eXosip_process_cancel(transaction, evt);
		return;
	}

	jd = NULL;
	/* first, look for a Dialog in the map of element */
	for (jc = eXosip.j_calls; jc != NULL; jc = jc->next) {
		for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
			if (jd->d_dialog != NULL) {
				if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip) == 0)
					break;
			}
		}
		if (jd != NULL)
			break;
	}

	/* check CSeq */
	if (jd != NULL && transaction != NULL && evt->sip != NULL
		&& evt->sip->cseq != NULL && evt->sip->cseq->number != NULL) {
		if (jd->d_dialog != NULL && jd->d_dialog->remote_cseq > 0) {
			int cseq = osip_atoi(evt->sip->cseq->number);
			if (cseq == jd->d_dialog->remote_cseq) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: receive a request with same cseq??\n"));
				_eXosip_dnsutils_release(transaction->naptr_record);
				transaction->naptr_record=NULL;
				osip_transaction_free(transaction);
				return;
			}
			if (cseq <= jd->d_dialog->remote_cseq) {
				eXosip_send_default_answer(jd, transaction, evt, 500,
										   NULL, "Wrong Lower CSeq", __LINE__);
				return;
			}
		}
	}
#ifndef MINISIZE
	if (ctx_type == IST) {
		i = _eXosip_build_response_default(&answer, NULL, 100, evt->sip);
		if (i != 0) {
			__eXosip_delete_jinfo(transaction);
			_eXosip_dnsutils_release(transaction->naptr_record);
			transaction->naptr_record=NULL;
			osip_transaction_free(transaction);
			return;
		}

		osip_message_set_content_length(answer, "0");
		/*  send message to transaction layer */

		evt_answer = osip_new_outgoing_sipmessage(answer);
		evt_answer->transactionid = transaction->transactionid;

		/* add the REQUEST & the 100 Trying */
		osip_transaction_add_event(transaction, evt_answer);
		__eXosip_wakeup();
	}
#endif

	if (jd != NULL) {
		osip_transaction_t *old_trn;

		/* it can be:
		   1: a new INVITE offer.
		   2: a REFER request from one of the party.
		   2: a BYE request from one of the party.
		   3: a REQUEST with a wrong CSeq.
		   4: a NOT-SUPPORTED method with a wrong CSeq.
		 */

		if (transaction == NULL) {
			/* cannot answer ACK transaction */
		} else if (!MSG_IS_BYE(evt->sip)) {
			/* reject all requests for a closed dialog */
			old_trn = eXosip_find_last_inc_transaction(jc, jd, "BYE");
			if (old_trn == NULL)
				old_trn = eXosip_find_last_out_transaction(jc, jd, "BYE");

			if (old_trn != NULL) {
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 481, NULL,
										   NULL, __LINE__);
				return;
			}
		}

		if (transaction != NULL)	/* NOT for ACK */
			osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);

		if (MSG_IS_INVITE(evt->sip)) {
			/* the previous transaction MUST be freed */
			old_trn = eXosip_find_last_inc_invite(jc, jd);

			if (old_trn != NULL && old_trn->state != IST_COMPLETED
				&& old_trn->state != IST_CONFIRMED
				&& old_trn->state != IST_TERMINATED) {
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 500,
										   "Retry Later",
										   "An INVITE is not terminated",
										   __LINE__);
				return;
			}

			old_trn = eXosip_find_last_out_invite(jc, jd);
			if (old_trn != NULL && old_trn->state != ICT_COMPLETED
				&& old_trn->state != ICT_TERMINATED) {
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 491, NULL,
										   NULL, __LINE__);
				return;
			}

			/* osip_dialog_update_osip_cseq_as_uas (jd->d_dialog, evt->sip); */
			osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

			eXosip_process_reinvite(jc, jd, transaction, evt);
		} else if (MSG_IS_BYE(evt->sip)) {
			osip_generic_param_t *tag_to = NULL;
			if (evt->sip->to != NULL)
				osip_from_param_get_byname(evt->sip->to, "tag", &tag_to);
			if (tag_to == NULL || tag_to->gvalue == NULL) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "Uncompliant user agent: missing a tag in To of incoming BYE\n"));
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 481,
										   "Missing tags in BYE",
										   "Missing tags in BYE", __LINE__);
				return;
			}

			old_trn = eXosip_find_last_inc_transaction(jc, jd, "BYE");

			if (old_trn != NULL) {	/* && old_trn->state!=NIST_TERMINATED) *//* this situation should NEVER occur?? (we can't receive
									   two different BYE for one call! */
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 500,
										   "Call Already Terminated",
										   "A pending BYE has already terminate this call",
										   __LINE__);
				return;
			}
			eXosip_process_bye(jc, jd, transaction, evt);
		} else if (MSG_IS_ACK(evt->sip)) {
			eXosip_process_ack(jc, jd, evt);
		} else {
			eXosip_process_message_within_dialog(jc, jd, transaction, evt);
		}
		return;
	}

	if (MSG_IS_ACK(evt->sip)) {
		/* no transaction has been found for this ACK! */
		osip_event_free(evt);
		return;
	}

	if (MSG_IS_INFO(evt->sip)) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_send_default_answer(jd, transaction, evt, 481, NULL, NULL,
								   __LINE__);
		return;					/* fixed */
	}
	if (MSG_IS_INVITE(evt->sip)) {
		eXosip_process_new_invite(transaction, evt);
		return;
	} else if (MSG_IS_BYE(evt->sip)) {
		osip_list_add(&eXosip.j_transactions, transaction, 0);
		eXosip_send_default_answer(jd, transaction, evt, 481, NULL, NULL,
								   __LINE__);
		return;
	}
#ifndef MINISIZE
	js = NULL;
	/* first, look for a Dialog in the map of element */
	for (js = eXosip.j_subscribes; js != NULL; js = js->next) {
		for (jd = js->s_dialogs; jd != NULL; jd = jd->next) {
			if (jd->d_dialog != NULL) {
				if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip) == 0)
					break;
			}
		}
		if (jd != NULL)
			break;
	}

	if (js != NULL) {
		/* dialog found */
		osip_transaction_t *old_trn;

		/* it can be:
		   1: a new INVITE offer.
		   2: a REFER request from one of the party.
		   2: a BYE request from one of the party.
		   3: a REQUEST with a wrong CSeq.
		   4: a NOT-SUPPORTED method with a wrong CSeq.
		 */
		if (MSG_IS_MESSAGE(evt->sip)) {
			/* eXosip_process_imessage_within_subscribe_dialog(transaction, evt); */
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			eXosip_send_default_answer(jd, transaction, evt,
									   SIP_NOT_IMPLEMENTED, NULL,
									   "MESSAGEs within dialogs are not implemented.",
									   __LINE__);
			return;
		} else if (MSG_IS_NOTIFY(evt->sip)) {
			/* the previous transaction MUST be freed */
			old_trn = eXosip_find_last_inc_notify(js, jd);

			/* shouldn't we wait for the COMPLETED state? */
			if (old_trn != NULL && old_trn->state != NIST_TERMINATED) {
				/* retry later? */
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 500,
										   "Retry Later",
										   "A pending NOTIFY is not terminated",
										   __LINE__);
				return;
			}

			osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
			osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

			eXosip_process_notify_within_dialog(js, jd, transaction, evt);
		} else {
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			eXosip_send_default_answer(jd, transaction, evt, 501, NULL,
									   "Just Not Implemented", __LINE__);
		}
		return;
	}

	if (MSG_IS_NOTIFY(evt->sip)) {
		/* let's try to check if the NOTIFY is related to an existing
		   subscribe */
		js = NULL;
		/* first, look for a Dialog in the map of element */
		for (js = eXosip.j_subscribes; js != NULL; js = js->next) {
			if (eXosip_match_notify_for_subscribe(js, evt->sip) == 0) {
				i = eXosip_dialog_init_as_uac(&jd, evt->sip);
				if (i != 0) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"eXosip: cannot establish a dialog\n"));
					return;
				}

				/* update local cseq from subscribe request */
				if (js->s_out_tr != NULL && js->s_out_tr->cseq != NULL
					&& js->s_out_tr->cseq->number != NULL) {
					jd->d_dialog->local_cseq = atoi(js->s_out_tr->cseq->number);
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"eXosip: local cseq has been updated\n"));
				}

				ADD_ELEMENT(js->s_dialogs, jd);
				eXosip_update();

				eXosip_process_notify_within_dialog(js, jd, transaction, evt);
				return;
			}
		}

		osip_list_add(&eXosip.j_transactions, transaction, 0);
		return;
	}

	jn = NULL;
	/* first, look for a Dialog in the map of element */
	for (jn = eXosip.j_notifies; jn != NULL; jn = jn->next) {
		for (jd = jn->n_dialogs; jd != NULL; jd = jd->next) {
			if (jd->d_dialog != NULL) {
				if (osip_dialog_match_as_uas(jd->d_dialog, evt->sip) == 0)
					break;
			}
		}
		if (jd != NULL)
			break;
	}

	if (jn != NULL) {
		/* dialog found */
		osip_transaction_t *old_trn;

		/* it can be:
		   1: a new INVITE offer.
		   2: a REFER request from one of the party.
		   2: a BYE request from one of the party.
		   3: a REQUEST with a wrong CSeq.
		   4: a NOT-SUPPORTED method with a wrong CSeq.
		 */
		if (MSG_IS_MESSAGE(evt->sip)) {
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			eXosip_send_default_answer(jd, transaction, evt,
									   SIP_NOT_IMPLEMENTED, NULL,
									   "MESSAGEs within dialogs are not implemented.",
									   __LINE__);
			return;
		} else if (MSG_IS_SUBSCRIBE(evt->sip)) {
			/* the previous transaction MUST be freed */
			old_trn = eXosip_find_last_inc_subscribe(jn, jd);

			/* shouldn't we wait for the COMPLETED state? */
			if (old_trn != NULL && old_trn->state != NIST_TERMINATED
				&& old_trn->state != NIST_COMPLETED) {
				/* retry later? */
				osip_list_add(&eXosip.j_transactions, transaction, 0);
				eXosip_send_default_answer(jd, transaction, evt, 500,
										   "Retry Later",
										   "A SUBSCRIBE is not terminated",
										   __LINE__);
				return;
			}

			osip_dialog_update_osip_cseq_as_uas(jd->d_dialog, evt->sip);
			osip_dialog_update_route_set_as_uas(jd->d_dialog, evt->sip);

			eXosip_process_subscribe_within_call(jn, jd, transaction, evt);
		} else {
			osip_list_add(&eXosip.j_transactions, transaction, 0);
			eXosip_send_default_answer(jd, transaction, evt, 501, NULL, NULL,
									   __LINE__);
		}
		return;
	}
#endif

#ifndef MINISIZE
	if (MSG_IS_SUBSCRIBE(evt->sip)) {
		eXosip_process_new_subscribe(transaction, evt);
		return;
	}
#endif

	/* default answer */
	osip_list_add(&eXosip.j_transactions, transaction, 0);
	__eXosip_wakeup();			/* needed? */
}

static void eXosip_process_response_out_of_transaction(osip_event_t * evt)
{
	eXosip_call_t *jc = NULL;
	eXosip_dialog_t *jd = NULL;
	time_t now;

	now = time(NULL);
	if (evt->sip == NULL
		|| evt->sip->cseq == NULL
		|| evt->sip->cseq->number == NULL
		|| evt->sip->to == NULL || evt->sip->from == NULL) {
		osip_event_free(evt);
		return;
	}

	if (!MSG_IS_RESPONSE_FOR(evt->sip, "INVITE"))
	{
		osip_event_free(evt);
		return;
	}

	/* search for existing dialog: match branch & to tag */
	for (jc = eXosip.j_calls; jc != NULL; jc = jc->next) {
		/* search for calls with only ONE outgoing transaction */
		if (jc->c_id >= 1 && jc->c_dialogs != NULL && jc->c_out_tr != NULL) {
			for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
				if (jd->d_id >= 1 && jd->d_dialog != NULL) {
					/* match answer with dialog */
					osip_generic_param_t *tag;

					osip_from_get_tag(evt->sip->to, &tag);

					if (jd->d_dialog->remote_tag == NULL || tag == NULL)
						continue;
					if (jd->d_dialog->remote_tag != NULL && tag != NULL
						&& tag->gvalue != NULL
						&& 0 == strcmp(jd->d_dialog->remote_tag, tag->gvalue))
						break;
				}
			}
			if (jd != NULL)
				break;			/* found a matching dialog! */

			/* check if the transaction match this from tag */
			if (jc->c_out_tr->orig_request != NULL
				&& jc->c_out_tr->orig_request->from != NULL) {
				osip_generic_param_t *tag_invite;
				osip_generic_param_t *tag;
				osip_from_get_tag(jc->c_out_tr->orig_request->from, &tag_invite);
				osip_from_get_tag(evt->sip->from, &tag);

				if (tag_invite == NULL || tag == NULL)
					continue;
				if (tag_invite->gvalue != NULL && tag->gvalue != NULL
					&& 0 == strcmp(tag_invite->gvalue, tag->gvalue))
					break;
			}
		}
	}

	if (jc == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"Incoming 2xx has no relations with current calls: Message discarded.\r\n"));
		osip_event_free(evt);
		return;
	}
#ifndef MINISIZE
	if (jc != NULL && jd != NULL) {
		/* we have to restransmit the ACK (if already available) */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"2xx restransmission receveid.\r\n"));
		/* check if the 2xx is for the same ACK */
		if (jd->d_ack != NULL && jd->d_ack->cseq != NULL
			&& jd->d_ack->cseq->number != NULL) {
			if (0 ==
				osip_strcasecmp(jd->d_ack->cseq->number, evt->sip->cseq->number)) {
				cb_snd_message(NULL, jd->d_ack, NULL, 0, -1);
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"ACK restransmission sent.\r\n"));
			}
		}

		osip_event_free(evt);
		return;
	}
#endif

	if (jc != NULL) {
		/* match answer with dialog */
		osip_dialog_t *dlg;
#ifndef MINISIZE
		osip_transaction_t *last_tr;
#endif
		int i;

		/* we match an existing dialog: send a retransmission of ACK */
		i = osip_dialog_init_as_uac(&dlg, evt->sip);
		if (i != 0 || dlg == NULL) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"Cannot build dialog for 200ok.\r\n"));
			osip_event_free(evt);
			return;
		}

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"sending ACK for 2xx out of transaction.\r\n"));

		{
			osip_message_t *bye = NULL;
			char *transport = _eXosip_transport_protocol(evt->sip);
#ifndef MINISIZE				/* Don't send ACK in MINISIZE mode to save code size */
			osip_message_t *ack;
			if (transport == NULL)
				i = _eXosip_build_request_within_dialog(&ack, "ACK", dlg, "UDP");
			else
				i = _eXosip_build_request_within_dialog(&ack, "ACK", dlg,
														transport);
			if (i != 0) {
				osip_dialog_free(dlg);
				osip_event_free(evt);
				return;
			}
			/* copy all credentials from INVITE! */
			last_tr = jc->c_out_tr;
			if (last_tr != NULL) {
				int pos = 0;
				int i;
				osip_proxy_authorization_t *pa = NULL;

				i = osip_message_get_proxy_authorization(last_tr->orig_request,
														 pos, &pa);
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
					i = osip_message_get_proxy_authorization(last_tr->orig_request,
															 pos, &pa);
				}
			}
			cb_snd_message(NULL, ack, NULL, 0, -1);
			osip_message_free(ack);
#endif
			/* in some case, PRACK and UPDATE may have been sent
			   so we have to send a cseq which is above the previous
			   one. */
			dlg->local_cseq = dlg->local_cseq + 4;

			/* ready to send a BYE */
			if (transport == NULL)
				i = generating_bye(&bye, dlg, "UDP");
			else
				i = generating_bye(&bye, dlg, transport);
			if (bye != NULL && i == OSIP_SUCCESS)
				cb_snd_message(NULL, bye, NULL, 0, -1);
			osip_message_free(bye);
		}

		osip_dialog_free(dlg);
		osip_event_free(evt);
		return;
	}

	/* ...code not reachable... */
}

int
_eXosip_handle_incoming_message(char *buf, size_t length, int socket,
								char *host, int port)
{
	int i;
	osip_event_t *se;

	se = (osip_event_t *) osip_malloc(sizeof(osip_event_t));
	if (se == NULL)
		return OSIP_NOMEM;
	se->type = UNKNOWN_EVT;
	se->sip = NULL;
	se->transactionid = 0;

	/* parse message and set up an event */
	i = osip_message_init(&(se->sip));
	if (i != 0) {
		osip_free(se);
		return i;
	}
	i = osip_message_parse(se->sip, buf, length);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"could not parse message\n"));
		osip_message_free(se->sip);
		osip_free(se);
		return i;
	}

	if (se->sip->call_id != NULL && se->sip->call_id->number != NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO3, NULL,
					"MESSAGE REC. CALLID:%s\n", se->sip->call_id->number));
	}

	if (eXosip.cbsipCallback != NULL) {
		eXosip.cbsipCallback(se->sip, 1);
	}

	if (MSG_IS_REQUEST(se->sip)) {
		if (se->sip->sip_method == NULL || se->sip->req_uri == NULL) {
			osip_message_free(se->sip);
			osip_free(se);
			return OSIP_SYNTAXERROR;
		}
	}

	if (MSG_IS_REQUEST(se->sip)) {
		if (MSG_IS_INVITE(se->sip))
			se->type = RCV_REQINVITE;
		else if (MSG_IS_ACK(se->sip))
			se->type = RCV_REQACK;
		else
			se->type = RCV_REQUEST;
	} else {
		if (MSG_IS_STATUS_1XX(se->sip))
			se->type = RCV_STATUS_1XX;
		else if (MSG_IS_STATUS_2XX(se->sip))
			se->type = RCV_STATUS_2XX;
		else
			se->type = RCV_STATUS_3456XX;
	}

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL,
				"Message received from: %s:%i\n", host, port));

	osip_message_fix_last_via_header(se->sip, host, port);

	i = osip_find_transaction_and_add_event(eXosip.j_osip, se);
	if (i != 0) {
		/* this event has no transaction, */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"This is a request\n", buf));
		eXosip_lock();
		if (MSG_IS_REQUEST(se->sip))
			eXosip_process_newrequest(se, socket);
		else if (MSG_IS_RESPONSE(se->sip))
			eXosip_process_response_out_of_transaction(se);
		eXosip_unlock();
	} else {
		/* handled by oSIP ! */
		return OSIP_SUCCESS;
	}
	return OSIP_SUCCESS;
}

#if defined (WIN32) || defined (_WIN32_WCE)
#define eXFD_SET(A, B)   FD_SET((unsigned int) A, B)
#else
#define eXFD_SET(A, B)   FD_SET(A, B)
#endif

/* if second==-1 && useconds==-1  -> wait for ever
   if max_message_nb<=0  -> infinite loop....  */
int eXosip_read_message(int max_message_nb, int sec_max, int usec_max)
{
	fd_set osip_fdset;
	fd_set osip_wrset;
	struct timeval tv;

	tv.tv_sec = sec_max;
	tv.tv_usec = usec_max;

	while (max_message_nb != 0 && eXosip.j_stop_ua == 0) {
		int i;
		int max = 0;
#ifdef OSIP_MT
		int wakeup_socket = jpipe_get_read_descr(eXosip.j_socketctl);
#endif

		FD_ZERO(&osip_fdset);
		FD_ZERO(&osip_wrset);
		eXtl_udp.tl_set_fdset(&osip_fdset, &osip_wrset, &max);
		eXtl_tcp.tl_set_fdset(&osip_fdset, &osip_wrset, &max);
#ifdef HAVE_OPENSSL_SSL_H
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
		eXtl_dtls.tl_set_fdset(&osip_fdset, &osip_wrset, &max);
#endif
		eXtl_tls.tl_set_fdset(&osip_fdset, &osip_wrset, &max);
#endif

#ifdef OSIP_MT
		eXFD_SET(wakeup_socket, &osip_fdset);
		if (wakeup_socket > max)
			max = wakeup_socket;
#endif

		if ((sec_max == -1) || (usec_max == -1))
			i = select(max + 1, &osip_fdset, NULL, NULL, NULL);
		else
			i = select(max + 1, &osip_fdset, NULL, NULL, &tv);

#if defined (_WIN32_WCE)
		/* TODO: fix me for wince */
		/* if (i == -1)
		   continue; */
#else
		if ((i == -1) && (errno == EINTR || errno == EAGAIN))
			continue;
#endif
#ifdef OSIP_MT
		if ((i > 0) && FD_ISSET(wakeup_socket, &osip_fdset)) {
			char buf2[500];

			jpipe_read(eXosip.j_socketctl, buf2, 499);
		}
#endif

		if (0 == i || eXosip.j_stop_ua != 0) {
		} else if (-1 == i) {
#if !defined (_WIN32_WCE)		/* TODO: fix me for wince */
			return -2000;		/* error */
#endif
		} else {
			eXtl_udp.tl_read_message(&osip_fdset, &osip_wrset);
			eXtl_tcp.tl_read_message(&osip_fdset, &osip_wrset);
#ifdef HAVE_OPENSSL_SSL_H
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
			eXtl_dtls.tl_read_message(&osip_fdset, &osip_wrset);
#endif
			eXtl_tls.tl_read_message(&osip_fdset, &osip_wrset);
#endif
		}

		max_message_nb--;
	}
	return OSIP_SUCCESS;
}

static int
eXosip_pendingosip_transaction_exist(eXosip_call_t * jc, eXosip_dialog_t * jd)
{
	osip_transaction_t *tr;
	time_t now = time(NULL);

	tr = eXosip_find_last_inc_transaction(jc, jd, "BYE");
	if (tr != NULL && tr->state != NIST_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
			/* remove the transaction from oSIP: */
			osip_remove_transaction(eXosip.j_osip, tr);
			eXosip_remove_transaction_from_call(tr, jc);
			osip_list_add(&eXosip.j_transactions, tr, 0);
		} else
			return OSIP_SUCCESS;
	}

	tr = eXosip_find_last_out_transaction(jc, jd, "BYE");
	if (tr != NULL && tr->state != NICT_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
			/* remove the transaction from oSIP: */
			osip_remove_transaction(eXosip.j_osip, tr);
			eXosip_remove_transaction_from_call(tr, jc);
			osip_list_add(&eXosip.j_transactions, tr, 0);
		} else
			return OSIP_SUCCESS;
	}

	tr = eXosip_find_last_inc_invite(jc, jd);
	if (tr != NULL && tr->state != IST_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
		} else
			return OSIP_SUCCESS;
	}

	tr = eXosip_find_last_out_invite(jc, jd);
	if (tr != NULL && tr->state != ICT_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (jc->expire_time < now) {
		} else
			return OSIP_SUCCESS;
	}

	tr = eXosip_find_last_inc_transaction(jc, jd, "REFER");
	if (tr != NULL && tr->state != NIST_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
			/* remove the transaction from oSIP: */
			osip_remove_transaction(eXosip.j_osip, tr);
			eXosip_remove_transaction_from_call(tr, jc);
			osip_list_add(&eXosip.j_transactions, tr, 0);
		} else
			return OSIP_SUCCESS;
	}

	tr = eXosip_find_last_out_transaction(jc, jd, "REFER");
	if (tr != NULL && tr->state != NICT_TERMINATED) {	/* Don't want to wait forever on broken transaction!! */
		if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
			/* remove the transaction from oSIP: */
			osip_remove_transaction(eXosip.j_osip, tr);
			eXosip_remove_transaction_from_call(tr, jc);
			osip_list_add(&eXosip.j_transactions, tr, 0);
		} else
			return OSIP_SUCCESS;
	}

	return OSIP_UNDEFINED_ERROR;
}

static int
eXosip_release_finished_transactions(eXosip_call_t * jc, eXosip_dialog_t * jd)
{
	time_t now = time(NULL);
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;
	osip_transaction_t *last_invite;
	int pos;
	int ret;

	ret = -1;

	last_invite = eXosip_find_last_inc_invite(jc, jd);

	if (jd != NULL) {
		/* go through all incoming transactions of this dialog */
		pos = 1;
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			if (0 != osip_strcasecmp(inc_tr->cseq->method, "INVITE")) {
				/* remove, if transaction too old, independent of the state */
				if ((inc_tr->state == NIST_TERMINATED) && (inc_tr->birth_time + 30 < now)) {	/* Wait a max of 30 seconds */
					/* remove the transaction from oSIP */
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
										  "eXosip: release non-INVITE server transaction (did=%i)\n",
										  jd->d_id));
					osip_remove_transaction(eXosip.j_osip, inc_tr);
					osip_list_remove(jd->d_inc_trs, pos);
					osip_list_add(&eXosip.j_transactions, inc_tr, 0);

					ret = 0;
					break;
				}
			} else {
				/* remove, if transaction too old, independent of the state */
				if (last_invite != inc_tr && (inc_tr->state == IST_TERMINATED) && (inc_tr->birth_time + 30 < now)) {	/* Wait a max of 30 seconds */
					/* remove the transaction from oSIP */
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
										  "eXosip: release INVITE server transaction (did=%i)\n",
										  jd->d_id));
					osip_remove_transaction(eXosip.j_osip, inc_tr);
					osip_list_remove(jd->d_inc_trs, pos);
					osip_list_add(&eXosip.j_transactions, inc_tr, 0);

					ret = 0;
					break;
				}
			}
			pos++;
		}

		last_invite = eXosip_find_last_out_invite(jc, jd);

		/* go through all outgoing transactions of this dialog */
		pos = 1;
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			if (0 != osip_strcasecmp(out_tr->cseq->method, "INVITE")) {
				/* remove, if transaction too old, independent of the state */
				if ((out_tr->state == NICT_TERMINATED) && (out_tr->birth_time + 30 < now)) {	/* Wait a max of 30 seconds */
					/* remove the transaction from oSIP */
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
										  "eXosip: release non INVITE client transaction (did=%i)\n",
										  jd->d_id));
					osip_remove_transaction(eXosip.j_osip, out_tr);
					osip_list_remove(jd->d_out_trs, pos);
					osip_list_add(&eXosip.j_transactions, out_tr, 0);

					ret = 0;
					break;
				}
			} else {
				/* remove, if transaction too old, independent of the state */
				if (last_invite != out_tr && (out_tr->state == ICT_TERMINATED) && (out_tr->birth_time + 30 < now)) {	/* Wait a max of 30 seconds */
					/* remove the transaction from oSIP */
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
										  "eXosip: release INVITE client transaction (did=%i)\n",
										  jd->d_id));
					osip_remove_transaction(eXosip.j_osip, out_tr);
					osip_list_remove(jd->d_out_trs, pos);
					osip_list_add(&eXosip.j_transactions, out_tr, 0);

					ret = 0;
					break;
				}
			}
			pos++;
		}
	}

	return ret;
}

static int eXosip_release_finished_calls(eXosip_call_t * jc, eXosip_dialog_t * jd)
{
	osip_transaction_t *tr;

	tr = eXosip_find_last_inc_transaction(jc, jd, "BYE");
	if (tr == NULL)
		tr = eXosip_find_last_out_transaction(jc, jd, "BYE");

	if (tr != NULL
		&& (tr->state == NIST_TERMINATED || tr->state == NICT_TERMINATED)) {
		int did = -2;

		if (jd != NULL)
			did = jd->d_id;
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "eXosip: eXosip_release_finished_calls remove a dialog (cid=%i did=%i)\n",
							  jc->c_id, did));
		/* Remove existing reference to the dialog from transactions! */
		__eXosip_call_remove_dialog_reference_in_call(jc, jd);
		REMOVE_ELEMENT(jc->c_dialogs, jd);
		eXosip_dialog_free(jd);
		return OSIP_SUCCESS;
	}
	return OSIP_UNDEFINED_ERROR;
}



static void __eXosip_release_call(eXosip_call_t * jc, eXosip_dialog_t * jd)
{
	REMOVE_ELEMENT(eXosip.j_calls, jc);
	report_call_event(EXOSIP_CALL_RELEASED, jc, jd, NULL);
	eXosip_call_free(jc);
	__eXosip_wakeup();
}


static int eXosip_release_aborted_calls(eXosip_call_t * jc, eXosip_dialog_t * jd)
{
	time_t now = time(NULL);
	osip_transaction_t *tr;

#if 0
	tr = eXosip_find_last_inc_invite(jc, jd);
	if (tr == NULL)
		tr = eXosip_find_last_out_invite(jc, jd);
#else
	/* close calls only when the initial INVITE failed */
	tr = jc->c_inc_tr;
	if (tr == NULL)
		tr = jc->c_out_tr;
#endif

	if (tr == NULL) {
		if (jd != NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "eXosip: eXosip_release_aborted_calls remove an empty dialog\n"));
			__eXosip_call_remove_dialog_reference_in_call(jc, jd);
			REMOVE_ELEMENT(jc->c_dialogs, jd);
			eXosip_dialog_free(jd);
			return OSIP_SUCCESS;
		}
		return OSIP_UNDEFINED_ERROR;
	}

	if (tr != NULL && tr->state != IST_TERMINATED && tr->state != ICT_TERMINATED && tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
		if (jd != NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "eXosip: eXosip_release_aborted_calls remove a dialog for an unfinished transaction\n"));
			__eXosip_call_remove_dialog_reference_in_call(jc, jd);
			REMOVE_ELEMENT(jc->c_dialogs, jd);
			report_call_event(EXOSIP_CALL_NOANSWER, jc, jd, NULL);
			eXosip_dialog_free(jd);
			__eXosip_wakeup();
			return OSIP_SUCCESS;
		}
	}

	if (tr != NULL && (tr->state == IST_TERMINATED || tr->state == ICT_TERMINATED)) {
		if (tr == jc->c_inc_tr) {
			if (jc->c_inc_tr->last_response == NULL) {
				/* OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
				   "eXosip: eXosip_release_aborted_calls transaction with no answer\n")); */
			}
#ifndef MINISIZE
			else if (MSG_IS_STATUS_3XX(jc->c_inc_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls answered with a 3xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_4XX(jc->c_inc_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls answered with a 4xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_5XX(jc->c_inc_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls answered with a 5xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_6XX(jc->c_inc_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls answered with a 6xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			}
#else
			else if (jc->c_inc_tr->last_response->status_code >= 300) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls answered with a answer above 3xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			}
#endif
		} else if (tr == jc->c_out_tr) {
			if (jc->c_out_tr->last_response == NULL) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed with no answer\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			}
#ifndef MINISIZE
			else if (MSG_IS_STATUS_3XX(jc->c_out_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed answered with 3xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_4XX(jc->c_out_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed answered with 4xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_5XX(jc->c_out_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed answered with 5xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			} else if (MSG_IS_STATUS_6XX(jc->c_out_tr->last_response)) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed answered with 6xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			}
#else
			else if (jc->c_out_tr->last_response->status_code >= 300) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_aborted_calls completed answered with 3xx\n"));
				__eXosip_release_call(jc, jd);
				return OSIP_SUCCESS;
			}
#endif
		}
	}

	return OSIP_UNDEFINED_ERROR;
}


void eXosip_release_terminated_calls(void)
{
	eXosip_dialog_t *jd;
	eXosip_dialog_t *jdnext;
	eXosip_call_t *jc;
	eXosip_call_t *jcnext;
	time_t now = time(NULL);
	int pos;


	for (jc = eXosip.j_calls; jc != NULL;) {
		jcnext = jc->next;
		/* free call terminated with a BYE */

		for (jd = jc->c_dialogs; jd != NULL;) {
			jdnext = jd->next;
			if (0 == eXosip_pendingosip_transaction_exist(jc, jd)) {
			} else if (0 == eXosip_release_finished_transactions(jc, jd)) {
			} else if (0 == eXosip_release_finished_calls(jc, jd)) {
				jd = jc->c_dialogs;
			} else if (0 == eXosip_release_aborted_calls(jc, jd)) {
				jdnext = NULL;
			} else if (jd->d_id == -1) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: eXosip_release_terminated_calls delete a removed dialog (cid=%i did=%i)\n",
									  jc->c_id, jd->d_id));
				/* Remove existing reference to the dialog from transactions! */
				__eXosip_call_remove_dialog_reference_in_call(jc, jd);
				REMOVE_ELEMENT(jc->c_dialogs, jd);
				eXosip_dialog_free(jd);

				jd = jc->c_dialogs;
			}
			jd = jdnext;
		}
		jc = jcnext;
	}

	for (jc = eXosip.j_calls; jc != NULL;) {
		jcnext = jc->next;
		if (jc->c_dialogs == NULL) {
			if (jc->c_inc_tr != NULL
				&& jc->c_inc_tr->state != IST_TERMINATED
				&& jc->c_inc_tr->birth_time + 180 < now) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "eXosip: remove an incoming call with no final answer\n"));
				__eXosip_release_call(jc, NULL);
			} else if (jc->c_out_tr != NULL
					   && jc->c_out_tr->state != ICT_TERMINATED
					   && jc->c_out_tr->birth_time + 180 < now) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "eXosip: remove an outgoing call with no final answer\n"));
				__eXosip_release_call(jc, NULL);
			} else if (jc->c_inc_tr != NULL
					   && jc->c_inc_tr->state != IST_TERMINATED) {
			} else if (jc->c_out_tr != NULL
					   && jc->c_out_tr->state != ICT_TERMINATED) {
			} else if (jc->c_out_tr != NULL
					   && jc->c_out_tr->state == ICT_TERMINATED
					   && jc->c_out_tr->completed_time + 10 > now) {
				/* With unreliable protocol, the transaction enter the terminated
				   state right after the ACK is sent: In this case, we really want
				   to wait for additionnal user/automatic action to be processed
				   before we decide to delete the call.
				 */


			} else {			/* no active pending transaction */

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "eXosip: remove a call\n"));
				__eXosip_release_call(jc, NULL);
			}
		}
		jc = jcnext;
	}

	pos = 0;
	while (!osip_list_eol(&eXosip.j_transactions, pos)) {
		osip_transaction_t *tr =
			(osip_transaction_t *) osip_list_get(&eXosip.j_transactions, pos);
		if (tr->state == NICT_TERMINATED
			&& tr->last_response!=NULL
			&& tr->completed_time + 5 > now) {
			/* keep transaction until authentication or ... */
			pos++;
		} else if (tr->state == IST_TERMINATED || tr->state == ICT_TERMINATED || tr->state == NICT_TERMINATED || tr->state == NIST_TERMINATED) {	/* free (transaction is already removed from the oSIP stack) */
			osip_list_remove(&eXosip.j_transactions, pos);
			__eXosip_delete_jinfo(tr);
			_eXosip_dnsutils_release(tr->naptr_record);
			tr->naptr_record=NULL;
			osip_transaction_free(tr);
		} else if (tr->birth_time + 180 < now) {	/* Wait a max of 2 minutes */
			osip_list_remove(&eXosip.j_transactions, pos);
			__eXosip_delete_jinfo(tr);
			_eXosip_dnsutils_release(tr->naptr_record);
			tr->naptr_record=NULL;
			osip_transaction_free(tr);
		} else
			pos++;
	}
}

void eXosip_release_terminated_registrations(void)
{
	eXosip_reg_t *jr;
	eXosip_reg_t *jrnext;
	time_t now = time(NULL);

	for (jr = eXosip.j_reg; jr != NULL;) {
		jrnext = jr->next;
		if (jr->r_reg_period == 0 && jr->r_last_tr != NULL) {
			if (now - jr->r_last_tr->birth_time > 75) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "Release a terminated registration\n"));
				REMOVE_ELEMENT(eXosip.j_reg, jr);
				eXosip_reg_free(jr);
			} else if (jr->r_last_tr->last_response != NULL
					   && jr->r_last_tr->last_response->status_code >= 200
					   && jr->r_last_tr->last_response->status_code <= 299) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "Release a terminated registration with 2xx\n"));
				REMOVE_ELEMENT(eXosip.j_reg, jr);
				eXosip_reg_free(jr);
			}
		}

		jr = jrnext;
	}

	return;
}

void eXosip_release_terminated_publications(void)
{
	eXosip_pub_t *jpub;
	eXosip_pub_t *jpubnext;
	time_t now = time(NULL);

	for (jpub = eXosip.j_pub; jpub != NULL;) {
		jpubnext = jpub->next;
		if (jpub->p_period == 0 && jpub->p_last_tr != NULL) {
			if (now - jpub->p_last_tr->birth_time > 75) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "Release a terminated publication\n"));
				REMOVE_ELEMENT(eXosip.j_pub, jpub);
				_eXosip_pub_free(jpub);
			} else if (jpub->p_last_tr->last_response != NULL
					   && jpub->p_last_tr->last_response->status_code >= 200
					   && jpub->p_last_tr->last_response->status_code <= 299) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "Release a terminated publication with 2xx\n"));
				REMOVE_ELEMENT(eXosip.j_pub, jpub);
				_eXosip_pub_free(jpub);
			}
		}

		jpub = jpubnext;
	}

}

#ifndef MINISIZE

static int
eXosip_release_finished_transactions_for_subscription(eXosip_dialog_t * jd)
{
	time_t now = time(NULL);
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;
	int skip_first = 0;
	int pos;
	int ret;

	ret = OSIP_UNDEFINED_ERROR;

	if (jd != NULL) {
		/* go through all incoming transactions of this dialog */
		pos = 0;
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			/* remove, if transaction too old, independent of the state */
			if ((skip_first == 1) && (inc_tr->state == NIST_TERMINATED) && (inc_tr->birth_time + 30 < now)) {	/* keep it for 30 seconds */
				/* remove the transaction from oSIP */
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: releaase non-INVITE server transaction (did=%i)\n",
									  jd->d_id));
				osip_remove_transaction(eXosip.j_osip, inc_tr);
				osip_list_remove(jd->d_inc_trs, pos);
				osip_list_add(&eXosip.j_transactions, inc_tr, 0);

				ret = OSIP_SUCCESS;	/* return "released" */
				break;
			}
			if (0 == osip_strcasecmp(inc_tr->cseq->method, "SUBSCRIBE"))
				skip_first = 1;
			if (0 == osip_strcasecmp(inc_tr->cseq->method, "NOTIFY"))
				skip_first = 1;
			pos++;
		}

		skip_first = 0;

		/* go through all outgoing transactions of this dialog */
		pos = 0;
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			/* remove, if transaction too old, independent of the state */
			if ((skip_first == 1) && (out_tr->state == NICT_TERMINATED) && (out_tr->birth_time + 30 < now)) {	/* Wait a max of 30 seconds */
				/* remove the transaction from oSIP */
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "eXosip: release non INVITE client transaction (did=%i)\n",
									  jd->d_id));
				osip_remove_transaction(eXosip.j_osip, out_tr);
				osip_list_remove(jd->d_out_trs, pos);
				osip_list_add(&eXosip.j_transactions, out_tr, 0);

				ret = OSIP_SUCCESS;	/* return "released" */
				break;
			}
			if (0 == osip_strcasecmp(out_tr->cseq->method, "SUBSCRIBE"))
				skip_first = 1;
			if (0 == osip_strcasecmp(out_tr->cseq->method, "NOTIFY"))
				skip_first = 1;
			pos++;
		}
	}

	return ret;
}

void eXosip_release_terminated_subscriptions()
{
	time_t now = time(NULL);
	eXosip_dialog_t *jd;
	eXosip_dialog_t *jdnext;
	eXosip_subscribe_t *js;
	eXosip_subscribe_t *jsnext;

	for (js = eXosip.j_subscribes; js != NULL;) {
		jsnext = js->next;

		if (js->s_dialogs == NULL) {
			if (js->s_out_tr != NULL && js->s_out_tr->birth_time + 64 < now) {	/* Wait a max of 64 sec */
				/* destroy non established contexts after max of 64 sec */
				REMOVE_ELEMENT(eXosip.j_subscribes, js);
				eXosip_subscribe_free(js);
				__eXosip_wakeup();
				return;
			}
		} else {
			/* fix 14/07/11. NULL pointer */
			jd = js->s_dialogs;
			if (jd!= NULL) {
				osip_transaction_t *transaction = eXosip_find_last_out_subscribe(js, jd);
				if (transaction != NULL
					&& transaction->orig_request!=NULL
					&& transaction->state == NICT_TERMINATED
					&& transaction->birth_time + 15 < now)
				{
					osip_header_t *expires;

					osip_message_get_expires(transaction->orig_request, 0, &expires);
					if (expires == NULL || expires->hvalue == NULL) {
					} else if (0 == strcmp(expires->hvalue, "0")) {
						/* In TCP mode, we don't have enough time to authenticate */
						REMOVE_ELEMENT(eXosip.j_subscribes, js);
						eXosip_subscribe_free(js);
						__eXosip_wakeup();
						return;
					}
				}
			}

			for (jd = js->s_dialogs; jd != NULL;) {
				jdnext = jd->next;
				eXosip_release_finished_transactions_for_subscription(jd);

				if (jd->d_dialog == NULL || jd->d_dialog->state == DIALOG_EARLY) {
					if (js->s_out_tr != NULL && js->s_out_tr->birth_time + 64 < now) {	/* Wait a max of 2 minutes */
						/* destroy non established contexts after max of 64 sec */
						REMOVE_ELEMENT(eXosip.j_subscribes, js);
						eXosip_subscribe_free(js);
						__eXosip_wakeup();
						return;
					}
				}

				jd = jdnext;
			}
		}
		js = jsnext;
	}

}

void eXosip_release_terminated_in_subscriptions(void)
{
	eXosip_dialog_t *jd;
	eXosip_dialog_t *jdnext;
	eXosip_notify_t *jn;
	eXosip_notify_t *jnnext;

	for (jn = eXosip.j_notifies; jn != NULL;) {
		jnnext = jn->next;

		for (jd = jn->n_dialogs; jd != NULL;) {
			jdnext = jd->next;
			eXosip_release_finished_transactions_for_subscription(jd);
			jd = jdnext;
		}
		jn = jnnext;
	}
}

#endif
