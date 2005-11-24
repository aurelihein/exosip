/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002,2003,2004,2005  Aymeric MOIZARD  - jack@atosc.org
  
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



#ifndef  WIN32
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

/* Private functions */
static jauthinfo_t *eXosip_find_authentication_info (const char *username,
                                                     const char *realm);

eXosip_t eXosip;

void
__eXosip_wakeup (void)
{
  jpipe_write (eXosip.j_socketctl, "w", 1);
}

void
__eXosip_wakeup_event (void)
{
  jpipe_write (eXosip.j_socketctl_event, "w", 1);
}


int
eXosip_lock (void)
{
  return osip_mutex_lock ((struct osip_mutex *) eXosip.j_mutexlock);
}

int
eXosip_unlock (void)
{
  return osip_mutex_unlock ((struct osip_mutex *) eXosip.j_mutexlock);
}

int
eXosip_transaction_find (int tid, osip_transaction_t ** transaction)
{
  int pos = 0;

  *transaction = NULL;
  while (!osip_list_eol (eXosip.j_transactions, pos))
    {
      osip_transaction_t *tr;

      tr = (osip_transaction_t *) osip_list_get (eXosip.j_transactions, pos);
      if (tr->transactionid == tid)
        {
          *transaction = tr;
          return 0;
        }
      pos++;
    }
  return -1;
}

static int
_eXosip_retry_with_auth (eXosip_dialog_t * jd, osip_transaction_t ** ptr,
                         int *retry)
{
  osip_transaction_t *out_tr = NULL;
  osip_transaction_t *tr = NULL;
  osip_message_t *msg = NULL;
  osip_event_t *sipevent;
  jinfo_t *ji = NULL;

  char locip[256];
  int cseq;
  char tmp[256];
  osip_via_t *via;
  int i;

  if (!ptr)
    return -1;

  if (jd != NULL)
    {
      if (jd->d_out_trs == NULL)
        return -1;
    }

  out_tr = *ptr;

  if (out_tr == NULL
      || out_tr->orig_request == NULL || out_tr->last_response == NULL)
    return -1;

  if (retry && (*retry >= 3))
    return -1;

  osip_message_clone (out_tr->orig_request, &msg);
  if (msg == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: could not clone msg for authentication\n"));
      return -1;
    }

  via = (osip_via_t *) osip_list_get (msg->vias, 0);
  if (via == NULL || msg->cseq == NULL || msg->cseq->number == NULL)
    {
      osip_message_free (msg);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: missing via or cseq header\n"));
      return -1;
    }

  /* increment cseq */
  cseq = atoi (msg->cseq->number);
  osip_free (msg->cseq->number);
  msg->cseq->number = strdup_printf ("%i", cseq + 1);
  if (jd != NULL && jd->d_dialog != NULL)
    {
      jd->d_dialog->local_cseq++;
    }

  osip_list_remove (msg->vias, 0);
  osip_via_free (via);
  i = _eXosip_find_protocol (out_tr->orig_request);
  if (i == IPPROTO_UDP)
    {
      eXosip_guess_ip_for_via (eXosip.net_interfaces[0].net_ip_family, locip,
                               sizeof (locip));
      if (eXosip.net_interfaces[0].net_ip_family == AF_INET6)
        snprintf (tmp, 256, "SIP/2.0/UDP [%s]:%s;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[0].net_port,
                  via_branch_new_random ());
      else
        snprintf (tmp, 256, "SIP/2.0/UDP %s:%s;rport;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[0].net_port,
                  via_branch_new_random ());
  } else if (i == IPPROTO_TCP)
    {
      eXosip_guess_ip_for_via (eXosip.net_interfaces[1].net_ip_family, locip,
                               sizeof (locip));
      if (eXosip.net_interfaces[1].net_ip_family == AF_INET6)
        snprintf (tmp, 256, "SIP/2.0/TCP [%s]:%s;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[1].net_port,
                  via_branch_new_random ());
      else
        snprintf (tmp, 256, "SIP/2.0/TCP %s:%s;rport;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[1].net_port,
                  via_branch_new_random ());
  } else
    {
      /* tls? */
      osip_message_free (msg);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: unsupported protocol\n"));
      return -1;
    }

  osip_via_init (&via);
  osip_via_parse (via, tmp);
  osip_list_add (msg->vias, via, 0);

  if (eXosip_add_authentication_information (msg, out_tr->last_response) < 0)
    {
      osip_message_free (msg);
      return -1;
    }

  osip_message_force_update (msg);

  if (MSG_IS_INVITE (msg))
    i = osip_transaction_init (&tr, ICT, eXosip.j_osip, msg);
  else
    i = osip_transaction_init (&tr, NICT, eXosip.j_osip, msg);

  if (i != 0)
    {
      osip_message_free (msg);
      return -1;
    }

  /* replace with the new tr */
  osip_list_add (eXosip.j_transactions, out_tr, 0);
  *ptr = tr;

  sipevent = osip_new_outgoing_sipmessage (msg);

  ji = osip_transaction_get_your_instance (out_tr);

  osip_transaction_set_your_instance (out_tr, NULL);
  osip_transaction_set_your_instance (tr, ji);
  osip_transaction_add_event (tr, sipevent);

  if (retry)
    (*retry)++;

  eXosip_update ();             /* fixed? */
  __eXosip_wakeup ();
  return 0;
}


