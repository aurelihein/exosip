/*
 * ssip - Shell SIP
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

static char rcsid[] = "main_shell:  $Id: main_shell.c,v 1.1.1.1 2003-03-11 21:23:12 aymeric Exp $";

#ifndef NCURSES_SUPPORT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include <term.h>
#include <ncurses.h>

#include "config.h"
#include <jmsip/jmsip.h>
#include <osip2/thread.h>
#include <jmsip/eXosip.h>

#include "ppl_getopt.h"

extern jmsip_t jmsip;



void __jmsip_message(char *command);
void __jmsip_start_call(char *command);
void __jmsip_answer_call(char *command);
void __jmsip_on_hold_call(char *command);
void __jmsip_bye_call(char *command);
void __jmsip_cancel_call(char *command);
void __jmsip_transfert_call(char *command);
void __jmsip_setup(char *comman);
void __jmsip_quit(char *command);
void __jmsip_help(char *command);

void __jmsip_register(char *command);

void show_dialog(int jid);              /* show call [jid]          */
void show_dialog_caller(int jid);       /* show call caller [jid]   */
void show_dialog_callee(int jid);       /* show call callee [jid]   */
void list_dialog_info_header(int jid);  /* show call info   [jid]   */
void list_dialog_contact(int jid);      /* show call contact [jid]  */
void list_dialog_payloads(int jid);     /* show call payloads [jid] */

/* some external methods */
void *smalloc(size_t size);
void  sfree(void *ptr);

#ifndef JD_EMPTY

#define JD_EMPTY          0
#define JD_INITIALIZED    1
#define JD_TRYING         2
#define JD_QUEUED         3
#define JD_RINGING        4
#define JD_ESTABLISHED    5
#define JD_REDIRECTED     6
#define JD_AUTH_REQUIRED  7
#define JD_CLIENTERROR    8
#define JD_SERVERERROR    9
#define JD_GLOBALFAILURE  10
#define JD_TERMINATED     11

#define JD_MAX            11

#endif


#ifdef WIN32
char *
simple_readline (int descr)
{
  int i = 0;
  char *tmp = (char *) smalloc (201);

  fgets (tmp, 200, stdin);
  sclrspace (tmp);
  return tmp;
}
#else
char *
simple_readline (int descr)
{
  int ret;
  fd_set fset;

  FD_ZERO (&fset);
  FD_SET (descr, &fset);
  ret = select (descr + 1, &fset, NULL, NULL, NULL);

  if (FD_ISSET (descr, &fset))
    {
      char *tmp;
      int i;

      tmp = (char *) smalloc (201);
      i = read (descr, tmp, 200);
      if (0==i)
	{
	  fprintf(stdout, "EOF. Thanks for using jmsip.!\n");
	  exit(0);
	}
      if (-1==i)
	{
	  fprintf(stdout, "jmsip could not read descriptor.!\n");
	  exit(0);
	}
      tmp[i] = '\0';
      if (i > 0)
        sclrspace (tmp);
      return tmp;
    }
  return NULL;
}
#endif


void __jmsip_menu() {
  char *tmp;
  while (1)
    {
      printf ("(jmsip) ");
      fflush (stdout);
      tmp = simple_readline (0);
      jmsip_update();
      if (tmp != NULL)
        {
          if (strlen (tmp) >= 5 && 0 == strncmp (tmp, "call ", 5))
            __jmsip_start_call(tmp+5);
          else if (strlen (tmp) >= 4 && 0 == strncmp (tmp, "bye ", 4))
            __jmsip_bye_call(tmp+4);
          else if (strlen (tmp) >= 6 && 0 == strncmp (tmp, "reply ", 6))
            __jmsip_answer_call(tmp+6);
          else if (strlen (tmp) >= 7 && 0 == strncmp (tmp, "cancel ", 7))
            __jmsip_cancel_call(tmp+7);
          else if (strlen (tmp) >= 6 && 0 == strncmp (tmp, "refer ", 6))
            __jmsip_transfert_call(tmp+6);
          else if (strlen (tmp) >= 8 && 0 == strncmp (tmp, "message ", 8))
            __jmsip_message(tmp+8);
          else if (strlen (tmp) >= 9 && 0 == strncmp (tmp, "register ", 9))
            __jmsip_register(tmp+8);
          else if (strlen (tmp) >= 6 && 0 == strncmp (tmp, "setup ", 6))
            __jmsip_setup(tmp+6);
          else if (strlen (tmp) >= 4 && 0 == strncmp (tmp, "help", 4))
            __jmsip_help(tmp+4);
          else if (strlen (tmp) == 1 && 0 == strncmp (tmp, "q", 1))
	    __jmsip_quit(tmp+4);
          else if (strlen (tmp) == 4 && 0 == strncmp (tmp, "quit", 4))
	    __jmsip_quit(tmp+4);
	  else
            {
	      fprintf (stdout, "error: %s: type!\n", tmp);
	      fprintf (stdout, "    type help for commands!\n");
	    }
          sfree (tmp);
      } else
        exit (1);
    }
}

