/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard_at_osip.org
  
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

#include "eXosip2.h"

static int _eXosip_register_build_register(struct eXosip_t *excontext, eXosip_reg_t * jr,
										   osip_message_t ** _reg);

static eXosip_reg_t *eXosip_reg_find(struct eXosip_t *excontext, int rid)
{
	eXosip_reg_t *jr;

	for (jr = excontext->j_reg; jr != NULL; jr = jr->next) {
		if (jr->r_id == rid) {
			return jr;
		}
	}
	return NULL;
}

int eXosip_register_remove(struct eXosip_t *excontext, int rid)
{
	eXosip_reg_t *jr;

	if (rid <= 0)
		return OSIP_BADPARAMETER;

	jr = eXosip_reg_find(excontext, rid);
	if (jr == NULL)
		return OSIP_NOTFOUND;
	jr->r_reg_period = 0;
	REMOVE_ELEMENT(excontext->j_reg, jr);
	_eXosip_reg_free(excontext, jr);
	jr = NULL;
	return OSIP_SUCCESS;
}

static int
_eXosip_register_build_register(struct eXosip_t *excontext, eXosip_reg_t * jr, osip_message_t ** _reg)
{
	osip_message_t *reg = NULL;
	int i;

	*_reg = NULL;

	if (jr == NULL)
		return OSIP_BADPARAMETER;

	if (jr->r_last_tr != NULL) {
		if (jr->r_last_tr->state != NICT_TERMINATED
			&& jr->r_last_tr->state != NICT_COMPLETED)
			return OSIP_WRONG_STATE;
		else {
			osip_message_t *last_response = NULL;
			osip_transaction_t *tr;

			i = osip_message_clone(jr->r_last_tr->orig_request, &reg);
			if (i != 0)
				return i;
			if (jr->r_last_tr->last_response != NULL) {
				i = osip_message_clone(jr->r_last_tr->last_response,
									   &last_response);
				if (i != 0) {
					osip_message_free(reg);
					return i;
				}
			}

			_eXosip_delete_reserved(jr->r_last_tr);
			tr = jr->r_last_tr;
			jr->r_last_tr = NULL;
			osip_list_add(&excontext->j_transactions, tr, 0);

			/* modify the REGISTER request */
			{
				int osip_cseq_num = osip_atoi(reg->cseq->number);
				int length = (int)strlen(reg->cseq->number);


				osip_list_special_free(&reg->authorizations, (void (*)(void *))
									   &osip_authorization_free);
				osip_list_special_free(&reg->proxy_authorizations,
									   (void (*)(void *))
									   &osip_proxy_authorization_free);


				i = _eXosip_update_top_via(reg);
				if (i != 0) {
					osip_message_free(reg);
					if (last_response != NULL)
						osip_message_free(last_response);
					return i;
				}

				osip_cseq_num++;
				osip_free(reg->cseq->number);
				reg->cseq->number = (char *) osip_malloc(length + 2);	/* +2 like for 9 to 10 */
				if (reg->cseq->number == NULL) {
					osip_message_free(reg);
					if (last_response != NULL)
						osip_message_free(last_response);
					return OSIP_NOMEM;
				}
				snprintf(reg->cseq->number, length + 2, "%i", osip_cseq_num);


				if (last_response != NULL && last_response->status_code == 423) {
					/* increase expires value to "min-expires" value */
					osip_header_t *exp;
					osip_header_t *min_exp;

					osip_message_header_get_byname(reg, "expires", 0, &exp);
					osip_message_header_get_byname(last_response, "min-expires", 0,
												   &min_exp);
					if (exp != NULL && exp->hvalue != NULL && min_exp != NULL
						&& min_exp->hvalue != NULL) {
						osip_free(exp->hvalue);
						exp->hvalue = osip_strdup(min_exp->hvalue);
						jr->r_reg_period = atoi(min_exp->hvalue);
					} else {
						osip_message_free(reg);
						if (last_response != NULL)
							osip_message_free(last_response);
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_ERROR, NULL,
									"eXosip: missing Min-Expires or Expires in REGISTER\n"));
						return OSIP_SYNTAXERROR;
					}
				} else {
					osip_header_t *exp;

					osip_message_header_get_byname(reg, "expires", 0, &exp);
					if (exp != NULL) {
						if (exp->hvalue != NULL)
							osip_free(exp->hvalue);
						exp->hvalue = (char *) osip_malloc(10);
						if (exp->hvalue == NULL) {
							osip_message_free(reg);
							if (last_response != NULL)
								osip_message_free(last_response);
							return OSIP_NOMEM;
						}
						snprintf(exp->hvalue, 9, "%i", jr->r_reg_period);
					}
				}

				osip_message_force_update(reg);
			}

			if (last_response != NULL) {
				if (last_response->status_code == 401
					|| last_response->status_code == 407) {
					_eXosip_add_authentication_information(excontext, reg, last_response);
				} else
					_eXosip_add_authentication_information(excontext, reg, NULL);
				osip_message_free(last_response);
			}
		}
	}
	if (reg == NULL) {
		i = _eXosip_generating_register(excontext, jr, &reg, excontext->transport,
								jr->r_aor, jr->r_registrar, jr->r_contact,
								jr->r_reg_period);
		if (i != 0)
			return i;
	}

	*_reg = reg;
	return OSIP_SUCCESS;
}