static int
_eXosip_retry_register_with_auth (eXosip_event_t * je)
{
  eXosip_reg_t *jr = NULL;

  if (eXosip_reg_find_id (&jr, je->rid) < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: registration not found\n"));
      return -1;
    }

  return _eXosip_retry_with_auth (NULL, &jr->r_last_tr, &jr->r_retry);
}

static int
_eXosip_retry_invite_with_auth (eXosip_event_t * je)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;
  int *retry = NULL;

  if (eXosip_call_dialog_find (je->cid, &jc, &jd) < 0)
    if (eXosip_call_find (je->cid, &jc) < 0)
      {
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: call dialog not found\n"));
        return -1;
      }

  if (jd && jd->d_dialog)
    retry = &jd->d_retry;
  else
    retry = &jc->c_retry;

  return _eXosip_retry_with_auth (jd, &jc->c_out_tr, retry);
}

static int
_eXosip_redirect_invite (eXosip_event_t * je)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_call_t *jc = NULL;

  if (eXosip_call_dialog_find (je->cid, &jc, &jd) < 0)
    if (eXosip_call_find (je->cid, &jc) < 0)
      {
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: call dialog not found\n"));
        return -1;
      }

  if (!jc->c_out_tr || jc->c_out_tr->transactionid != je->tid)
    return -1;

  return _eXosip_call_redirect_request (jc, jd, jc->c_out_tr);
}

static int
_eXosip_retry_subscribe_with_auth (eXosip_event_t * je)
{
  eXosip_dialog_t *jd = NULL;
  eXosip_subscribe_t *js = NULL;
  int *retry = NULL;

  if (eXosip_subscribe_dialog_find (je->sid, &js, &jd) < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: subscribe dialog not found\n"));
      return -1;
    }

  if (jd && jd->d_dialog)
    retry = &jd->d_retry;
  else
    retry = &js->s_retry;

  return _eXosip_retry_with_auth (jd, &js->s_out_tr, retry);
}

static int
_eXosip_retry_publish_with_auth (eXosip_event_t * je)
{
  eXosip_pub_t *jp = NULL;

  if (_eXosip_pub_find_by_tid (&jp, je->tid) < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: publish transaction not found\n"));
      return -1;
    }

  return _eXosip_retry_with_auth (NULL, &jp->p_last_tr, NULL);
}

