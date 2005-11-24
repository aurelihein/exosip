/*
  The antisipc program is a modular SIP softphone (SIP -rfc3261-)
  Copyright (C) 2005  Aymeric MOIZARD - <jack@atosc.org>
*/


#include "jcalls.h"
#include <eXosip2/eXosip.h>
#include "sdptools.h"

call_t calls[MAX_NUMBER_OF_CALLS];
static int ___call_init = 0;

static int
__call_init ()
{
  int k;

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      memset (&(calls[k]), 0, sizeof (call_t));
      calls[k].state = NOT_USED;
      snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");
    }

#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
  return os_sound_init ();
#else
  return 0;
#endif
}

#if defined(ORTP_SUPPORT)
void
rcv_telephone_event (RtpSession * rtp_session, call_t * ca)
{
  /* telephone-event received! */

}
#endif

int
call_get_number_of_pending_calls (void)
{
  int pos = 0;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED)
        {
          pos++;
        }
    }
  return pos;
}


call_t *
call_get_call (int pos)
{
  return &calls[pos];
}


int
call_get_callpos (call_t * ca)
{
  return ca - calls;
}

call_t *
call_locate_call_by_cid (int cid)
{
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED && calls[k].cid == cid)
        return &calls[k];
    }

  return 0;
}


call_t *
call_create_call (int cid)
{
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state == NOT_USED)
        {
          memset (&(calls[k]), 0, sizeof (call_t));
          snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");
          calls[k].cid = cid;
          calls[k].did = -1;
          calls[k].state = -1;
          return &calls[k];
        }

    }

  return 0;
}




call_t *
call_locate_call (eXosip_event_t * je, int createit)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    {
      if (!createit)
        return 0;
      for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
        {
          if (calls[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_CALLS)
        return NULL;

      ca = &(calls[k]);
      memset (&(calls[k]), 0, sizeof (call_t));
      snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");

      ca->cid = je->cid;
      ca->did = je->did;

      if (ca->did < 1 && ca->cid < 1)
        {
          exit (0);
          return NULL;          /* not enough information for this event?? */
        }
    }


  ca = &(calls[k]);
  osip_strncpy (ca->textinfo, je->textinfo, 255);

  ca->state = je->type;
  return ca;

}


call_t *
call_find_call (int pos)
{
  int k;

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED)
        {
          if (pos == 0)
            return &(calls[k]);
          pos--;
        }
    }
  return NULL;
}

int
call_new (eXosip_event_t * je)
{
  sdp_message_t *remote_sdp = NULL;
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state == NOT_USED)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);
  memset (&(calls[k]), 0, sizeof (call_t));
  snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");

  ca->cid = je->cid;
  ca->did = je->did;
  ca->tid = je->tid;

  if (ca->did < 1 && ca->cid < 1)
    {
      return -1;                /* not enough information for this event?? */
    }

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
  os_sound_init ();
#endif

  /* negotiate payloads */
  if (je->request != NULL)
    {
      remote_sdp = eXosip_get_sdp_info (je->request);
    }

  if (remote_sdp == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_WARNING, NULL,
                   "missing SDP in INVITE request\n"));
    }

  if (remote_sdp != NULL)       /* TODO: else build an offer */
    {
      sdp_connection_t *conn;
      sdp_media_t *remote_med;
      char *tmp = NULL;

      if (remote_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "No remote SDP body found for call\n"));
          eXosip_call_send_answer (ca->tid, 400, NULL);
          sdp_message_free (remote_sdp);
          return 0;
        }

      conn = eXosip_get_audio_connection (remote_sdp);
      if (conn != NULL && conn->c_addr != NULL)
        {
          snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
        }
      remote_med = eXosip_get_audio_media (remote_sdp);

      if (remote_med == NULL || remote_med->m_port == NULL)
        {
          /* no audio media proposed */
          eXosip_call_send_answer (ca->tid, 415, NULL);
          sdp_message_free (remote_sdp);
          return 0;
        }

      ca->remote_sdp_audio_port = atoi (remote_med->m_port);

      if (ca->remote_sdp_audio_port > 0 && ca->remote_sdp_audio_ip[0] != '\0')
        {
          int pos;

          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "audio connection found: %s:%i\n",
                       ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));
          pos = 0;
          while (!osip_list_eol (remote_med->m_payloads, pos))
            {
              tmp = (char *) osip_list_get (remote_med->m_payloads, pos);
              if (tmp != NULL &&
                  (0 == osip_strcasecmp (tmp, "0")
                   || 0 == osip_strcasecmp (tmp, "8")))
                {
                  break;
                }
              tmp = NULL;
              pos++;
            }
        }
      if (tmp != NULL)
        {
          ca->payload = atoi (tmp);
      } else
        {
          eXosip_call_send_answer (ca->tid, 415, NULL);
          sdp_message_free (remote_sdp);
          return 0;
        }

      if (tmp != NULL
          && (ca->payload == 0 || ca->payload == 8)
          && ca->remote_sdp_audio_port > 0 && ca->remote_sdp_audio_ip[0] != '\0')

        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "audio connection: (payload=%i) -> %s:%i\n",
                       ca->payload,
                       ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));
        }
    }