typedef struct _main_config_t {
  char  config_file[256];    /* -F <config file>   */
  char  identity_file[256];  /* -I <identity file> */
  char  contact_file[256];   /* -C <contact file>  */
  char  log_file[256];       /* -L <log file>      */
  int   debug_level;         /* -d <verbose level>   */
  int   port;                /* -p <SIP port>  default is 5060 */
  char  identity[256];       /* -i <from url>  local identity */

  /* the application can command to make a new call */
  char *to;             /* -t <sipurl to call>!  */
  char *route;          /* -r <sipurl for route> */
  char *subject;        /* -s <subject>          */
  int   timeout;        /* -T <delay> connection is closed after 60s */
} main_config_t;

main_config_t cfg = {
  "\0", "\0", "\0", "\0", 1, 5060, "\0", "\0", "\0", "\0", 60
};

#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif


void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   jmsip args\n\
\n\
\t [-F <config file>]\n\
\t [-I <identity file>]\n\
\t [-C <contact file>]\n\
\t [-L <log file>]\n\
\t [-f <from url>]\n\
\t [-d <verbose level>]\n\
\t [-p <SIP port>]\n\
\t [-h]\n\
\t [-i]                       interactive mode\n\
\t [-v]\n\
\n\
  arguments can be used to make a new call.\n\
\n\
\t [-t <sipurl to call>]\n\
\t [-r <sipurl for route>]\n\
\t [-s <subject>]\n\
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
  int interactive_mode = 0;
  const char *optarg;

  if (argc > 1 && strlen (argv[1]) == 1 && 0 == strncmp (argv[1], "-", 2))
    usage (0);
  if (argc > 1 && strlen (argv[1]) >= 2 && 0 == strncmp (argv[1], "--", 2))
    usage (0);

  ppl_getopt_init (&opt, argc, argv);

  /*
  snprintf(cfg.subject, 255, "Let's talk.");
*/
#define __APP_BASEARGS "F:I:C:L:f:d:p:t:r:s:T:vVih?X"
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
          case 'T':
            cfg.timeout = atoi(optarg);
            break;
          case 'v':
          case 'V':
            printf ("jmsip: version:     %s\n", VERSION);
            printf ("build: %s\n", server_built);
            exit (0);
          case 'i':
            interactive_mode = 1;
            break;
          case 'h':
          case '?':
            printf ("jmsip: version:     %s\n", VERSION);
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
      printf ("jmsip: %s\n", VERSION);
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
          perror ("jmsip: log file");
          exit (1);
        }
      TRACE_INITIALIZE (cfg.debug_level, log_file);
    }

  sfree ((void *) (opt->argv));
  sfree ((void *) opt);

  if (cfg.identity[0]=='\0')
    {
      printf ("jmsip: specify an identity\n");
      fclose(log_file);
      exit(1);
    }

  i = jmsip_init(stdin, stdout, cfg.port);
  if (i!=0)
    {
      fclose(log_file);
      exit(0);
    }

  jfreind_load();
  jidentity_load();

  if (interactive_mode != 1)
    {
      /* we've got to look at the timeout value, send a BYE, and quit. */
      select (0, NULL, NULL, NULL, NULL);
    }
  else
    {
      __jmsip_menu();
      printf("please, do not use the interactive mode!\n");
    }

  fclose(log_file);
  exit(1);
  return(0);
}