static int
_eXosip_retry_notify_with_auth (eXosip_event_t * je)
{
  /* TODO untested */
  eXosip_dialog_t *jd = NULL;
  eXosip_notify_t *jn = NULL;

  if (eXosip_notify_dialog_find (je->cid, &jn, &jd) < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: notify dialog not found\n"));
      return -1;
    }

  return _eXosip_retry_with_auth (jd, &jn->n_out_tr, NULL);
}

static int
eXosip_retry_with_auth (eXosip_event_t * je)
{
  if (!je || !je->request || !je->response)
    return -1;

  switch (je->type)
    {
      case EXOSIP_REGISTRATION_FAILURE:
        return _eXosip_retry_register_with_auth (je);

      case EXOSIP_CALL_REQUESTFAILURE:
        return _eXosip_retry_invite_with_auth (je);

      case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
        if (MSG_IS_PUBLISH (je->request))
          return _eXosip_retry_publish_with_auth (je);
        else if (MSG_IS_NOTIFY (je->request))
          return _eXosip_retry_notify_with_auth (je);

        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: not implemented\n"));
        return -1;

      case EXOSIP_MESSAGE_REQUESTFAILURE:
        if (MSG_IS_PUBLISH (je->request))
          return _eXosip_retry_publish_with_auth (je);

        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: not implemented\n"));
        return -1;

      case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
        return _eXosip_retry_subscribe_with_auth (je);

      default:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Can't retry event %d with auth\n", je->type));
        return -1;
    }
}

static int
_eXosip_redirect (eXosip_event_t * je)
{
  switch (je->type)
    {
      case EXOSIP_CALL_REDIRECTED:
        return _eXosip_redirect_invite (je);

      case EXOSIP_CALL_MESSAGE_REDIRECTED:
      case EXOSIP_MESSAGE_REDIRECTED:
      case EXOSIP_SUBSCRIPTION_REDIRECTED:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: not implemented\n"));
        return -1;

      default:
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_ERROR, NULL,
                     "eXosip: Can't redirect event %d\n", je->type));
        return -1;
    }
}


int
eXosip_default_action (eXosip_event_t * je)
{
  if (!je || !je->response)
    return -1;

  if (je->response->status_code == 401 || je->response->status_code == 407)
    return eXosip_retry_with_auth (je);
  else if (je->response->status_code >= 300 && je->response->status_code <= 399)
    return _eXosip_redirect (je);
  else
    return 1;
}


void
eXosip_automatic_refresh (void)
{
  eXosip_subscribe_t *js;
  eXosip_dialog_t *jd;

  eXosip_reg_t *jr;
  time_t now;

  now = time (NULL);

  for (js = eXosip.j_subscribes; js != NULL; js = js->next)
    {
      for (jd = js->s_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL && (jd->d_id >= 1))  /* finished call */
            {
              osip_transaction_t *out_tr = NULL;

              out_tr = osip_list_get (jd->d_out_trs, 0);
              if (out_tr == NULL)
                out_tr = js->s_out_tr;

              if (js->s_reg_period == 0 || out_tr == NULL)
                {
              } else if (now - out_tr->birth_time > js->s_reg_period - 60)
                {               /* will expires in 60 sec: send refresh! */
                  int i;

                  i =
                    _eXosip_subscribe_send_request_with_credential (js,
                                                                    jd, out_tr);
                  if (i != 0)
                    {
                      OSIP_TRACE (osip_trace
                                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                   "eXosip: could not clone subscribe for refresh\n"));
                    }
                }
            }
        }
    }

  for (jr = eXosip.j_reg; jr != NULL; jr = jr->next)
    {
      if (jr->r_id >= 1 && jr->r_last_tr != NULL)
        {
          if (jr->r_reg_period == 0)
            {
              /* skip refresh! */
          } else if (now - jr->r_last_tr->birth_time > 900)
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
          } else if (now - jr->r_last_tr->birth_time > jr->r_reg_period - 60)
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
          } else if (now - jr->r_last_tr->birth_time > 120 &&
                     (jr->r_last_tr->last_response == NULL
                      || (!MSG_IS_STATUS_2XX (jr->r_last_tr->last_response))))
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
            }
        }
    }
}

