/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard@antisip.com
  
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

int
_eXosip_call_find (struct eXosip_t *excontext, int cid, eXosip_call_t ** jc)
{
  if (cid <= 0)
    return OSIP_BADPARAMETER;

  for (*jc = excontext->j_calls; *jc != NULL; *jc = (*jc)->next) {
    if ((*jc)->c_id == cid) {
      return OSIP_SUCCESS;
    }
  }
  *jc = NULL;
  return OSIP_NOTFOUND;
}

void
_eXosip_call_renew_expire_time (eXosip_call_t * jc)
{
  time_t now = osip_getsystemtime (NULL);

  jc->expire_time = now + 180;
}

int
_eXosip_call_init (eXosip_call_t ** jc)
{
  *jc = (eXosip_call_t *) osip_malloc (sizeof (eXosip_call_t));
  if (*jc == NULL)
    return OSIP_NOMEM;
  memset (*jc, 0, sizeof (eXosip_call_t));

  (*jc)->c_id = -1;             /* make sure the _eXosip_update will assign a valid id to the call */
  return OSIP_SUCCESS;
}

void
_eXosip_call_remove_dialog_reference_in_call (eXosip_call_t * jc, eXosip_dialog_t * jd)
{
  eXosip_dialog_t *_jd;

  if (jc == NULL)
    return;
  if (jd == NULL)
    return;


  for (_jd = jc->c_dialogs; _jd != NULL; _jd = _jd->next) {
    if (jd == _jd)
      break;
  }
  if (_jd == NULL) {
    /* dialog not found??? */
  }

  _jd = (eXosip_dialog_t *) osip_transaction_get_reserved3 (jc->c_inc_tr);
  if (_jd != NULL && _jd == jd)
    osip_transaction_set_reserved3 (jc->c_inc_tr, NULL);
  _jd = (eXosip_dialog_t *) osip_transaction_get_reserved3 (jc->c_out_tr);
  if (_jd != NULL && _jd == jd)
    osip_transaction_set_reserved3 (jc->c_out_tr, NULL);
}

void
_eXosip_call_free (struct eXosip_t *excontext, eXosip_call_t * jc)
{
  /* ... */

  eXosip_dialog_t *jd;

  if (jc->c_inc_tr != NULL && jc->c_inc_tr->orig_request != NULL && jc->c_inc_tr->orig_request->call_id != NULL && jc->c_inc_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (excontext, jc->c_inc_tr->orig_request->call_id->number);
  else if (jc->c_out_tr != NULL && jc->c_out_tr->orig_request != NULL && jc->c_out_tr->orig_request->call_id != NULL && jc->c_out_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (excontext, jc->c_out_tr->orig_request->call_id->number);

  for (jd = jc->c_dialogs; jd != NULL; jd = jc->c_dialogs) {
    REMOVE_ELEMENT (jc->c_dialogs, jd);
    _eXosip_dialog_free (excontext, jd);
  }

  _eXosip_delete_reserved (jc->c_inc_tr);
  _eXosip_delete_reserved (jc->c_out_tr);
  if (jc->c_inc_tr != NULL)
    osip_list_add (&excontext->j_transactions, jc->c_inc_tr, 0);
  if (jc->c_out_tr != NULL)
    osip_list_add (&excontext->j_transactions, jc->c_out_tr, 0);

  osip_free (jc);

}
