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

static char rcsid[] = "main_ncurses:  $Id: main_ncurses.c,v 1.49 2003-07-01 17:07:11 aymeric Exp $";

#ifdef NCURSES_SUPPORT

#include "gui.h"
#include "gui_online.h"

extern eXosip_t eXosip;

main_config_t cfg = {
  "\0", "\0", "\0", "\0", 1, 5060, "\0", "\0", "\0", "\0", 60
};

#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif

int
josua_event_get()
{
  int counter =0;
  /* use events to print some info */
  eXosip_event_t *je;
  for (;;)
    {
      char buf[100];
      je = eXosip_event_wait(0,0);
      if (je==NULL)
	break;
      counter++;
      if (je->type==EXOSIP_CALL_NEW)
	{
	  snprintf(buf, 99, "<- (%i %i) INVITE from: %s",
		   je->cid, je->did,
		   je->remote_uri);
	  josua_printf(buf);

#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	  jcall_new(je);
	}
      else if (je->type==EXOSIP_CALL_ANSWERED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	  jcall_answered(je);
	}
      else if (je->type==EXOSIP_CALL_PROCEEDING)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);

#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	  jcall_proceeding(je);
	}
      else if (je->type==EXOSIP_CALL_RINGING)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	  jcall_ringing(je);
	}
      else if (je->type==EXOSIP_CALL_REDIRECTED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jcall_redirected(je);
	}
      else if (je->type==EXOSIP_CALL_REQUESTFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jcall_requestfailure(je);
	}
      else if (je->type==EXOSIP_CALL_SERVERFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jcall_serverfailure(je);
	}
      else if (je->type==EXOSIP_CALL_GLOBALFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jcall_globalfailure(je);
	}
      else if (je->type==EXOSIP_CALL_CLOSED)
	{
	  snprintf(buf, 99, "<- (%i %i) BYE from: %s",
		   je->cid, je->did, je->remote_uri);
	  josua_printf(buf);
	  jcall_closed(je);
	}
      else if (je->type==EXOSIP_CALL_HOLD)
	{
	  snprintf(buf, 99, "<- (%i %i) INVITE (On Hold) from: %s",
		   je->cid, je->did, je->remote_uri);
	  josua_printf(buf);
	  jcall_onhold(je);
	}
      else if (je->type==EXOSIP_CALL_OFFHOLD)
	{
	  snprintf(buf, 99, "<- (%i %i) INVITE (Off Hold) from: %s",
		   je->cid, je->did, je->remote_uri);
	  josua_printf(buf);
	  jcall_offhold(je);
	}
      else if (je->type==EXOSIP_REGISTRATION_SUCCESS)
	{
	  snprintf(buf, 99, "<- (%i) [%i %s] %s for REGISTER %s",
		   je->rid,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri,
		   je->req_uri);
	  josua_printf(buf);

	  josua_registration_status = je->status_code;
	  snprintf(josua_registration_server, 100, "%s", je->req_uri);
	  if (je->reason_phrase!='\0')
	    snprintf(josua_registration_reason_phrase, 100, "%s", je->reason_phrase);
	  else josua_registration_reason_phrase[0] = '\0';
	}
      else if (je->type==EXOSIP_REGISTRATION_FAILURE)
	{
	  snprintf(buf, 99, "<- (%i) [%i %s] %s for REGISTER %s",
		   je->rid,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri,
		   je->req_uri);
	  josua_printf(buf);
	  josua_registration_status = je->status_code;
	  snprintf(josua_registration_server, 100, "%s", je->req_uri);
	  if (je->reason_phrase!='\0')
	    snprintf(josua_registration_reason_phrase, 100, "%s", je->reason_phrase);
	  else josua_registration_reason_phrase[0] = '\0';
	  
	}
      else if (je->type==EXOSIP_OPTIONS_NEW)
	{
	  int k;
	  snprintf(buf, 99, "<- (%i %i) OPTIONS from: %s",
		   je->cid, je->did,
		   je->remote_uri);
	  josua_printf(buf);

	  /* answer the OPTIONS method */
	  /* 1: search for an existing call */
	  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	    {
	      if (jcalls[k].state != NOT_USED
		  || jcalls[k].cid==je->cid)
		break;
	    }
	  eXosip_lock();
	  if (jcalls[k].cid==je->cid)
	    {
	      /* already answered! */
	    }
	  else if (k==MAX_NUMBER_OF_CALLS)
	    {
	      /* answer 200 ok */
	      eXosip_answer_options(je->cid, je->did, 200);
	    }
	  else
	    {
	      /* answer 486 ok */
	      eXosip_answer_options(je->cid, je->did, 486);
	    }
	  eXosip_unlock();

#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	}
      else if (je->type==EXOSIP_OPTIONS_ANSWERED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	}
      else if (je->type==EXOSIP_OPTIONS_PROCEEDING)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);

#if 0
	  if (je->remote_sdp_audio_ip[0]!='\0')
	    {
	      snprintf(buf, 99, "<- Remote sdp info: %s:%i",
		       je->remote_sdp_audio_ip,
		       je->remote_sdp_audio_port);
	      josua_printf(buf);
	    }