void
eXosip_automatic_action (void)
{
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  eXosip_dialog_t *jd;
  eXosip_notify_t *jn;

  eXosip_reg_t *jr;
  time_t now;

  now = time (NULL);

  for (jc = eXosip.j_calls; jc != NULL; jc = jc->next)
    {
      if (jc->c_id < 1)
        {
      } else if (jc->c_dialogs == NULL || jc->c_dialogs->d_dialog == NULL)
        {
          /* an EARLY dialog may have failed with 401,407 or 3Xx */

          osip_transaction_t *out_tr = NULL;

          out_tr = jc->c_out_tr;

          if (out_tr != NULL
              && (out_tr->state == ICT_TERMINATED
                  || out_tr->state == NICT_TERMINATED
                  || out_tr->state == ICT_COMPLETED
                  || out_tr->state == NICT_COMPLETED) &&
              now - out_tr->birth_time < 120 &&
              out_tr->orig_request != NULL &&
              out_tr->last_response != NULL &&
              (out_tr->last_response->status_code == 401
               || out_tr->last_response->status_code == 407))
            {
              /* retry with credential */
              if (jc->c_retry < 3)
                {
                  int i;

                  i = _eXosip_call_send_request_with_credential (jc, NULL, out_tr);
                  if (i != 0)
                    {
                      OSIP_TRACE (osip_trace
                                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                   "eXosip: could not clone msg for authentication\n"));
                    }
                  jc->c_retry++;
                }
          } else if (out_tr != NULL
                     && (out_tr->state == ICT_TERMINATED
                         || out_tr->state == NICT_TERMINATED
                         || out_tr->state == ICT_COMPLETED
                         || out_tr->state == NICT_COMPLETED) &&
                     now - out_tr->birth_time < 120 &&
                     out_tr->orig_request != NULL &&
                     out_tr->last_response != NULL &&
                     (out_tr->last_response->status_code >= 300
                      && out_tr->last_response->status_code <= 399))
            {
              /* retry with credential */
              int i;

              i = _eXosip_call_redirect_request (jc, NULL, out_tr);
              if (i != 0)
                {
                  OSIP_TRACE (osip_trace
                              (__FILE__, __LINE__, OSIP_ERROR, NULL,
                               "eXosip: could not clone msg for redirection\n"));
                }
            }
        }

      for (jd = jc->c_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog == NULL)     /* finished call */
            {
          } else
            {
              osip_transaction_t *out_tr = NULL;

              out_tr = osip_list_get (jd->d_out_trs, 0);
              if (out_tr == NULL)
                out_tr = jc->c_out_tr;

              if (out_tr != NULL
                  && (out_tr->state == ICT_TERMINATED
                      || out_tr->state == NICT_TERMINATED
                      || out_tr->state == ICT_COMPLETED
                      || out_tr->state == NICT_COMPLETED) &&
                  now - out_tr->birth_time < 120 &&
                  out_tr->orig_request != NULL &&
                  out_tr->last_response != NULL &&
                  (out_tr->last_response->status_code == 401
                   || out_tr->last_response->status_code == 407))
                {
                  /* retry with credential */
                  if (jd->d_retry < 3)
                    {
                      int i;

                      i =
                        _eXosip_call_send_request_with_credential (jc, jd, out_tr);
                      if (i != 0)
                        {
                          OSIP_TRACE (osip_trace
                                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                       "eXosip: could not clone msg for authentication\n"));
                        }
                      jd->d_retry++;
                    }
              } else if (out_tr != NULL
                         && (out_tr->state == ICT_TERMINATED
                             || out_tr->state == NICT_TERMINATED
                             || out_tr->state == ICT_COMPLETED
                             || out_tr->state == NICT_COMPLETED) &&
                         now - out_tr->birth_time < 120 &&
                         out_tr->orig_request != NULL &&
                         out_tr->last_response != NULL &&
                         (out_tr->last_response->status_code >= 300
                          && out_tr->last_response->status_code <= 399))
                {
                  /* retry with credential */
                  int i;

                  i = _eXosip_call_redirect_request (jc, jd, out_tr);
                  if (i != 0)
                    {
                      OSIP_TRACE (osip_trace
                                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                   "eXosip: could not clone msg for redirection\n"));
                    }
                }
            }
        }
    }

  for (js = eXosip.j_subscribes; js != NULL; js = js->next)
    {
      if (js->s_id < 1)
        {
      } else if (js->s_dialogs == NULL)
        {
          osip_transaction_t *out_tr = NULL;

          out_tr = js->s_out_tr;

          if (out_tr != NULL
              && (out_tr->state == NICT_TERMINATED
                  || out_tr->state == NICT_COMPLETED) &&
              now - out_tr->birth_time < 120 &&
              out_tr->orig_request != NULL &&
              out_tr->last_response != NULL &&
              (out_tr->last_response->status_code == 401
               || out_tr->last_response->status_code == 407))
            {
              /* retry with credential */
              if (js->s_retry < 3)
                {
                  int i;

                  i =
                    _eXosip_subscribe_send_request_with_credential (js, NULL,
                                                                    out_tr);
                  if (i != 0)
                    {
                      OSIP_TRACE (osip_trace
                                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                   "eXosip: could not clone msg for authentication\n"));
                    }
                  js->s_retry++;
                }
            }
        }

      for (jd = js->s_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL)     /* finished call */
            {
              if (jd->d_id >= 1)
                {
                  osip_transaction_t *out_tr = NULL;

                  out_tr = osip_list_get (jd->d_out_trs, 0);
                  if (out_tr == NULL)
                    out_tr = js->s_out_tr;

                  if (out_tr != NULL
                      && (out_tr->state == NICT_TERMINATED
                          || out_tr->state == NICT_COMPLETED) &&
                      now - out_tr->birth_time < 120 &&
                      out_tr->orig_request != NULL &&
                      out_tr->last_response != NULL &&
                      (out_tr->last_response->status_code == 401
                       || out_tr->last_response->status_code == 407))
                    {
                      /* retry with credential */
                      if (jd->d_retry < 3)
                        {
                          int i;

                          i =
                            _eXosip_subscribe_send_request_with_credential
                            (js, jd, out_tr);
                          if (i != 0)
                            {
                              OSIP_TRACE (osip_trace
                                          (__FILE__, __LINE__, OSIP_ERROR,
                                           NULL,
                                           "eXosip: could not clone suscbribe for authentication\n"));
                            }
                          jd->d_retry++;
                        }
                  } else if (js->s_reg_period == 0 || out_tr == NULL)
                    {
                  } else if (now - out_tr->birth_time > js->s_reg_period - 60)
                    {           /* will expires in 60 sec: send refresh! */
                      int i;

                      i =
                        _eXosip_subscribe_send_request_with_credential (js,
                                                                        jd,
                                                                        out_tr);
                      if (i != 0)
                        {
                          OSIP_TRACE (osip_trace
                                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                       "eXosip: could not clone subscribe for refresh\n"));
                        }
                    }
                }
            }
        }
    }


  for (jn = eXosip.j_notifies; jn != NULL; jn = jn->next)
    {
      for (jd = jn->n_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL)     /* finished call */
            {
              if (jd->d_id >= 1)
                {
                  osip_transaction_t *out_tr = NULL;

                  out_tr = osip_list_get (jd->d_out_trs, 0);

                  if (out_tr != NULL
                      && (out_tr->state == NICT_TERMINATED
                          || out_tr->state == NICT_COMPLETED) &&
                      now - out_tr->birth_time < 120 &&
                      out_tr->orig_request != NULL &&
                      out_tr->last_response != NULL &&
                      (out_tr->last_response->status_code == 401
                       || out_tr->last_response->status_code == 407))
                    {
                      /* retry with credential */
                      if (jd->d_retry < 3)
                        {
                          int i;

                          i =
                            _eXosip_insubscription_send_request_with_credential
                            (jn, jd, out_tr);
                          if (i != 0)
                            {
                              OSIP_TRACE (osip_trace
                                          (__FILE__, __LINE__, OSIP_ERROR,
                                           NULL,
                                           "eXosip: could not clone notify for authentication\n"));
                            }
                          jd->d_retry++;
                        }
                    }
                }
            }
        }
    }


  for (jr = eXosip.j_reg; jr != NULL; jr = jr->next)
    {
      if (jr->r_id >= 1 && jr->r_last_tr != NULL)
        {
          if (jr->r_reg_period != 0 && now - jr->r_last_tr->birth_time > 900)
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
          } else if (jr->r_reg_period != 0
                     && now - jr->r_last_tr->birth_time > jr->r_reg_period - 60)
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
          } else if (jr->r_reg_period != 0
                     && now - jr->r_last_tr->birth_time > 120
                     && (jr->r_last_tr->last_response == NULL
                         || (!MSG_IS_STATUS_2XX (jr->r_last_tr->last_response))))
            {
              /* automatic refresh */
              eXosip_register_send_register (jr->r_id, NULL);
          } else if (now - jr->r_last_tr->birth_time < 120 &&
                     jr->r_last_tr->orig_request != NULL &&
                     (jr->r_last_tr->last_response != NULL
                      && (jr->r_last_tr->last_response->status_code == 401
                          || jr->r_last_tr->last_response->status_code == 407)))
            {
              if (jr->r_retry < 3)
                {
                  /* TODO: improve support for several retries when
                     several credentials are needed */
                  eXosip_register_send_register (jr->r_id, NULL);
                  jr->r_retry++;
                }
            }
        }
    }
}