#ifndef TEST_PRACK_SUPPORT
  eXosip_lock ();
  eXosip_call_send_answer (ca->tid, 180, NULL);
  eXosip_unlock ();
#else

  if (remote_sdp != NULL)       /* TODO: else build an offer */
    {
      osip_message_t *answer;
      int i;

      eXosip_lock ();
      i = eXosip_call_build_answer (ca->tid, 183, &answer);
      if (i == 0)
        {

          osip_message_set_require (answer, "100rel");
          osip_message_set_header (answer, "RSeq", "1");

          i = sdp_complete_message (ca->did, remote_sdp, answer);
          if (i != 0)
            {
              osip_message_free (answer);
              eXosip_call_send_answer (ca->tid, 415, NULL);
          } else
            {
              /* start sending audio */
              if (ca->enable_audio > 0)
                {
                  ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
                  os_sound_close (ca);
#endif
                }

              if (ca->enable_audio != 1)        /* audio is started */
                {
                  sdp_message_t *local_sdp;

                  local_sdp = eXosip_get_sdp_info (answer);
                  if (remote_sdp != NULL && local_sdp != NULL)
                    {
                      sdp_connection_t *conn;
                      sdp_media_t *local_med;
                      sdp_media_t *remote_med;
                      char *tmp = NULL;
                      int audio_port = 0;

                      conn = eXosip_get_audio_connection (remote_sdp);
                      if (conn != NULL && conn->c_addr != NULL)
                        {
                          snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
                        }
                      remote_med = eXosip_get_audio_media (remote_sdp);
                      if (remote_med != NULL && remote_med->m_port != NULL)
                        {
                          ca->remote_sdp_audio_port = atoi (remote_med->m_port);
                        }
                      local_med = eXosip_get_audio_media (local_sdp);
                      if (local_med != NULL && local_med->m_port != NULL)
                        {
                          audio_port = atoi (local_med->m_port);
                        }

                      if (ca->remote_sdp_audio_port > 0
                          && ca->remote_sdp_audio_ip[0] != '\0'
                          && local_med != NULL)
                        {
                          OSIP_TRACE (osip_trace
                                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                                       "audio connection found: %s:%i\n",
                                       ca->remote_sdp_audio_ip,
                                       ca->remote_sdp_audio_port));

                          tmp = (char *) osip_list_get (local_med->m_payloads, 0);
                        }
                      if (tmp != NULL)
                        {
                          ca->payload = atoi (tmp);
                        }
                      if (tmp != NULL
                          && audio_port > 0
                          && ca->remote_sdp_audio_port > 0
                          && ca->remote_sdp_audio_ip[0] != '\0')

                        {
                          OSIP_TRACE (osip_trace
                                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                                       "audio connection: (payload=%i) %i -> %s:%i\n",
                                       ca->payload,
                                       audio_port,
                                       ca->remote_sdp_audio_ip,
                                       ca->remote_sdp_audio_port));

                          /* search if stream is sendonly or recvonly */
                          ca->remote_sendrecv =
                            _sdp_analyse_attribute (remote_sdp, remote_med);
                          ca->local_sendrecv =
                            _sdp_analyse_attribute (local_sdp, local_med);
                          if (ca->local_sendrecv == _SENDRECV)
                            {
                              if (ca->remote_sendrecv == _SENDONLY)
                                ca->local_sendrecv = _RECVONLY;
                              else if (ca->remote_sendrecv == _RECVONLY)
                                ca->local_sendrecv = _SENDONLY;
                            }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
                          if (0 == os_sound_start (ca, audio_port))
                            {
                              ca->enable_audio = 1;     /* audio is started */
                            }
#endif
                        }
                    }
                  sdp_message_free (local_sdp);
                }

              i = eXosip_call_send_answer (ca->tid, 183, answer);
            }

          if (i != 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "cannot send 183 progress?\n"));
            }
        }
      eXosip_unlock ();
    }
