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

int
eXosip_call_find (int cid, eXosip_call_t ** jc)
{
  if (cid <= 0)
    return OSIP_BADPARAMETER;

  for (*jc = eXosip.j_calls; *jc != NULL; *jc = (*jc)->next)
    {
      if ((*jc)->c_id == cid)
        {
          return OSIP_SUCCESS;
        }
    }
  *jc = NULL;
  return OSIP_NOTFOUND;
}

int
eXosip_call_init (eXosip_call_t ** jc)
{
  *jc = (eXosip_call_t *) osip_malloc (sizeof (eXosip_call_t));
  if (*jc == NULL)
    return OSIP_NOMEM;
  memset (*jc, 0, sizeof (eXosip_call_t));

  (*jc)->c_id = -1;             /* make sure the eXosip_update will assign a valid id to the call */
  return OSIP_SUCCESS;
}

void
__eXosip_call_remove_dialog_reference_in_call (eXosip_call_t * jc,
                                               eXosip_dialog_t * jd)
{
  eXosip_dialog_t *_jd;
  jinfo_t *ji;

  if (jc == NULL)
    return;
  if (jd == NULL)
    return;


  for (_jd = jc->c_dialogs; _jd != NULL; _jd = _jd->next)
    {
      if (jd == _jd)
        break;
    }
  if (_jd == NULL)
    {
      /* dialog not found??? */
    }

  ji = osip_transaction_get_your_instance (jc->c_inc_tr);
  if (ji != NULL && ji->jd == jd)
    ji->jd = NULL;
  ji = osip_transaction_get_your_instance (jc->c_out_tr);
  if (ji != NULL && ji->jd == jd)
    ji->jd = NULL;
}

void
eXosip_call_free (eXosip_call_t * jc)
{
  /* ... */

  eXosip_dialog_t *jd;

  if (jc->c_inc_tr != NULL && jc->c_inc_tr->orig_request != NULL
      && jc->c_inc_tr->orig_request->call_id != NULL
      && jc->c_inc_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (jc->c_inc_tr->orig_request->call_id->number);
  else if (jc->c_out_tr != NULL && jc->c_out_tr->orig_request != NULL
           && jc->c_out_tr->orig_request->call_id != NULL
           && jc->c_out_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (jc->c_out_tr->orig_request->call_id->number);

  for (jd = jc->c_dialogs; jd != NULL; jd = jc->c_dialogs)
    {
      REMOVE_ELEMENT (jc->c_dialogs, jd);
      eXosip_dialog_free (jd);
    }

  __eXosip_delete_jinfo (jc->c_inc_tr);
  __eXosip_delete_jinfo (jc->c_out_tr);
  if (jc->c_inc_tr != NULL)
    osip_list_add (&eXosip.j_transactions, jc->c_inc_tr, 0);
  if (jc->c_out_tr != NULL)
    osip_list_add (&eXosip.j_transactions, jc->c_out_tr, 0);

  osip_free (jc);

}