void
eXosip_update ()
{
  static int static_id = 1;
  eXosip_call_t *jc;
  eXosip_subscribe_t *js;
  eXosip_notify_t *jn;
  eXosip_dialog_t *jd;
  time_t now;

  if (static_id > 100000)
    static_id = 1;              /* loop */

  now = time (NULL);
  for (jc = eXosip.j_calls; jc != NULL; jc = jc->next)
    {
      if (jc->c_id < 1)
        {
          jc->c_id = static_id;
          static_id++;
        }
      for (jd = jc->c_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL)     /* finished call */
            {
              if (jd->d_id < 1)
                {
                  jd->d_id = static_id;
                  static_id++;
                }
          } else
            jd->d_id = -1;
        }
    }

  for (js = eXosip.j_subscribes; js != NULL; js = js->next)
    {
      if (js->s_id < 1)
        {
          js->s_id = static_id;
          static_id++;
        }
      for (jd = js->s_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL)     /* finished call */
            {
              if (jd->d_id < 1)
                {
                  jd->d_id = static_id;
                  static_id++;
                }
          } else
            jd->d_id = -1;
        }
    }

  for (jn = eXosip.j_notifies; jn != NULL; jn = jn->next)
    {
      if (jn->n_id < 1)
        {
          jn->n_id = static_id;
          static_id++;
        }
      for (jd = jn->n_dialogs; jd != NULL; jd = jd->next)
        {
          if (jd->d_dialog != NULL)     /* finished call */
            {
              if (jd->d_id < 1)
                {
                  jd->d_id = static_id;
                  static_id++;
                }
          } else
            jd->d_id = -1;
        }
    }
}

