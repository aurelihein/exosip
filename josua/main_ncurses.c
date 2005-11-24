/*
 * josua - Jack's open sip User Agent
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef NCURSES_SUPPORT

#include "gui.h"
#include "gui_online.h"

#include "jfriends.h"
#include "jidentity.h"

/* extern eXosip_t eXosip; */

main_config_t cfg = {
  "\0", "\0", "\0", "\0", 1, 5060, "\0", "\0", "\0", "\0", 0, "\0"
};

#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif

static void usage (int code);

static void log_event (eXosip_event_t * je);

static void
log_event (eXosip_event_t * je)
{
  char buf[100];

  buf[0] = '\0';
  if (je->type == EXOSIP_CALL_NOANSWER)
    {
      snprintf (buf, 99, "<- (%i %i) No answer", je->cid, je->did);
  } else if (je->type == EXOSIP_CALL_CLOSED)
    {
      snprintf (buf, 99, "<- (%i %i) Call Closed", je->cid, je->did);
  } else if (je->type == EXOSIP_CALL_RELEASED)
    {
      snprintf (buf, 99, "<- (%i %i) Call released", je->cid, je->did);
  } else if (je->type == EXOSIP_MESSAGE_NEW
             && je->request != NULL && MSG_IS_MESSAGE (je->request))
    {
      char *tmp = NULL;

      if (je->request != NULL)
        {
          osip_body_t *body;

          osip_from_to_str (je->request->from, &tmp);

          osip_message_get_body (je->request, 0, &body);
          if (body != NULL && body->body != NULL)
            {
              snprintf (buf, 99, "<- (%i) from: %s TEXT: %s",
                        je->tid, tmp, body->body);
            }
          osip_free (tmp);
      } else
        {
          snprintf (buf, 99, "<- (%i) New event for unknown request?", je->tid);
        }
  } else if (je->type == EXOSIP_MESSAGE_NEW)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (%i) %s from: %s",
                je->tid, je->request->sip_method, tmp);
      osip_free (tmp);
  } else if (je->type == EXOSIP_MESSAGE_PROCEEDING
             || je->type == EXOSIP_MESSAGE_ANSWERED
             || je->type == EXOSIP_MESSAGE_REDIRECTED
             || je->type == EXOSIP_MESSAGE_REQUESTFAILURE
             || je->type == EXOSIP_MESSAGE_SERVERFAILURE
             || je->type == EXOSIP_MESSAGE_GLOBALFAILURE)
    {
      if (je->response != NULL && je->request != NULL)
        {
          char *tmp = NULL;

          osip_to_to_str (je->request->to, &tmp);
          snprintf (buf, 99, "<- (%i) [%i %s for %s] to: %s",
                    je->tid, je->response->status_code,
                    je->response->reason_phrase, je->request->sip_method, tmp);
          osip_free (tmp);
      } else if (je->request != NULL)
        {
          snprintf (buf, 99, "<- (%i) Error for %s request",
                    je->tid, je->request->sip_method);
      } else
        {
          snprintf (buf, 99, "<- (%i) Error for unknown request", je->tid);
        }
  } else if (je->response == NULL && je->request != NULL && je->cid > 0)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (%i %i) %s from: %s",
                je->cid, je->did, je->request->cseq->method, tmp);
      osip_free (tmp);
  } else if (je->response != NULL && je->cid > 0)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      snprintf (buf, 99, "<- (%i %i) [%i %s] for %s to: %s",
                je->cid, je->did, je->response->status_code,
                je->response->reason_phrase, je->request->sip_method, tmp);
      osip_free (tmp);
  } else if (je->response == NULL && je->request != NULL && je->rid > 0)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (%i) %s from: %s",
                je->rid, je->request->cseq->method, tmp);
      osip_free (tmp);
  } else if (je->response != NULL && je->rid > 0)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (%i) [%i %s] from: %s",
                je->rid, je->response->status_code,
                je->response->reason_phrase, tmp);
      osip_free (tmp);
  } else if (je->response == NULL && je->request != NULL && je->sid > 0)
    {
      char *tmp = NULL;
      char *stat = NULL;
      osip_header_t *sub_state;

      osip_message_header_get_byname (je->request, "subscription-state",
                                      0, &sub_state);
      if (sub_state != NULL && sub_state->hvalue != NULL)
        stat = sub_state->hvalue;

      osip_uri_to_str (je->request->from->url, &tmp);
      snprintf (buf, 99, "<- (%i) [%s] %s from: %s",
                je->sid, stat, je->request->cseq->method, tmp);
      osip_free (tmp);
  } else if (je->response != NULL && je->sid > 0)
    {
      char *tmp = NULL;

      osip_uri_to_str (je->request->to->url, &tmp);
      snprintf (buf, 99, "<- (%i) [%i %s] from: %s",
                je->sid, je->response->status_code,
                je->response->reason_phrase, tmp);
      osip_free (tmp);
  } else if (je->response == NULL && je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (c=%i|d=%i|s=%i|n=%i) %s from: %s",
                je->cid, je->did, je->sid, je->nid, je->request->sip_method, tmp);
      osip_free (tmp);
  } else if (je->response != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->from, &tmp);
      snprintf (buf, 99, "<- (c=%i|d=%i|s=%i|n=%i) [%i %s] for %s from: %s",
                je->cid, je->did, je->sid, je->nid,
                je->response->status_code, je->response->reason_phrase,
                je->request->sip_method, tmp);
      osip_free (tmp);
  } else
    {
      snprintf (buf, 99, "<- (c=%i|d=%i|s=%i|n=%i|t=%i) %s",
                je->cid, je->did, je->sid, je->nid, je->tid, je->textinfo);
    }

  josua_printf (buf);
}

