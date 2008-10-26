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

#ifndef MINISIZE

#include "eXosip2.h"

extern eXosip_t eXosip;

osip_transaction_t *
eXosip_find_last_inc_subscribe (eXosip_notify_t * jn, eXosip_dialog_t * jd)
{
  osip_transaction_t *inc_tr;
  int pos;

  inc_tr = NULL;
  pos = 0;
  if (jd != NULL)
    {
      while (!osip_list_eol (jd->d_inc_trs, pos))
        {
          inc_tr = osip_list_get (jd->d_inc_trs, pos);
          if (0 == strcmp (inc_tr->cseq->method, "SUBSCRIBE"))
            break;
          else
            inc_tr = NULL;
          pos++;
        }
  } else
    inc_tr = NULL;

  if (inc_tr == NULL)
    return jn->n_inc_tr;        /* can be NULL */

  return inc_tr;
}


osip_transaction_t *
eXosip_find_last_out_notify (eXosip_notify_t * jn, eXosip_dialog_t * jd)
{
  osip_transaction_t *out_tr;
  int pos;

  out_tr = NULL;
  pos = 0;
  if (jd != NULL)
    {
      while (!osip_list_eol (jd->d_out_trs, pos))
        {
          out_tr = osip_list_get (jd->d_out_trs, pos);
          if (0 == strcmp (out_tr->cseq->method, "NOTIFY"))
            return out_tr;
          pos++;
        }
    }

  return NULL;
}

int
eXosip_notify_init (eXosip_notify_t ** jn, osip_message_t * inc_subscribe)
{
  osip_contact_t *co;

  *jn = NULL;
  co = (osip_contact_t *) osip_list_get (&inc_subscribe->contacts, 0);
  if (co == NULL || co->url == NULL)
    return -1;

  *jn = (eXosip_notify_t *) osip_malloc (sizeof (eXosip_notify_t));
  if (*jn == NULL)
    return OSIP_NOMEM;
  memset (*jn, 0, sizeof (eXosip_notify_t));

  return OSIP_SUCCESS;
}

void
eXosip_notify_free (eXosip_notify_t * jn)
{
  /* ... */

  eXosip_dialog_t *jd;

  if (jn->n_inc_tr != NULL && jn->n_inc_tr->orig_request != NULL
      && jn->n_inc_tr->orig_request->call_id != NULL
      && jn->n_inc_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (jn->n_inc_tr->orig_request->call_id->number);
  else if (jn->n_out_tr != NULL && jn->n_out_tr->orig_request != NULL
           && jn->n_out_tr->orig_request->call_id != NULL
           && jn->n_out_tr->orig_request->call_id->number != NULL)
    _eXosip_delete_nonce (jn->n_out_tr->orig_request->call_id->number);

  for (jd = jn->n_dialogs; jd != NULL; jd = jn->n_dialogs)
    {
      REMOVE_ELEMENT (jn->n_dialogs, jd);
      eXosip_dialog_free (jd);
    }

  __eXosip_delete_jinfo (jn->n_inc_tr);
  __eXosip_delete_jinfo (jn->n_out_tr);
  if (jn->n_inc_tr != NULL)
    osip_list_add (&eXosip.j_transactions, jn->n_inc_tr, 0);
  if (jn->n_out_tr != NULL)
    osip_list_add (&eXosip.j_transactions, jn->n_out_tr, 0);
  osip_free (jn);
}

int
_eXosip_notify_set_refresh_interval (eXosip_notify_t * jn,
                                     osip_message_t * inc_subscribe)
{
  osip_header_t *exp;
  time_t now;

  now = time (NULL);
  if (jn == NULL || inc_subscribe == NULL)
    return -1;

  osip_message_get_expires (inc_subscribe, 0, &exp);
  if (exp == NULL || exp->hvalue == NULL)
    jn->n_ss_expires = now + 600;
  else
    {
      jn->n_ss_expires = osip_atoi (exp->hvalue);
      if (jn->n_ss_expires != -1)
        jn->n_ss_expires = now + jn->n_ss_expires;
      else                      /* on error, set it to default */
        jn->n_ss_expires = now + 600;
    }

  return OSIP_SUCCESS;
}

void
_eXosip_notify_add_expires_in_2XX_for_subscribe (eXosip_notify_t * jn,
                                                 osip_message_t * answer)
{
  char tmp[20];
  time_t now;

  now = time (NULL);

  if (jn->n_ss_expires - now < 0)
    {
      tmp[0] = '0';
      tmp[1] = '\0';
  } else
    {
      sprintf (tmp, "%li", jn->n_ss_expires - now);
    }
  osip_message_set_expires (answer, tmp);
}

#endif
