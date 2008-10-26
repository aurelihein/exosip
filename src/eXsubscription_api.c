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

int
_eXosip_subscribe_transaction_find (int tid, eXosip_subscribe_t ** js,
                                    eXosip_dialog_t ** jd,
                                    osip_transaction_t ** tr)
{
  for (*js = eXosip.j_subscribes; *js != NULL; *js = (*js)->next)
    {
      if ((*js)->s_inc_tr != NULL && (*js)->s_inc_tr->transactionid == tid)
        {
          *tr = (*js)->s_inc_tr;
          *jd = (*js)->s_dialogs;
          return OSIP_SUCCESS;
        }
      if ((*js)->s_out_tr != NULL && (*js)->s_out_tr->transactionid == tid)
        {
          *tr = (*js)->s_out_tr;
          *jd = (*js)->s_dialogs;
          return OSIP_SUCCESS;
        }
      for (*jd = (*js)->s_dialogs; *jd != NULL; *jd = (*jd)->next)
        {
          osip_transaction_t *transaction;
          int pos = 0;

          while (!osip_list_eol ((*jd)->d_inc_trs, pos))
            {
              transaction =
                (osip_transaction_t *) osip_list_get ((*jd)->d_inc_trs, pos);
              if (transaction != NULL && transaction->transactionid == tid)
                {
                  *tr = transaction;
                  return OSIP_SUCCESS;
                }
              pos++;
            }

          pos = 0;
          while (!osip_list_eol ((*jd)->d_out_trs, pos))
            {
              transaction =
                (osip_transaction_t *) osip_list_get ((*jd)->d_out_trs, pos);
              if (transaction != NULL && transaction->transactionid == tid)
                {
                  *tr = transaction;
                  return OSIP_SUCCESS;
                }
              pos++;
            }
        }
    }
  *jd = NULL;
  *js = NULL;
  return OSIP_NOTFOUND;
}

int
eXosip_subscribe_remove (int did)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  if (did <= 0)
    return OSIP_BADPARAMETER;

  if (did > 0)
    {
      eXosip_subscribe_dialog_find (did, &js, &jd);
    }
  if (js == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: No outgoing subscription here?\n"));
      return OSIP_NOTFOUND;
    }
  REMOVE_ELEMENT (eXosip.j_subscribes, js);
  eXosip_subscribe_free (js);
  return OSIP_SUCCESS;
}

int
eXosip_subscribe_build_initial_request (osip_message_t ** sub, const char *to,
                                        const char *from, const char *route,
                                        const char *event, int expires)
{
  char tmp[10];
  int i;
  osip_to_t *_to = NULL;

  *sub = NULL;
  if (to == NULL || *to == '\0')
    return OSIP_BADPARAMETER;
  if (from == NULL || *from == '\0')
    return OSIP_BADPARAMETER;
  if (event == NULL || *event == '\0')
    return OSIP_BADPARAMETER;
  if (route == NULL || *route == '\0')
    route = NULL;

  i = osip_to_init (&_to);
  if (i != 0)
    return i;

  i = osip_to_parse (_to, to);
  if (i != 0)
    {
      osip_to_free (_to);
      return i;
    }

  i =
    generating_request_out_of_dialog (sub, "SUBSCRIBE", to, eXosip.transport, from,
                                      route);
  osip_to_free (_to);
  if (i != 0)
    return i;
  _eXosip_dialog_add_contact (*sub, NULL);

  snprintf (tmp, 10, "%i", expires);
  osip_message_set_expires (*sub, tmp);

  osip_message_set_header (*sub, "Event", event);

  return OSIP_SUCCESS;
}

int
eXosip_subscribe_send_initial_request (osip_message_t * subscribe)
{
  eXosip_subscribe_t *js = NULL;
  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;

  i = eXosip_subscribe_init (&js);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: cannot subscribe."));
      osip_message_free (subscribe);
      return i;
    }

  i = _eXosip_transaction_init (&transaction, NICT, eXosip.j_osip, subscribe);
  if (i != 0)
    {
      eXosip_subscribe_free (js);
      osip_message_free (subscribe);
      return i;
    }

  js->s_reg_period = 3600;
  _eXosip_subscribe_set_refresh_interval (js, subscribe);
  js->s_out_tr = transaction;

  sipevent = osip_new_outgoing_sipmessage (subscribe);
  sipevent->transactionid = transaction->transactionid;

  osip_transaction_set_your_instance (transaction,
                                      __eXosip_new_jinfo (NULL, NULL, js, NULL));
  osip_transaction_add_event (transaction, sipevent);

  ADD_ELEMENT (eXosip.j_subscribes, js);
  eXosip_update ();             /* fixed? */
  __eXosip_wakeup ();
  return js->s_id;
}