int
josua_event_get ()
{
  int counter = 0;

  /* use events to print some info */
  eXosip_event_t *je;

  for (;;)
    {
      je = eXosip_event_wait (0, 50);
      eXosip_lock ();
      /* eXosip_automatic_action (); */

      eXosip_default_action (je);
      eXosip_automatic_refresh ();

      eXosip_unlock ();
      if (je == NULL)
        break;
      counter++;
      log_event (je);
      if (je->type == EXOSIP_CALL_INVITE)
        {
          call_new (je);
      } else if (je->type == EXOSIP_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_REFER (je->request))
        {
          int i;

          if (je->tid > 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_message_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  eXosip_message_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_REFER (je->request))
        {
          int i;

          /* accepte call transfer */
          if (je->cid > 0 && je->did > 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 202, &answer);
              if (i == 0)
                {
                  i = eXosip_call_send_answer (je->tid, 202, answer);
                }
              eXosip_unlock ();

              eXosip_lock ();
              if (i == 0)
                {
                  osip_message_t *notify;

                  i =
                    eXosip_call_build_notify (je->did,
                                              EXOSIP_SUBCRSTATE_ACTIVE, &notify);
                  if (i == 0)
                    {
                      osip_message_set_header (notify, "Event", "refer");
                      osip_message_set_content_type (notify, "message/sipfrag");
                      osip_message_set_body (notify, "SIP/2.0 100 Trying",
                                             strlen ("SIP/2.0 100 Trying"));
                      i = eXosip_call_send_request (je->did, notify);

                    }
                  if (i != 0)
                    beep ();
                }
              eXosip_unlock ();
          } else if (je->tid > 0)       /* bug?? */
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_message_build_answer (je->tid, 400, &answer);
              if (i == 0)
                {
                  eXosip_message_send_answer (je->tid, 400, answer);
                }
              eXosip_unlock ();
            }

      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_UPDATE (je->request))
        {
          int i;

          /* accepte call transfer */
          if (je->cid > 0 && je->did > 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  i = eXosip_call_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();

          } else if (je->tid > 0)       /* bug?? */
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_message_build_answer (je->tid, 400, &answer);
              if (i == 0)
                {
                  eXosip_message_send_answer (je->tid, 400, answer);
                }
              eXosip_unlock ();
            }

      } else if (je->type == EXOSIP_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_NOTIFY (je->request))
        {
          int i;

          if (je->tid > 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_message_build_answer (je->tid, 501, &answer);
              if (i == 0)
                {
                  eXosip_message_send_answer (je->tid, 501, answer);
                }
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_NOTIFY (je->request))
        {
          int i;

          if (je->cid > 0 && je->did > 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  i = eXosip_call_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_CALL_ACK)
        {
          char buf[100];

          snprintf (buf, 99, "<- (%i %i) ACK received", je->cid, je->did);
          josua_printf (buf);
          call_ack (je);
      } else if (je->type == EXOSIP_CALL_ANSWERED)
        {
          call_answered (je);
      } else if (je->type == EXOSIP_CALL_PROCEEDING)
        {
          call_proceeding (je);
      } else if (je->type == EXOSIP_CALL_RINGING)
        {
          call_ringing (je);
      } else if (je->type == EXOSIP_CALL_REDIRECTED)
        {
          call_redirected (je);
      } else if (je->type == EXOSIP_CALL_REQUESTFAILURE)
        {
          call_requestfailure (je);
      } else if (je->type == EXOSIP_CALL_SERVERFAILURE)
        {
          call_serverfailure (je);
      } else if (je->type == EXOSIP_CALL_GLOBALFAILURE)
        {
          call_globalfailure (je);
      } else if (je->type == EXOSIP_CALL_NOANSWER)
        {
      } else if (je->type == EXOSIP_CALL_CLOSED)
        {
          call_closed (je);
      } else if (je->type == EXOSIP_CALL_RELEASED)
        {
          call_closed (je);
      } else if (je->type == EXOSIP_CALL_REINVITE)
        {
          call_modified (je);
      } else if (je->type == EXOSIP_REGISTRATION_SUCCESS)
        {
          if (je->response != NULL)
            josua_registration_status = je->response->status_code;
          else
            josua_registration_status = 0;
          if (je->request != NULL && je->request->req_uri != NULL &&
              je->request->req_uri->host != NULL)
            snprintf (josua_registration_server, 100, "sip:%s",
                      je->request->req_uri->host);
          if (je->response != NULL && je->response->reason_phrase != NULL
              && je->response->reason_phrase != '\0')
            snprintf (josua_registration_reason_phrase, 100, "%s",
                      je->response->reason_phrase);
          else
            josua_registration_reason_phrase[0] = '\0';

          if (je->response != NULL)
            {
              _josua_set_service_route (je->response);
            }

      } else if (je->type == EXOSIP_REGISTRATION_FAILURE)
        {
          if (je->response != NULL)
            josua_registration_status = je->response->status_code;
          else
            josua_registration_status = 0;
          if (je->request != NULL && je->request->req_uri != NULL &&
              je->request->req_uri->host != NULL)
            snprintf (josua_registration_server, 100, "sip:%s",
                      je->request->req_uri->host);
          if (je->response != NULL && je->response->reason_phrase != NULL
              && je->response->reason_phrase != '\0')
            snprintf (josua_registration_reason_phrase, 100, "%s",
                      je->response->reason_phrase);
          else
            josua_registration_reason_phrase[0] = '\0';
      } else if (je->type == EXOSIP_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_OPTIONS (je->request))
        {
          int k;

          /* outside of any call */
          for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
            {
              if (calls[k].state != NOT_USED)
                break;
            }
          eXosip_lock ();
          if (k == MAX_NUMBER_OF_CALLS)
            {
              eXosip_options_send_answer (je->tid, 200, NULL);
          } else
            {
              /* answer 486 ok */
              eXosip_options_send_answer (je->tid, 486, NULL);
            }
          eXosip_unlock ();

      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_OPTIONS (je->request))
        {
          int k;

          /* answer the OPTIONS method */
          /* 1: search for an existing call */
          if (je->cid != 0)
            {
              osip_message_t *answer;
              int i;

              for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
                {
                  if (calls[k].state != NOT_USED && calls[k].did == je->did)
                    break;
                }
              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  eXosip_call_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();
          } else                /* bug? */
            {
              eXosip_lock ();
              eXosip_options_send_answer (je->tid, 400, NULL);
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_INFO (je->request))
        {
          int i;

          /* what's the purpose of sending INFO outside dialog? */
          eXosip_lock ();
          i = eXosip_message_send_answer (je->tid, 501, NULL);
          eXosip_unlock ();
      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_INFO (je->request))
        {
          if (je->cid != 0)
            {
              osip_message_t *answer;
              int i;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  eXosip_call_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();
          } else                /* bug? */
            {
              int i;

              eXosip_lock ();
              i = eXosip_call_send_answer (je->tid, 400, NULL);
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_MESSAGE (je->request))
        {
          /* answer 2xx */
          eXosip_lock ();
          eXosip_message_send_answer (je->tid, 200, NULL);
          eXosip_unlock ();
      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW
                 && je->request != NULL && MSG_IS_MESSAGE (je->request))
        {
          /* answer 2xx */
          if (je->cid != 0)
            {
              osip_message_t *answer;
              int i;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 200, &answer);
              if (i == 0)
                {
                  eXosip_call_send_answer (je->tid, 200, answer);
                }
              eXosip_unlock ();
          } else                /* bug? */
            {
              int i;

              eXosip_lock ();
              i = eXosip_call_send_answer (je->tid, 400, NULL);
              eXosip_unlock ();
            }
      } else if (je->type == EXOSIP_SUBSCRIPTION_ANSWERED)
        {
          jsubscription_answered (je);
      } else if (je->type == EXOSIP_SUBSCRIPTION_PROCEEDING)
        {
          jsubscription_proceeding (je);
      } else if (je->type == EXOSIP_SUBSCRIPTION_REDIRECTED)
        {
          jsubscription_redirected (je);
      } else if (je->type == EXOSIP_SUBSCRIPTION_REQUESTFAILURE)
        {
          jsubscription_requestfailure (je);

          if ((je->response != NULL && je->response->status_code == 407)
              || (je->response != NULL && je->response->status_code == 401))
            {
              static int oddnumber = 0;

              if (oddnumber == 0)
                {
                  /* eXosip_subscribe_refresh(je->sid, "600"); */
                  oddnumber = 1;
              } else
                oddnumber = 0;
            }
      } else if (je->type == EXOSIP_SUBSCRIPTION_SERVERFAILURE)
        {
          jsubscription_serverfailure (je);
      } else if (je->type == EXOSIP_SUBSCRIPTION_GLOBALFAILURE)
        {
          jsubscription_globalfailure (je);
      } else if (je->type == EXOSIP_SUBSCRIPTION_NOTIFY)
        {
          jsubscription_notify (je);
      } else if (je->type == EXOSIP_IN_SUBSCRIPTION_NEW)
        {
          /* search for the user to see if he has been
             previously accepted or not! */

          jinsubscription_new (je);
      } else if (je->type == EXOSIP_MESSAGE_NEW && je->request != NULL)
        {
          osip_message_t *answer;
          int i;

          eXosip_lock ();
          i = eXosip_message_build_answer (je->tid, 405, &answer);
          if (i == 0)
            {
              eXosip_message_send_answer (je->tid, 405, answer);
            }
          eXosip_unlock ();
      } else if (je->type == EXOSIP_CALL_MESSAGE_NEW && je->request != NULL)
        {
          int i;

          /* answer 2xx */
          if (je->cid != 0)
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_call_build_answer (je->tid, 405, &answer);
              if (i == 0)
                {
                  eXosip_call_send_answer (je->tid, 405, answer);
                }
              eXosip_unlock ();
          } else                /* bug? */
            {
              osip_message_t *answer;

              eXosip_lock ();
              i = eXosip_message_build_answer (je->tid, 405, &answer);
              if (i == 0)
                {
                  eXosip_message_send_answer (je->tid, 405, answer);
                }
              eXosip_unlock ();
            }
        }
      eXosip_event_free (je);
    }
  if (counter > 0)
    return 0;
  return -1;
}