int
jmsip_get_and_set_next_token (char **dest, char *buf, char **next)
{
  char *end;
  char *start;

  *next = NULL;

  /* find first non space and tab element */
  start = buf;
  while (((*start == ' ') || (*start == '\t')) && (*start != '\0')
         && (*start != '\r') && (*start != '\n'))
    start++;
  end = start;
  while ((*end != '\0') && (*end != '\r') && (*end != '\n') && (*end != ' ')
         && (*end != '\t'))
    end++;
  if ((*end == '\r') || (*end == '\n'))
    /* we should continue normally only if this is the separator asked! */
    return -1;
  if (end == start)
    return -1;                  /* empty value (or several space!) */

  *dest = smalloc (end - (start) + 1);
  sstrncpy (*dest, start, end - start);

  *next = end + 1;              /* return the position right after the separator */
  return 0;
}

void __jmsip_message(char *command) {
  char to[100];
  char route[100];
  char mymessage[100];

  int i;

  char *result;
  char *next;

  sclrspace(command);
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      fprintf(stderr, "jmsip: bad arguments. (1)");
      return;
    }
  sclrspace(result);
  sprintf(to, "%s", result);
  sfree(result);

  command = next;
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      fprintf(stderr, "jmsip: bad arguments. (2)");
      return;
    }
  sclrspace(result);
  sprintf(mymessage, "%s", result);
  sfree(result);

  command = next;
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      jmsip_message(to, cfg.identity, cfg.route, mymessage);
      return;
    }
  sclrspace(result);
  sprintf(route, "%s", result);
  sfree(result);
  
  jmsip_message(to, cfg.identity, route, mymessage);
}

/* call <url> <subject> <route> */
void __jmsip_start_call(char *command) {
  sip_t *invite;
  char to[100];
  char subject[100];
  char route[100];

  int i;

  char *result;
  char *next;

  sclrspace(command);
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      fprintf(stderr, "jmsip: bad arguments. (1)");
      return;
    }
  sclrspace(result);
  sprintf(to, "%s", result);
  sfree(result);

  command = next;
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      i = jmsip_build_initial_invite(&invite,
				     to,
				     cfg.identity,
				     cfg.route,
				     cfg.subject);
      if (i!=0) return;
      jmsip_lock();
      jmsip_start_call(invite);
      jmsip_unlock();
      return;
    }
  sclrspace(result);
  sprintf(route, "%s", result);
  sfree(result);

  command = next;
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      i = jmsip_build_initial_invite(&invite, to, cfg.identity, route, cfg.subject);
      if (i!=0) return;
      jmsip_lock();
      jmsip_start_call(invite);
      jmsip_unlock();
      return;
    }
  sclrspace(result);
  sprintf(subject, "%s", result);
  sfree(result);
  
  i = jmsip_build_initial_invite(&invite, to, cfg.identity, route, subject);
  if (i!=0) return;
  jmsip_lock();
  jmsip_start_call(invite);
  jmsip_unlock();
}

/* reply <jid> <code> */
void __jmsip_answer_call(char *command) {
  int c, i;
  char *tmp;

  sclrspace(command);
  tmp = strchr(command+1, ' ');

  sclrspace(tmp+1);
  c = satoi(tmp+1);
  if (c==-1) return;
  tmp[0]='\0';

  i = satoi(command);
  if (i==-1) return;

  jmsip_lock();
  jmsip_answer_call(i, c);
  jmsip_unlock();
  
}

/* onhold <jid> */
void __jmsip_on_hold_call(char *command) {
  int c;

  sclrspace(command);
  c = satoi(command);
  if (c==-1) return;

  jmsip_lock();
  jmsip_on_hold_call(c);
  jmsip_unlock();
}

/* bye <cid> <jid> */
void __jmsip_bye_call(char *command) {
  int c, i;
  char *tmp;

  sclrspace(command);
  tmp = strchr(command+1, ' ');

  sclrspace(tmp+1);
  i = satoi(tmp+1);
  if (i==-1) return;
  tmp[0]='\0';

  c = satoi(command);
  if (c==-1) return;

  jmsip_lock();
  jmsip_terminate_call(c, i);
  jmsip_unlock();
}

/* cancel <cid> */
void __jmsip_cancel_call(char *command) {
  int c;

  sclrspace(command);
  c = satoi(command);
  if (c==-1) return;

  jmsip_lock();
  jmsip_terminate_call(c, -1);
  jmsip_unlock();
}

/* refer <jid> <sip_url> */
void __jmsip_transfert_call(char *command) {
  int c;
  char *tmp;
  char refer_to[121];

  sclrspace(command);
  tmp = strchr(command+1, ' ');

  sclrspace(tmp+1);
  if (tmp[1]=='\0')
    return ;
  snprintf(refer_to, 120, "%s", tmp+1);
  tmp[0]='\0';

  c = satoi(command);
  if (c==-1) return;  

  jmsip_lock();
  jmsip_transfert_call(c, refer_to);
  jmsip_unlock();
  
}