int
eXosip_subscribe_build_refresh_request (int did, osip_message_t ** sub)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  osip_transaction_t *transaction;
  char *transport;
  int i;

  *sub = NULL;

  if (did <= 0)
    return OSIP_BADPARAMETER;

  if (did > 0)
    {
      eXosip_subscribe_dialog_find (did, &js, &jd);
    }
  if (jd == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: No subscribe here?\n"));
      return OSIP_NOTFOUND;
    }

  transaction = NULL;
  transaction = eXosip_find_last_out_subscribe (js, jd);

  if (transaction != NULL)
    {
      if (transaction->state != NICT_TERMINATED &&
          transaction->state != NIST_TERMINATED &&
          transaction->state != NICT_COMPLETED &&
          transaction->state != NIST_COMPLETED)
        return OSIP_WRONG_STATE;
    }

  transport = NULL;
  if (transaction != NULL && transaction->orig_request != NULL)
    transport = _eXosip_transport_protocol (transaction->orig_request);

  transaction = NULL;

  if (transport == NULL)
    i =
      _eXosip_build_request_within_dialog (sub, "SUBSCRIBE", jd->d_dialog, "UDP");
  else
    i =
      _eXosip_build_request_within_dialog (sub, "SUBSCRIBE", jd->d_dialog,
                                           transport);

  if (i != 0)
    return i;


  eXosip_add_authentication_information (*sub, NULL);

  return OSIP_SUCCESS;
}

int
eXosip_subscribe_send_refresh_request (int did, osip_message_t * sub)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;

  osip_transaction_t *transaction;
  osip_event_t *sipevent;
  int i;

  if (did <= 0)
    return OSIP_BADPARAMETER;

  if (did > 0)
    {
      eXosip_subscribe_dialog_find (did, &js, &jd);
    }
  if (jd == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: No subscribe here?\n"));
      osip_message_free (sub);
      return OSIP_NOTFOUND;
    }

  transaction = NULL;
  transaction = eXosip_find_last_out_subscribe (js, jd);

  if (transaction != NULL)
    {
      if (transaction->state != NICT_TERMINATED &&
          transaction->state != NIST_TERMINATED &&
          transaction->state != NICT_COMPLETED &&
          transaction->state != NIST_COMPLETED)
        {
          osip_message_free (sub);
          return OSIP_WRONG_STATE;
        }
      transaction = NULL;
    }

  transaction = NULL;
  i = _eXosip_transaction_init (&transaction, NICT, eXosip.j_osip, sub);

  if (i != 0)
    {
      osip_message_free (sub);
      return i;
    }

  js->s_reg_period = 3600;
  _eXosip_subscribe_set_refresh_interval (js, sub);
  osip_list_add (jd->d_out_trs, transaction, 0);

  sipevent = osip_new_outgoing_sipmessage (sub);
  sipevent->transactionid = transaction->transactionid;

  osip_transaction_set_your_instance (transaction,
                                      __eXosip_new_jinfo (NULL, jd, js, NULL));
  osip_transaction_add_event (transaction, sipevent);
  __eXosip_wakeup ();
  return OSIP_SUCCESS;
}