#endif
  sdp_message_free (remote_sdp);

  ca->state = je->type;
  return 0;
}

int
call_ack (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);

  if (je->ack != NULL)
    {
      sdp_message_t *remote_sdp;

      remote_sdp = eXosip_get_sdp_info (je->ack);
      if (remote_sdp != NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "SDP detected in ACK!\n"));
      } else
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "no SDP detected in ACK!\n"));
        }
    }

  if (ca->enable_audio != 1)    /* audio is started */
    {
      sdp_message_t *remote_sdp;
      sdp_message_t *local_sdp;

      remote_sdp = eXosip_get_remote_sdp (ca->did);
      local_sdp = eXosip_get_local_sdp (ca->did);
      if (remote_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "No remote SDP body found for call\n"));
        }
      if (remote_sdp != NULL && local_sdp != NULL)
        {
          sdp_connection_t *conn;
          sdp_media_t *local_med;
          sdp_media_t *remote_med;
          char *tmp = NULL;
          int audio_port = 0;

          conn = eXosip_get_audio_connection (remote_sdp);
          if (conn != NULL && conn->c_addr != NULL)
            {
              snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
            }
          remote_med = eXosip_get_audio_media (remote_sdp);
          if (remote_med != NULL && remote_med->m_port != NULL)
            {
              ca->remote_sdp_audio_port = atoi (remote_med->m_port);
            }
          local_med = eXosip_get_audio_media (local_sdp);
          if (local_med != NULL && local_med->m_port != NULL)
            {
              audio_port = atoi (local_med->m_port);
            }

          if (ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0' && local_med != NULL)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection found: %s:%i\n",
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              tmp = (char *) osip_list_get (local_med->m_payloads, 0);
            }
          if (tmp != NULL)
            {
              ca->payload = atoi (tmp);
            }
          if (tmp != NULL
              && audio_port > 0
              && ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0')

            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection: (payload=%i) %i -> %s:%i\n",
                           ca->payload,
                           audio_port,
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              /* search if stream is sendonly or recvonly */
              ca->remote_sendrecv =
                _sdp_analyse_attribute (remote_sdp, remote_med);
              ca->local_sendrecv = _sdp_analyse_attribute (local_sdp, local_med);
              if (ca->local_sendrecv == _SENDRECV)
                {
                  if (ca->remote_sendrecv == _SENDONLY)
                    ca->local_sendrecv = _RECVONLY;
                  else if (ca->remote_sendrecv == _RECVONLY)
                    ca->local_sendrecv = _SENDONLY;
                }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
              if (0 == os_sound_start (ca, audio_port))
                {
                  ca->enable_audio = 1; /* audio is started */
                }
#endif
            }
        }
      sdp_message_free (local_sdp);
      sdp_message_free (remote_sdp);
    }

  ca->state = je->type;
  return 0;
}