static jauthinfo_t *
eXosip_find_authentication_info (const char *username, const char *realm)
{
  jauthinfo_t *fallback = NULL;
  jauthinfo_t *authinfo;

  for (authinfo = eXosip.authinfos; authinfo != NULL; authinfo = authinfo->next)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "INFO: authinfo: %s %s\n", realm, authinfo->realm));
      if (0 == strcmp (authinfo->username, username))
        {
          if (authinfo->realm == NULL || authinfo->realm[0] == '\0')
            {
              fallback = authinfo;
          } else if (strcmp (realm, authinfo->realm) == 0
                     || 0 == strncmp (realm + 1, authinfo->realm,
                                      strlen (realm) - 2))
            {
              return authinfo;
            }
        }
    }

  /* no matching username has been found for this realm,
     try with another username... */
  for (authinfo = eXosip.authinfos; authinfo != NULL; authinfo = authinfo->next)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "INFO: authinfo: %s %s\n", realm, authinfo->realm));
      if (authinfo->realm == NULL || authinfo->realm[0] == '\0')
        {
          fallback = authinfo;
      } else if (strcmp (realm, authinfo->realm) == 0
                 || 0 == strncmp (realm + 1, authinfo->realm, strlen (realm) - 2))
        {
          return authinfo;
        }
    }

  return fallback;
}