int
eXosip_register_build_initial_register_withqvalue(struct eXosip_t *excontext, const char *from, const char *proxy,
									   const char *contact, int expires,
									   const char *qvalue,
									   osip_message_t ** reg)
{
	eXosip_reg_t *jr = NULL;
	int i;

	*reg = NULL;

	if (from == NULL || proxy == NULL)
		return OSIP_BADPARAMETER;

#ifdef REJECT_DOUBLE_REGISTRATION
	/* Avoid adding the same registration info twice to prevent mem leaks */
	for (jr = excontext->j_reg; jr != NULL; jr = jr->next) {
		if (strcmp(jr->r_aor, from) == 0 && strcmp(jr->r_registrar, proxy) == 0) {
			REMOVE_ELEMENT(excontext->j_reg, jr);
			_eXosip_reg_free(excontext, jr);
			jr = NULL;
			break;
		}
	}
#endif

	if (jr == NULL) {
		/* Add new registration info */
		i = _eXosip_reg_init(excontext, &jr, from, proxy, contact);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot register! "));
			return i;
		}
		ADD_ELEMENT(excontext->j_reg, jr);
	}

	/* build register */
	jr->r_reg_period = expires;
	if (jr->r_reg_period <= 0)	/* too low */
		jr->r_reg_period = 0;
	else if (jr->r_reg_period < 30)	/* too low */
		jr->r_reg_period = 30;

	if(qvalue)
		osip_strncpy(jr->r_qvalue, qvalue, sizeof(jr->r_qvalue));

	i = _eXosip_register_build_register(excontext, jr, reg);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot build REGISTER!\n"));
		*reg = NULL;
		return i;
	}

	return jr->r_id;
}

int
eXosip_register_build_initial_register(struct eXosip_t *excontext, const char *from, const char *proxy,
									   const char *contact, int expires,
									   osip_message_t ** reg)
{
	return eXosip_register_build_initial_register_withqvalue(excontext, from, proxy, contact, expires, NULL, reg);
}

int eXosip_register_build_register(struct eXosip_t *excontext, int rid, int expires, osip_message_t ** reg)
{
	eXosip_reg_t *jr;
	int i;

	*reg = NULL;

	if (rid <= 0)
		return OSIP_BADPARAMETER;

	jr = eXosip_reg_find(excontext, rid);
	if (jr == NULL)
		return OSIP_NOTFOUND;
	jr->r_reg_period = expires;
	if (jr->r_reg_period == 0) {
	} /* unregistration */
	else if (jr->r_reg_period > 3600)
		jr->r_reg_period = 3600;
	else if (jr->r_reg_period < 30)	/* too low */
		jr->r_reg_period = 30;

	if (jr->r_last_tr != NULL) {
		if (jr->r_last_tr->state != NICT_TERMINATED
			&& jr->r_last_tr->state != NICT_COMPLETED) {
			return OSIP_WRONG_STATE;
		}
	}

	i = _eXosip_register_build_register(excontext, jr, reg);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: cannot build REGISTER!"));
		*reg = NULL;
		return i;
	}
	return OSIP_SUCCESS;
}

int eXosip_register_send_register(struct eXosip_t *excontext, int rid, osip_message_t * reg)
{
	osip_transaction_t *transaction;
	osip_event_t *sipevent;
	eXosip_reg_t *jr;
	int i;

	if (rid <= 0) {
		osip_message_free(reg);
		return OSIP_BADPARAMETER;
	}

	jr = eXosip_reg_find(excontext, rid);
	if (jr == NULL) {
		osip_message_free(reg);
		return OSIP_NOTFOUND;
	}

	if (jr->r_last_tr != NULL) {
		if (jr->r_last_tr->state != NICT_TERMINATED
			&& jr->r_last_tr->state != NICT_COMPLETED) {
			osip_message_free(reg);
			return OSIP_WRONG_STATE;
		}
	}

	if (reg == NULL) {
		i = _eXosip_register_build_register(excontext, jr, &reg);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: cannot build REGISTER!"));
			return i;
		}
	}

	i = _eXosip_transaction_init(excontext, &transaction, NICT, excontext->j_osip, reg);
	if (i != 0) {
		osip_message_free(reg);
		return i;
	}

	jr->r_last_tr = transaction;

	/* send REGISTER */
	sipevent = osip_new_outgoing_sipmessage(reg);
	sipevent->transactionid = transaction->transactionid;
	osip_message_force_update(reg);

	osip_transaction_add_event(transaction, sipevent);
	_eXosip_wakeup(excontext);
	return OSIP_SUCCESS;
}