int
call_remove (call_t * ca)
{
  if (ca == NULL)
    return -1;


  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_proceeding (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    {
      for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
        {
          if (calls[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_CALLS)
        return -1;

      ca = &(calls[k]);
      memset (&(calls[k]), 0, sizeof (call_t));
      snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");

      ca->cid = je->cid;
      ca->did = je->did;

      if (ca->did < 1 || ca->cid < 1)
        {
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(calls[k]);
  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = je->type;
  return 0;

}

int
call_ringing (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    {
      for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
        {
          if (calls[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_CALLS)
        return -1;
      ca = &(calls[k]);
      memset (&(calls[k]), 0, sizeof (call_t));
      snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");

      ca->cid = je->cid;
      ca->did = je->did;

      if (ca->did < 1 || ca->cid < 1)
        {
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(calls[k]);

  ca->cid = je->cid;
  ca->did = je->did;
  ca->tid = je->tid;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  if (ca->enable_audio == 1 && je->response != NULL)
    {
      sdp_message_t *sdp = eXosip_get_sdp_info (je->response);

      if (sdp != NULL)
        {
          /* audio is started and session may just have been modified */
          sdp_message_free (sdp);
        }
    }

  {
    osip_header_t *rseq;

    osip_message_header_get_byname (je->response, "RSeq", 0, &rseq);
    if (rseq != NULL && rseq->hvalue != NULL)
      {
        /* try sending a PRACK */
        osip_message_t *prack = NULL;
        int i;

        eXosip_lock ();
        i = eXosip_call_build_prack (ca->tid, &prack);
        if (i != 0)
          {
            OSIP_TRACE (osip_trace
                        (__FILE__, __LINE__, OSIP_WARNING, NULL,
                         "Failed to build PRACK request\n"));
        } else
          {
            eXosip_call_send_prack (ca->tid, prack);
          }

        eXosip_unlock ();
      }
  }

  if (ca->enable_audio != 1)    /* audio is started */
    {
      sdp_message_t *remote_sdp;
      sdp_message_t *local_sdp;

      local_sdp = eXosip_get_sdp_info (je->request);
      remote_sdp = eXosip_get_sdp_info (je->response);
      if (remote_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "No remote SDP body found for call\n"));
          /* TODO: remote_sdp = retreive from ack above */
        }
      if (local_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "SDP body was probably in the ACK (TODO)\n"));
        }

      if (remote_sdp != NULL && local_sdp != NULL)
        {
          sdp_connection_t *conn;
          sdp_media_t *local_med;
          sdp_media_t *remote_med;
          char *tmp = NULL;
          int audio_port = 0;

          conn = eXosip_get_audio_connection (remote_sdp);
          if (conn != NULL && conn->c_addr != NULL)
            {
              snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
            }
          remote_med = eXosip_get_audio_media (remote_sdp);
          if (remote_med != NULL && remote_med->m_port != NULL)
            {
              ca->remote_sdp_audio_port = atoi (remote_med->m_port);
            }
          local_med = eXosip_get_audio_media (local_sdp);
          if (local_med != NULL && local_med->m_port != NULL)
            {
              audio_port = atoi (local_med->m_port);
            }

          if (ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0' && remote_med != NULL)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection found: %s:%i\n",
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              tmp = (char *) osip_list_get (remote_med->m_payloads, 0);
            }
          if (tmp != NULL)
            {
              ca->payload = atoi (tmp);
            }
          if (tmp != NULL
              && audio_port > 0
              && ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0')

            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection: (payload=%i) %i -> %s:%i\n",
                           ca->payload,
                           audio_port,
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              /* search if stream is sendonly or recvonly */
              ca->remote_sendrecv =
                _sdp_analyse_attribute (remote_sdp, remote_med);
              ca->local_sendrecv = _sdp_analyse_attribute (local_sdp, local_med);
              if (ca->local_sendrecv == _SENDRECV)
                {
                  if (ca->remote_sendrecv == _SENDONLY)
                    ca->local_sendrecv = _RECVONLY;
                  else if (ca->remote_sendrecv == _RECVONLY)
                    ca->local_sendrecv = _SENDONLY;
                }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
              if (0 == os_sound_start (ca, audio_port))
                {
                  ca->enable_audio = 1; /* audio is started */
                }
#endif
            }
        }
      sdp_message_free (local_sdp);
      sdp_message_free (remote_sdp);
    }

  ca->state = je->type;;
  return 0;
}

int
call_answered (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    {
      for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
        {
          if (calls[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_CALLS)
        return -1;
      ca = &(calls[k]);
      memset (&(calls[k]), 0, sizeof (call_t));
      snprintf (calls[k].wav_file, 256, "%s", "ringback.wav");

      ca->cid = je->cid;
      ca->did = je->did;

      if (ca->did < 1 && ca->cid < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(calls[k]);
  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }
  eXosip_lock ();
  {
    osip_message_t *ack = NULL;
    int i;

    i = eXosip_call_build_ack (ca->did, &ack);
    if (i != 0)
      {
        OSIP_TRACE (osip_trace
                    (__FILE__, __LINE__, OSIP_WARNING, NULL,
                     "Cannot build ACK for call!\n"));
    } else
      {
        sdp_message_t *local_sdp = NULL;
        sdp_message_t *remote_sdp = NULL;

        if (je->request != NULL && je->response != NULL)
          {
            local_sdp = eXosip_get_sdp_info (je->request);
            remote_sdp = eXosip_get_sdp_info (je->response);
          }
        if (local_sdp == NULL && remote_sdp != NULL)
          {
            /* sdp in ACK */
            i = sdp_complete_message (ca->did, remote_sdp, ack);
            if (i != 0)
              {
                OSIP_TRACE (osip_trace
                            (__FILE__, __LINE__, OSIP_WARNING, NULL,
                             "Cannot complete ACK with sdp body?!\n"));
              }
          }
        sdp_message_free (local_sdp);
        sdp_message_free (remote_sdp);

        eXosip_call_send_ack (ca->did, ack);
      }
  }
  eXosip_unlock ();

  if (ca->enable_audio == 1 && je->response != NULL)
    {
      sdp_message_t *sdp = eXosip_get_sdp_info (je->response);

      if (sdp != NULL)
        {
          /* audio is started and session has just been modified */
          ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
          os_sound_close (ca);
#endif
          sdp_message_free (sdp);
        }
    }

  if (ca->enable_audio != 1)    /* audio is started */
    {
      sdp_message_t *remote_sdp;
      sdp_message_t *local_sdp;

      local_sdp = eXosip_get_sdp_info (je->request);
      remote_sdp = eXosip_get_sdp_info (je->response);
      if (remote_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "No remote SDP body found for call\n"));
          /* TODO: remote_sdp = retreive from ack above */
        }
      if (local_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "SDP body was probably in the ACK (TODO)\n"));
        }

      if (remote_sdp != NULL && local_sdp != NULL)
        {
          sdp_connection_t *conn;
          sdp_media_t *local_med;
          sdp_media_t *remote_med;
          char *tmp = NULL;
          int audio_port = 0;

          conn = eXosip_get_audio_connection (remote_sdp);
          if (conn != NULL && conn->c_addr != NULL)
            {
              snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
            }
          remote_med = eXosip_get_audio_media (remote_sdp);
          if (remote_med != NULL && remote_med->m_port != NULL)
            {
              ca->remote_sdp_audio_port = atoi (remote_med->m_port);
            }
          local_med = eXosip_get_audio_media (local_sdp);
          if (local_med != NULL && local_med->m_port != NULL)
            {
              audio_port = atoi (local_med->m_port);
            }

          if (ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0' && remote_med != NULL)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection found: %s:%i\n",
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              tmp = (char *) osip_list_get (remote_med->m_payloads, 0);
            }
          if (tmp != NULL)
            {
              ca->payload = atoi (tmp);
            }
          if (tmp != NULL
              && audio_port > 0
              && ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0')

            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection: (payload=%i) %i -> %s:%i\n",
                           ca->payload,
                           audio_port,
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              /* search if stream is sendonly or recvonly */
              ca->remote_sendrecv =
                _sdp_analyse_attribute (remote_sdp, remote_med);
              ca->local_sendrecv = _sdp_analyse_attribute (local_sdp, local_med);
              if (ca->local_sendrecv == _SENDRECV)
                {
                  if (ca->remote_sendrecv == _SENDONLY)
                    ca->local_sendrecv = _RECVONLY;
                  else if (ca->remote_sendrecv == _RECVONLY)
                    ca->local_sendrecv = _SENDONLY;
                }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
              if (0 == os_sound_start (ca, audio_port))
                {
                  ca->enable_audio = 1; /* audio is started */
                }
#endif
            }
        }
      sdp_message_free (local_sdp);
      sdp_message_free (remote_sdp);
    }

  ca->state = je->type;
  return 0;
}

int
call_redirected (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);

  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_requestfailure (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }

  if ((je->response != NULL && je->response->status_code == 407)
      || (je->response != NULL && je->response->status_code == 401))
    {
      /* try authentication */
      return 0;
    }

  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);

  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_serverfailure (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);


  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_globalfailure (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);

  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_closed (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED && calls[k].cid == je->cid)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);

  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  ca->state = NOT_USED;
  return 0;
}

int
call_modified (eXosip_event_t * je)
{
  call_t *ca;
  int k;

  if (___call_init == 0)
    {
      ___call_init = -1;
      __call_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED
          && calls[k].cid == je->cid && calls[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(calls[k]);
  ca->tid = je->tid;
  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }
  ca->state = je->type;

  eXosip_lock ();
  {
    osip_message_t *answer = NULL;
    int i;

    i = eXosip_call_build_answer (ca->tid, 200, &answer);
    if (i != 0)
      {
        eXosip_call_send_answer (ca->tid, 400, NULL);
        eXosip_unlock ();
        return 0;
    } else
      {
        sdp_message_t *remote_sdp = NULL;

        if (je->request != NULL)
          remote_sdp = eXosip_get_sdp_info (je->request);
        if (remote_sdp == NULL)
          {
            OSIP_TRACE (osip_trace
                        (__FILE__, __LINE__, OSIP_WARNING, NULL,
                         "No remote SDP body found for call\n"));
            /* TODO: sdp in 200 & ACK */
            eXosip_call_send_answer (ca->tid, 200, answer);
        } else
          {
            i = sdp_complete_message (ca->did, remote_sdp, answer);
            if (i != 0)
              {
                sdp_message_free (remote_sdp);
                osip_message_free (answer);
                eXosip_call_send_answer (ca->tid, 415, NULL);
                eXosip_unlock ();
                return 0;
            } else
              eXosip_call_send_answer (ca->tid, 200, answer);
            sdp_message_free (remote_sdp);
          }
      }
  }
  eXosip_unlock ();

  if (ca->enable_audio > 0)
    {
      ca->enable_audio = -1;
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
      os_sound_close (ca);
#endif
    }

  if (ca->enable_audio != 1)    /* audio is started */
    {
      sdp_message_t *remote_sdp;
      sdp_message_t *local_sdp;

      remote_sdp = eXosip_get_remote_sdp (ca->did);
      local_sdp = eXosip_get_local_sdp (ca->did);
      if (remote_sdp == NULL)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_WARNING, NULL,
                       "No remote SDP body found for call\n"));
        }
      if (remote_sdp != NULL && local_sdp != NULL)
        {
          sdp_connection_t *conn;
          sdp_media_t *local_med;
          sdp_media_t *remote_med;
          char *tmp = NULL;
          int audio_port = 0;

          conn = eXosip_get_audio_connection (remote_sdp);
          if (conn != NULL && conn->c_addr != NULL)
            {
              snprintf (ca->remote_sdp_audio_ip, 50, conn->c_addr);
            }
          remote_med = eXosip_get_audio_media (remote_sdp);
          if (remote_med != NULL && remote_med->m_port != NULL)
            {
              ca->remote_sdp_audio_port = atoi (remote_med->m_port);
            }
          local_med = eXosip_get_audio_media (local_sdp);
          if (local_med != NULL && local_med->m_port != NULL)
            {
              audio_port = atoi (local_med->m_port);
            }

          if (ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0' && local_med != NULL)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection found: %s:%i\n",
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              tmp = (char *) osip_list_get (local_med->m_payloads, 0);
            }
          if (tmp != NULL)
            {
              ca->payload = atoi (tmp);
            }
          if (tmp != NULL
              && audio_port > 0
              && ca->remote_sdp_audio_port > 0
              && ca->remote_sdp_audio_ip[0] != '\0')

            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_WARNING, NULL,
                           "audio connection: (payload=%i) %i -> %s:%i\n",
                           ca->payload,
                           audio_port,
                           ca->remote_sdp_audio_ip, ca->remote_sdp_audio_port));

              /* search if stream is sendonly or recvonly */
              ca->remote_sendrecv =
                _sdp_analyse_attribute (remote_sdp, remote_med);
              ca->local_sendrecv = _sdp_analyse_attribute (local_sdp, local_med);
              if (ca->local_sendrecv == _SENDRECV)
                {
                  if (ca->remote_sendrecv == _SENDONLY)
                    ca->local_sendrecv = _RECVONLY;
                  else if (ca->remote_sendrecv == _RECVONLY)
                    ca->local_sendrecv = _SENDONLY;
                }
#if defined(ORTP_SUPPORT) || defined(UCL_SUPPORT)
              if (0 == os_sound_start (ca, audio_port))
                {
                  ca->enable_audio = 1; /* audio is started */
                }
#endif
            }
        }
      sdp_message_free (local_sdp);
      sdp_message_free (remote_sdp);
    }

  return 0;
}