int
eXosip_clear_authentication_info ()
{
  jauthinfo_t *jauthinfo;

  for (jauthinfo = eXosip.authinfos; jauthinfo != NULL;
       jauthinfo = eXosip.authinfos)
    {
      REMOVE_ELEMENT (eXosip.authinfos, jauthinfo);
      osip_free (jauthinfo);
    }
  return 0;
}

int
eXosip_add_authentication_info (const char *username, const char *userid,
                                const char *passwd, const char *ha1,
                                const char *realm)
{
  jauthinfo_t *authinfos;

  if (username == NULL || username[0] == '\0')
    return -1;
  if (userid == NULL || userid[0] == '\0')
    return -1;

  if (passwd != NULL && passwd[0] != '\0')
    {
  } else if (ha1 != NULL && ha1[0] != '\0')
    {
  } else
    return -1;

  authinfos = (jauthinfo_t *) osip_malloc (sizeof (jauthinfo_t));
  if (authinfos == NULL)
    return -1;
  memset (authinfos, 0, sizeof (jauthinfo_t));

  snprintf (authinfos->username, 50, "%s", username);
  snprintf (authinfos->userid, 50, "%s", userid);
  if (passwd != NULL && passwd[0] != '\0')
    snprintf (authinfos->passwd, 50, "%s", passwd);
  else if (ha1 != NULL && ha1[0] != '\0')
    snprintf (authinfos->ha1, 50, "%s", ha1);
  if (realm != NULL && realm[0] != '\0')
    snprintf (authinfos->realm, 50, "%s", realm);

  ADD_ELEMENT (eXosip.authinfos, authinfos);
  return 0;
}