void __jmsip_register(char *command) {
  char *tmp;
  char *tmp2;
  int jid;
  static int previous_jid = -2;

  sclrspace(command);
  jid = satoi(command);

  if (jid==-1) return;

  jmsip_lock();
  tmp = jidentity_get_identity(jid);
  tmp2 = jidentity_get_registrar(jid);
  jmsip_unlock();
  if (tmp==NULL||tmp2==NULL) return;

  if (jid!=previous_jid)
    {
      jmsip_register_init(tmp,
			  tmp2, NULL);
      previous_jid = jid;
    }

  jmsip_lock();
  jmsip_register(-1);
  jmsip_unlock();
}

void __jmsip_setup(char *command) {

  char identity[100];
  char registrar[100];
  char realm[100];
  char userid[100];
  char password[100];
  int i;

  char *result;
  char *next;

  sclrspace(command);
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      fprintf(stderr, "jmsip: bad arguments. (1)");
      return;
    }
  sclrspace(result);
  sprintf(identity, "%s", result);
  sfree(result);

  command = next;
  i = jmsip_get_and_set_next_token(&result, command, &next);
  if (i != 0)
    {
      fprintf(stderr, "jmsip: bad arguments. (2)");
      return;
    }
  sclrspace(result);
  sprintf(registrar, "%s", result);
  sfree(result);

  sprintf(realm, "%s", "\"\"");
  sprintf(userid, "%s", "\"\"");
  sprintf(password, "%s", "\"\"");

  identitys_add(identity, registrar, realm, userid, password);
}

void __jmsip_help(char *command) {
  fprintf(stdout, "jmsip help\n\n");
  fprintf(stdout, "  jmsip commands!\n\n");
  fprintf(stdout, " call      start a call.\n");
  fprintf(stdout, "                   call <sip:jack@192.168.1.1>.\n");
  fprintf(stdout, " reply     answer a call.\n");
  fprintf(stdout, "                   reply 1 200   or   reply 1 486.\n");
  fprintf(stdout, " refer     transfert call.\n");
  fprintf(stdout, "                   refer 1 <sip:joe@192.168.1.2>.\n");
  fprintf(stdout, " cancel    cancel a pending call.\n");
  fprintf(stdout, "                   cancel 1.\n");
  fprintf(stdout, " bye       terminate a pending call.\n");
  fprintf(stdout, "                   bye 1 1.\n");
  fprintf(stdout, " message   send a message to a freind.\n");
  fprintf(stdout, "                   message <sip:joe@192.168.1.2> my_message_sample_one_word_only.\n");
  fprintf(stdout, " register  register identities.\n");
  fprintf(stdout, "                   register 1   (from the list of identity)\n");
  fprintf(stdout, " quit      quit application.\n");
  fprintf(stdout, " help      print this help!\n\n");
}


void __jmsip_quit(char *command) {

  jmsip_quit();

  exit(1);
}


#define JD_EMPTY          0
#define JD_INITIALIZED    1
#define JD_TRYING         2
#define JD_QUEUED         3
#define JD_RINGING        4
#define JD_ESTABLISHED    5
#define JD_REDIRECTED     6
#define JD_AUTH_REQUIRED  7
#define JD_CLIENTERROR    8
#define JD_SERVERERROR    9
#define JD_GLOBALFAILURE  10
#define JD_TERMINATED     11
#define JD_MAX            11

static char *state_tab[] = {
  "Empty",
  "Initialized",
  "Trying",
  "Queued",
  "Ringing",
  "Established",
  "Redirected",
  "Authentication Required",
  "ClientError",
  "ServerError",
  "GlobalFailure",

};

/*
  j->d_id must be set
  j->state must be set
  j->d_dialog must exist
  j->d_dialog->remote_uri must exist
  j->d_dialog->remote_contact_uri must exist
  j->d_200Ok must exist (or ack if SDP wasn't in INVITE))
  j->ack must exist
  j->audio_lines
  j->video_lines

*/