#endif
	}
      else if (je->type==EXOSIP_OPTIONS_REDIRECTED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	}
      else if (je->type==EXOSIP_OPTIONS_REQUESTFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	}
      else if (je->type==EXOSIP_OPTIONS_SERVERFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	}
      else if (je->type==EXOSIP_OPTIONS_GLOBALFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s",
		   je->cid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_ANSWERED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);

	  snprintf(buf, 99, "<- (%i %i) online=%i [status: %i reason:%i]",
		   je->sid, je->did, 
		   je->online_status,
		   je->ss_status,
		   je->ss_reason);
	  josua_printf(buf);

	  jsubscription_answered(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_PROCEEDING)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);

	  jsubscription_proceeding(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_REDIRECTED)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jsubscription_redirected(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_REQUESTFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jsubscription_requestfailure(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_SERVERFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jsubscription_serverfailure(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_GLOBALFAILURE)
	{
	  snprintf(buf, 99, "<- (%i %i) [%i %s] %s for SUBSCRIBE",
		   je->sid, je->did,
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri);
	  josua_printf(buf);
	  jsubscription_globalfailure(je);
	}
      else if (je->type==EXOSIP_SUBSCRIPTION_NOTIFY)
	{
	  snprintf(buf, 99, "<- (%i %i) NOTIFY from: %s",
		   je->sid, je->did,
		   je->remote_uri);
	  josua_printf(buf);

	  snprintf(buf, 99, "<- (%i %i) online=%i [status: %i reason:%i]",
		   je->sid, je->did, 
		   je->online_status,
		   je->ss_status,
		   je->ss_reason);
	  josua_printf(buf);

	  jsubscription_notify(je);
	}
      else if (je->type==EXOSIP_IN_SUBSCRIPTION_NEW)
	{
	  snprintf(buf, 99, "<- (%i %i) SUBSCRIBE from: %s",
		   je->nid, je->did,
		   je->remote_uri);
	  josua_printf(buf);

	  /* search for the user to see if he has been
	     previously accepted or not! */

	  eXosip_notify(je->did, EXOSIP_SUBCRSTATE_PENDING, EXOSIP_NOTIFY_AWAY);
	  jinsubscription_new(je);
	}
      else if (je->textinfo[0]!='\0')
	{
	  snprintf(buf, 99, "(%i %i %i %i) %s", je->cid, je->sid, je->nid, je->did, je->textinfo);
	  josua_printf(buf);
	}

	
      eXosip_event_free(je);
    }
  if (counter>0)
    return 0;
  return -1;
}


void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   josua args\n\
\n\
\t [-f <config file>]\n\
\t [-I <identity file>]\n\
\t [-C <contact file>]\n\
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
\t [-T <delay>]               close calls after 60s as a default timeout.\n");
    exit (code);
}


int main(int argc, const char *const *argv) {

  /* deal with options */
  FILE *log_file = NULL;
  char c;
  int i;
  ppl_getopt_t *opt;
  ppl_status_t rv;
  const char *optarg;
  int send_subscription = 0;

  if (argc > 1 && strlen (argv[1]) == 1 && 0 == strncmp (argv[1], "-", 2))
    usage (0);
  if (argc > 1 && strlen (argv[1]) >= 2 && 0 == strncmp (argv[1], "--", 2))
    usage (0);

  ppl_getopt_init (&opt, argc, argv);

#define __APP_BASEARGS "F:I:C:L:f:d:p:t:r:s:T:vVSh?X"
  while ((rv = ppl_getopt (opt, __APP_BASEARGS, &c, &optarg)) == PPL_SUCCESS)
    {
      switch (c)
        {
          case 'F':
            snprintf(cfg.config_file, 255, optarg);
            break;
          case 'I':
            snprintf(cfg.identity_file, 255, optarg);
            break;
          case 'C':
            snprintf(cfg.contact_file, 255, optarg);
            break;
          case 'L':
            snprintf(cfg.log_file, 255, optarg);
            break;
          case 'f':
            snprintf(cfg.identity, 255, optarg);
            break;
          case 'd':
            cfg.debug_level = atoi (optarg);
            break;
          case 'p':
            cfg.port = atoi (optarg);
            break;
          case 't':
            snprintf(cfg.to, 255, optarg);
            break;
          case 'r':
            snprintf(cfg.route, 255, optarg);
            break;
          case 's':
            snprintf(cfg.subject, 255, optarg);
            break;
          case 'S':
	    send_subscription = 1;
            break;
          case 'T':
            cfg.timeout = atoi(optarg);
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

  if (cfg.identity[0]=='\0')
    {
      fprintf (stderr, "josua: specify an identity\n");
      fclose(log_file);
      exit(1);
    }

  i = eXosip_init(stdin, stdout, cfg.port);
  if (i!=0)
    {
      fprintf (stderr, "josua: could not initialize eXosip\n");
      fclose(log_file);
      exit(0);
    }

  /* register callbacks? */
  eXosip_set_mode(EVENT_MODE);

  if (cfg.to[0]!='\0')
    { /* start a command line call, if needed */
      if (send_subscription==0)
	{
	  osip_message_t *invite;
	  i = eXosip_build_initial_invite(&invite,
					  cfg.to,
					  cfg.identity,
					  cfg.route,
					  cfg.subject);
	  if (i!=0)
	    {
	      fprintf (stderr, "josua: (bad arguments?)\n");
	      exit(0);
	    }
	  eXosip_lock();
	  eXosip_start_call(invite, NULL, NULL, "10500");
	  eXosip_unlock();
	}
      else
	{
	  eXosip_lock();
	  eXosip_subscribe(cfg.to,
			   cfg.identity,
			   cfg.route);
	  eXosip_unlock();
	}
    }

  josua_printf("Welcome To Josua");

  gui_start();


  fclose(log_file);
  exit(1);
  return(0);
}

#endif