int
_eXosip_subscribe_automatic_refresh (eXosip_subscribe_t * js,
                                     eXosip_dialog_t * jd,
                                     osip_transaction_t * out_tr)
{
  osip_message_t *sub = NULL;
  osip_header_t *expires;
  int i;
  if (js == NULL || jd == NULL || out_tr == NULL || out_tr->orig_request == NULL)
    return OSIP_BADPARAMETER;

  i = eXosip_subscribe_build_refresh_request (jd->d_id, &sub);
  if (i != 0)
    return i;

  i = osip_message_get_expires (out_tr->orig_request, 0, &expires);
  if (expires != NULL && expires->hvalue != NULL)
    {
      osip_message_set_expires (sub, expires->hvalue);
    }

  {
    int pos = 0;
    osip_accept_t *_accept = NULL;

    i = osip_message_get_accept (out_tr->orig_request, pos, &_accept);
    while (i >= 0 && _accept != NULL)
      {
        osip_accept_t *_accept2;

        i = osip_accept_clone (_accept, &_accept2);
        if (i != 0)
          {
            OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                    "Error in Accept header\n"));
            break;
          }
        osip_list_add (&sub->accepts, _accept2, -1);
        _accept = NULL;
        pos++;
        i = osip_message_get_accept (out_tr->orig_request, pos, &_accept);
      }
  }

  {
    int pos = 0;
    osip_header_t *_event = NULL;

    pos =
      osip_message_header_get_byname (out_tr->orig_request, "Event", 0, &_event);
    while (pos >= 0 && _event != NULL)
      {
        osip_header_t *_event2;

        i = osip_header_clone (_event, &_event2);
        if (i != 0)
          {
            OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                    "Error in Event header\n"));
            break;
          }
        osip_list_add (&sub->headers, _event2, -1);
        _event = NULL;
        pos++;
        pos =
          osip_message_header_get_byname (out_tr->orig_request, "Event", pos,
                                          &_event);
      }
  }

  i = eXosip_subscribe_send_refresh_request (jd->d_id, sub);
  return i;
}

int
_eXosip_subscribe_send_request_with_credential (eXosip_subscribe_t * js,
                                                eXosip_dialog_t * jd,
                                                osip_transaction_t * out_tr)
{
  osip_transaction_t *tr = NULL;
  osip_message_t *msg = NULL;
  osip_event_t *sipevent;

  int cseq;
  osip_via_t *via;
  int i;

  if (js == NULL)
    return OSIP_BADPARAMETER;
  if (jd != NULL)
    {
      if (jd->d_out_trs == NULL)
        return OSIP_BADPARAMETER;
    }

  if (out_tr == NULL)
    {
      out_tr = eXosip_find_last_out_subscribe (js, jd);
    }

  if (out_tr == NULL
      || out_tr->orig_request == NULL || out_tr->last_response == NULL)
    return OSIP_NOTFOUND;

  i = osip_message_clone (out_tr->orig_request, &msg);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: could not clone msg for authentication\n"));
      return i;
    }

  {
    osip_generic_param_t *tag = NULL;

    osip_to_get_tag (msg->to, &tag);
    if (NULL == tag && jd != NULL && jd->d_dialog != NULL
        && jd->d_dialog->remote_tag != NULL)
      {
        osip_to_set_tag (msg->to, osip_strdup (jd->d_dialog->remote_tag));
      }
  }

  via = (osip_via_t *) osip_list_get (&msg->vias, 0);
  if (via == NULL || msg->cseq == NULL || msg->cseq->number == NULL)
    {
      osip_message_free (msg);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: missing via or cseq header\n"));
      return OSIP_SYNTAXERROR;
    }

  /* increment cseq */
  cseq = atoi (msg->cseq->number);
  osip_free (msg->cseq->number);
  msg->cseq->number = strdup_printf ("%i", cseq + 1);
  if (msg->cseq->number == NULL)
    {
      osip_message_free (msg);
      return OSIP_NOMEM;
    }

  if (jd != NULL && jd->d_dialog != NULL)
    {
      jd->d_dialog->local_cseq++;
    }

  i = eXosip_update_top_via (msg);
  if (i != 0)
    {
      osip_message_free (msg);
      return i;
    }

  eXosip_add_authentication_information (msg, out_tr->last_response);
  osip_message_force_update (msg);

  i = _eXosip_transaction_init (&tr, NICT, eXosip.j_osip, msg);

  if (i != 0)
    {
      osip_message_free (msg);
      return i;
    }

  if (out_tr == js->s_out_tr)
    {
      /* replace with the new tr */
      osip_list_add (&eXosip.j_transactions, js->s_out_tr, 0);
      js->s_out_tr = tr;
  } else
    {
      /* add the new tr for the current dialog */
      osip_list_add (jd->d_out_trs, tr, 0);
    }

  sipevent = osip_new_outgoing_sipmessage (msg);

  osip_transaction_set_your_instance (tr, __eXosip_new_jinfo (NULL, jd, js, NULL));
  osip_transaction_add_event (tr, sipevent);

  eXosip_update ();             /* fixed? */
  __eXosip_wakeup ();
  return OSIP_SUCCESS;
}

#endif