int
eXosip_add_authentication_information (osip_message_t * req,
                                       osip_message_t * last_response)
{
  osip_authorization_t *aut = NULL;
  osip_www_authenticate_t *wwwauth = NULL;
  osip_proxy_authorization_t *proxy_aut = NULL;
  osip_proxy_authenticate_t *proxyauth = NULL;
  jauthinfo_t *authinfo = NULL;
  int pos;
  int i;

  if (req == NULL
      || req->from == NULL
      || req->from->url == NULL || req->from->url->username == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "authinfo: Invalid message\n"));
      return -1;
    }

  pos = 0;
  osip_message_get_www_authenticate (last_response, pos, &wwwauth);
  osip_message_get_proxy_authenticate (last_response, pos, &proxyauth);
  if (wwwauth == NULL && proxyauth == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "authinfo: No WWW-Authenticate or Proxy-Authenticate\n"));
      return -1;
    }

  while (wwwauth != NULL)
    {
      char *uri;

      authinfo = eXosip_find_authentication_info (req->from->url->username,
                                                  wwwauth->realm);
      if (authinfo == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "authinfo: No authentication found for %s %s\n",
                       req->from->url->username, wwwauth->realm));
          return -1;
        }
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "authinfo: %s\n", authinfo->username));
      i = osip_uri_to_str (req->req_uri, &uri);
      if (i != 0)
        return -1;

      i = __eXosip_create_authorization_header (last_response, uri,
                                                authinfo->userid,
                                                authinfo->passwd,
                                                authinfo->ha1, &aut,
                                                req->sip_method);
      osip_free (uri);
      if (i != 0)
        return -1;

      if (aut != NULL)
        {
          osip_list_add (req->authorizations, aut, -1);
          osip_message_force_update (req);
        }

      pos++;
      osip_message_get_www_authenticate (last_response, pos, &wwwauth);
    }

  pos = 0;
  while (proxyauth != NULL)
    {
      char *uri;

      authinfo = eXosip_find_authentication_info (req->from->url->username,
                                                  proxyauth->realm);
      if (authinfo == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "authinfo: No authentication found for %s %s\n",
                       req->from->url->username, proxyauth->realm));
          return -1;
        }
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "authinfo: %s\n", authinfo->username));
      i = osip_uri_to_str (req->req_uri, &uri);
      if (i != 0)
        return -1;

      i = __eXosip_create_proxy_authorization_header (last_response, uri,
                                                      authinfo->userid,
                                                      authinfo->passwd,
                                                      authinfo->ha1,
                                                      &proxy_aut, req->sip_method);
      osip_free (uri);
      if (i != 0)
        return -1;

      if (proxy_aut != NULL)
        {
          osip_list_add (req->proxy_authorizations, proxy_aut, -1);
          osip_message_force_update (req);
        }

      pos++;
      osip_message_get_proxy_authenticate (last_response, pos, &proxyauth);
    }

  return 0;
}

int
eXosip_update_top_via (osip_message_t * sip)
{
  char locip[50];
  char *tmp = (char *) osip_malloc (256 * sizeof (char));
  osip_via_t *via = (osip_via_t *) osip_list_get (sip->vias, 0);
  int i;

  i = _eXosip_find_protocol (sip);

  osip_list_remove (sip->vias, 0);
  osip_via_free (via);
#ifdef SM
  eXosip_get_localip_for (sip->req_uri->host, locip, 49);
#else
  if (i == IPPROTO_UDP)
    eXosip_guess_ip_for_via (eXosip.net_interfaces[0].net_ip_family, locip, 49);
  else if (i == IPPROTO_TCP)
    eXosip_guess_ip_for_via (eXosip.net_interfaces[1].net_ip_family, locip, 49);
  else
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: unsupported protocol (using default UDP)\n"));
      eXosip_guess_ip_for_via (eXosip.net_interfaces[0].net_ip_family, locip, 49);
    }
#endif
  if (i == IPPROTO_UDP)
    {
      if (eXosip.net_interfaces[0].net_ip_family == AF_INET6)
        snprintf (tmp, 256, "SIP/2.0/UDP [%s]:%s;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[0].net_port,
                  via_branch_new_random ());
      else
        snprintf (tmp, 256, "SIP/2.0/UDP %s:%s;rport;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[0].net_port,
                  via_branch_new_random ());
  } else if (i == IPPROTO_TCP)
    {
      if (eXosip.net_interfaces[1].net_ip_family == AF_INET6)
        snprintf (tmp, 256, "SIP/2.0/TCP [%s]:%s;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[1].net_port,
                  via_branch_new_random ());
      else
        snprintf (tmp, 256, "SIP/2.0/TCP %s:%s;rport;branch=z9hG4bK%u",
                  locip, eXosip.net_interfaces[1].net_port,
                  via_branch_new_random ());
    }

  osip_via_init (&via);
  osip_via_parse (via, tmp);
  osip_list_add (sip->vias, via, 0);
  osip_free (tmp);

  return 0;
}