static void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   josua args\n\
\n\
\t [-6] Use IPv6\n\
\t [-f <config file>]\n\
\t [-I <identity file>]\n\
\t [-c <nat address>]\n\
\t [-L <log file>]\n\
\t [-f <from url>]\n\
\t [-d <verbose level>]\n\
\t [-p <SIP port>]\n\
\t [-h]\n\
\t [-v]\n\
\n\
  arguments can be used to make a new call or to subscribe.\n\
\n\
\t [-t <sipurl to call>]\n\
\t [-r <sipurl for route>]\n\
\t [-s <subject>]\n\
\t [-S]                       Send a subscription\n\
\t [-T <proto>]               0 (udp) 1 (tcp) 2 (tls).\n");
  exit (code);
}


int
main (int argc, const char *const *argv)
{

  /* deal with options */
  FILE *log_file = NULL;
  char c;
  int i;
  ppl_getopt_t *opt;
  ppl_status_t rv;
  const char *optarg;
  int send_subscription = 0;
  int ipv6only = 0;

  if (argc > 1 && strlen (argv[1]) == 1 && 0 == strncmp (argv[1], "-", 2))
    usage (0);
  if (argc > 1 && strlen (argv[1]) >= 2 && 0 == strncmp (argv[1], "--", 2))
    usage (0);

  ppl_getopt_init (&opt, argc, argv);

#define __APP_BASEARGS "F:I:c:L:f:d:p:t:r:s:T:6vVSh?X"
  while ((rv = ppl_getopt (opt, __APP_BASEARGS, &c, &optarg)) == PPL_SUCCESS)
    {
      switch (c)
        {
          case '6':
            ipv6only = 1;
            eXosip_enable_ipv6 (1);     /* enable IPv6 */
            break;
          case 'F':
            snprintf (cfg.config_file, 255, optarg);
            break;
          case 'I':
            snprintf (cfg.identity_file, 255, optarg);
            break;
          case 'c':
            snprintf (cfg.nat_address, 255, optarg);
            break;
          case 'L':
            snprintf (cfg.log_file, 255, optarg);
            break;
          case 'f':
            snprintf (cfg.identity, 255, optarg);
            break;
          case 'd':
            cfg.debug_level = atoi (optarg);
            break;
          case 'p':
            cfg.port = atoi (optarg);
            break;
          case 't':
            snprintf (cfg.to, 255, optarg);
            break;
          case 'r':
            snprintf (cfg.route, 255, optarg);
            break;
          case 's':
            snprintf (cfg.subject, 255, optarg);
            break;
          case 'S':
            send_subscription = 1;
            break;
          case 'T':
            cfg.proto = atoi (optarg);
            break;
          case 'v':
          case 'V':
            printf ("josua: version:     %s\n", VERSION);
            printf ("build: %s\n", server_built);
            exit (0);
          case 'h':
          case '?':
            printf ("josua: version:     %s\n", VERSION);
            printf ("build: %s\n", server_built);
            usage (0);
            break;
          default:
            /* bad cmdline option?  then we die */
            usage (1);
        }
    }

  if (rv != PPL_EOF)
    {
      usage (1);
    }


  if (cfg.debug_level > 0)
    {
      printf ("josua: %s\n", VERSION);
      printf ("Debug level:        %i\n", cfg.debug_level);
      printf ("Config name:        %s\n", cfg.config_file);
      if (cfg.log_file == NULL)
        printf ("Log name:           Standard output\n");
      else
        printf ("Log name:           %s\n", cfg.log_file);
    }

  /*********************************/
  /* INIT Log File and Log LEVEL   */

  if (cfg.debug_level > 0)
    {
      if (cfg.log_file[0] != '\0')
        {
          log_file = fopen (cfg.log_file, "w+");
      } else
        log_file = stdout;
      if (NULL == log_file)
        {
          perror ("josua: log file");
          exit (1);
        }
      TRACE_INITIALIZE (cfg.debug_level, log_file);
    }

  osip_free ((void *) (opt->argv));
  osip_free ((void *) opt);

  if (cfg.identity[0] == '\0')
    {
      fprintf (stderr, "josua: specify an identity\n");
      fclose (log_file);
      exit (1);
    }

  i = eXosip_init ();
  if (i != 0)
    {
      fprintf (stderr, "josua: could not initialize eXosip\n");
      fclose (log_file);
      exit (0);
    }

  jfriend_load ();
  jidentity_load ();

  if (cfg.proto == 0)
    cfg.proto = IPPROTO_UDP;
  if (cfg.proto == 1)
    cfg.proto = IPPROTO_TCP;
  if (cfg.proto == 2)
    cfg.proto = IPPROTO_TCP;

  if (cfg.proto != 0)
    {
      fprintf (stderr,
               "josua: TCP and TLS support is unfinished: please report bugs or patch\n");
    }

  if (ipv6only == 0)
    i = eXosip_listen_addr (cfg.proto, "0.0.0.0", cfg.port, AF_INET, 0);
  else
    i = eXosip_listen_addr (cfg.proto, "::", cfg.port, AF_INET6, 0);

  if (i != 0)
    {
      eXosip_quit ();
      fprintf (stderr, "josua: could not initialize transport layer\n");
      fclose (log_file);
      exit (0);
    }

  if (cfg.nat_address[0] != '\0')
    {
      eXosip_masquerade_contact (cfg.nat_address, cfg.port);
    }

  if (cfg.to[0] != '\0')
    {                           /* start a command line call, if needed */
      if (send_subscription == 0)
        {
          osip_message_t *invite;

          i = eXosip_call_build_initial_invite (&invite,
                                                cfg.to,
                                                cfg.identity,
                                                cfg.route, cfg.subject);
          if (i != 0)
            {
              fprintf (stderr, "josua: (bad arguments?)\n");
              exit (0);
            }
          eXosip_lock ();
          eXosip_call_send_initial_invite (invite);
          eXosip_unlock ();
      } else
        {
          osip_message_t *sub;

          i = eXosip_subscribe_build_initial_request (&sub, cfg.to,
                                                      cfg.identity,
                                                      cfg.route, "presence", 1800);
          if (i != 0)
            {
              return -1;
            }
#ifdef SUPPORT_MSN
          osip_message_set_accept (sub, "application/xpidf+xml");
#else
          osip_message_set_accept (sub, "application/pidf+xml");
#endif

          eXosip_lock ();
          i = eXosip_subscribe_send_initial_request (sub);
          eXosip_unlock ();
        }
    }

  josua_printf ("Welcome To Josua (%s - port=%i)", cfg.identity, cfg.port);

  gui_start ();


  jfriend_unload ();
  jidentity_unload ();

  fclose (log_file);
  exit (1);
  return (0);
}

#endif