void _show_dialog_state(jmsip_dialog_t *jd)       /* show call callee [jid] */
{
  if (0<=jd->d_STATE && jd->d_STATE<=JD_MAX)
    fprintf(jmsip.j_output, "\tState  :  %s\n", state_tab[jd->d_STATE]);
  else
    fprintf(jmsip.j_output, "\tState  :  **UNDEFINED**\n");
}

void _show_dialog_with(jmsip_dialog_t *jd)       /* show call callee [jid] */
{
  char *tmp;
  int i;
  if (jd->d_dialog==NULL)
    return ;
  i = to_2char(jd->d_dialog->remote_uri, &tmp);
  if (i!=0)
    fprintf(jmsip.j_output, "\tWith   :  %s\n", tmp);
  else
    fprintf(jmsip.j_output, "\tWith   :  **error in remote_uri**\n");
}

void _show_dialog_contact(jmsip_dialog_t *jd)       /* show call callee [jid] */
{
  char *tmp;
  int i;
  if (jd->d_dialog==NULL)
    return ;
  if (jd->d_dialog->remote_contact_uri==NULL)
    {
      fprintf(jmsip.j_output, "\tContact:  **unknown**\n");
      return ;
    }

  i = contact_2char(jd->d_dialog->remote_contact_uri, &tmp);

  if (i==0)
    fprintf(jmsip.j_output, "\tContact:  %s\n", tmp);
  else
    fprintf(jmsip.j_output, "\tContact:  **error in contact**\n");
  return ;
}

void _show_dialog_media(jmsip_dialog_t *jd, int ml)  /* show call callee [jid] */
{
  char *tmp;
  tmp = list_get(jd->media_lines, ml);
  fprintf(jmsip.j_output, "%s\n", tmp);
}


void _show_dialog(jmsip_dialog_t *jd, char *subject)       /* show call callee [jid] */
{
  char *tmp;
  int pos;

  fprintf(jmsip.j_output, "\tCall   :  %i\n", jd->d_id);
  _show_dialog_state(jd);
  if (subject!=NULL)
    fprintf(jmsip.j_output, "\tSubject:  %s\n", subject);
  else
    fprintf(jmsip.j_output, "\tSubject:  (no subject)\n");
  if (jd->d_dialog==NULL)
    return ;
  _show_dialog_with(jd);
  _show_dialog_contact(jd);
  
  pos=0;
  while (!list_eol(jd->media_lines, pos))
    {
      tmp = list_get(jd->media_lines, pos);
      pos++;
      fprintf(jmsip.j_output, "%s\n", tmp);
    }
}


void show()
{
  jmsip_call_t *jc;
  jmsip_dialog_t *jd;
  jmsip_update();
  for (jc=jmsip.j_calls; jc!=NULL; jc=jc->next)
    {
      for (jd=jc->c_dialogs; jd!=NULL; jd=jd->next)
	{
	  _show_dialog(jd, jc->c_subject);
	}
    }
}

void show_dialog(int jid)              /* show call [jid] */
{
  jmsip_call_t *jc;
  jmsip_dialog_t *jd;
  for (jc=jmsip.j_calls; jc!=NULL; jc=jc->next)
    {
      for (jd=jc->c_dialogs; jd!=NULL; jd=jd->next)
	{
	  if (jd->d_id==jid)
	    {
	      _show_dialog(jd, jc->c_subject);
	      return;
	    }
	}
    }
}

void show_dialog_state(int jid)       /* show call caller [jid] */
{
  jmsip_dialog_t *jd;
  jmsip_call_t *jc;
  jmsip_dialog_find(jid, &jc, &jd);
  if (jd==NULL) return;
  _show_dialog_state(jd);
}

void show_dialog_with(int jid)       /* show call caller [jid] */
{
  jmsip_dialog_t *jd;
  jmsip_call_t *jc;
  jmsip_dialog_find(jid, &jc, &jd);
  if (jd==NULL) return;
  _show_dialog_with(jd);
}

void show_dialog_contact(int jid)       /* show call caller [jid] */
{
  jmsip_dialog_t *jd;
  jmsip_call_t *jc;
  jmsip_dialog_find(jid, &jc, &jd);
  if (jd==NULL) return;
  _show_dialog_contact(jd);
}

void show_dialog_media(int jid, int ml)       /* show call caller [jid] */
{
  jmsip_dialog_t *jd;
  jmsip_call_t *jc;
  jmsip_dialog_find(jid, &jc, &jd);
  if (jd==NULL) return;
  _show_dialog_media(jd, ml);
}


#endif
