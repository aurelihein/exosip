/*
 * miniua -  minimalistic sip user agent
 * Copyright (C) 2002,2003,2004,2005   Aymeric Moizard <jack@atosc.org>
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

/*
 * Initial author: Vadim Lebedev <vadim@mbdsys.com>
 */

/**
 * @file miniua.c
 * @brief minimalistic SIP User Agent
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <errno.h>
//#include <unistd.h>
//#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <signal.h>

#ifdef WIN32
#define snprintf _snprintf
#define strncasecmp strnicmp
#endif



#include "phapi.h"
#include "phrpc.h"


/* extern eXosip_t eXosip; */


#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif



/**
 * call progress callback routine
 * @param cid    call id
 * @param info   call state information
 */
static void  callProgress(int cid, const phCallStateInfo_t *info);

static void  transferProgress(int cid, const phTransferStateInfo_t *info);
static void  confProgress(int cfid, const phConfStateInfo_t *info);
static void  regProgress(int regid, int status);

phCallbacks_t mycbk = { callProgress, transferProgress, confProgress, regProgress };

static char default_sip_target[128] = "nowhere";

void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   miniua args\n\
\n\
\t [-c default sip target]\n\
\t [-l <log file>]\n\
\t [-f <from url>]\n\
\t [-d <verbose level>]\n\
\t [-s <phapi server address>[:<port>]]\n\
\t [-cp <phapi callback port>]\n\
\t [-p SIP proxy address]\n\
\t [-fp force proxy\n\
\t [-r userid,pass,realm,server]\n\
\t [-n nattype  (auto, none, fcone, rcone, prcone, sym)]\n\
\t [-codecs codec preference list (PCMU,PCMA,GSM)\
\t [-h]\n\
\t [-v]\n");
    exit (code);
}

static void cmdloop();

static void getreginfo(const char *buf, char *user, char *pass, char *realm, char *server)
{
  const char *tok = strstr(buf, ",");

  while(buf < tok)
    *user++ = *buf++;
  *user = 0;

  tok = strstr(++buf, ",");
  while(buf < tok)
    *pass++ = *buf++;
  *pass = 0;

  tok = strstr(++buf, ",");
  while(buf < tok)
    *realm++ = *buf++;
  *realm = 0;

  strcpy(server, buf+1);
}




  

int main(int argc, const char *const *argv) 
{

  /* deal with options */
  char c;
  int i;
  int send_subscription = 0;
  int needreg=0;
  static char server[32] = "127.0.0.1";
  char userid[256], regserver[256], realm[256], pass[256];



  if (argc <= 1)
    usage(0);

  for( i = 1; i < argc; i++)
    {
      const char *arg = argv[i];

      if (!strcmp(arg, "-c"))
	{
	  strncpy(default_sip_target, argv[++i], sizeof(default_sip_target));
	}
      else if (!strcmp(arg, "-f"))
	{
	  if (i == argc - 1)
	    usage(0);

	  strncpy(phcfg.identity, argv[++i], sizeof(phcfg.identity));
	}
      else if (!strcmp(arg, "-d"))
	{
	  if (i == argc - 1)
	    usage(0);
	     
	  phDebugLevel = atoi(argv[++i]);
	}
      else if (!strcmp(arg, "-s"))
	{
	  char *colon;
	  if (i == argc - 1)
	    usage(0);
	  
	  strncpy(server, argv[++i], sizeof(server));
	  if (colon = strstr(server,":"))
	    {
	      phServerPort = atoi(arg+1);
	      *colon = 0;
	    }
	}
      else if (!strcmp(arg, "-l"))
	{
	  if (i == argc - 1)
	    usage(0);

	  phLogFileName = (char*) argv[++i];
	}
      else if (!strcmp(arg, "-p"))
	{
	  if (i == argc - 1)
	    usage(0);


	  strncpy(phcfg.proxy, argv[++i], sizeof(phcfg.proxy));
	}
      else if (!strcmp(arg, "-fp"))
	phcfg.force_proxy = 1;
      else if (!strcmp(arg, "-r"))
	{
	  getreginfo(argv[++i], userid, pass, realm, regserver);
	  snprintf(phcfg.identity, sizeof(phcfg.identity), "sip:%s@%s", userid,regserver);
	  needreg = 1;
	}

      else if (!strcmp(arg, "-v") || !strcmp(arg, "-V"))
	{
#ifdef VERSION
            printf ("miniua: version:     %s\n", VERSION);
#endif
            printf ("build: %s\n", server_built);
	}
      else if (!strcmp(arg, "-h") || !strcmp(arg, "-?") || !strcmp(arg, "?"))
	{
#ifdef VERSION
            printf ("miniua: version:     %s\n", VERSION);
#endif
            printf ("build: %s\n", server_built);
            usage (0);
	}
      else if (!strcmp(arg, "-n"))
	{
	  if (i == argc - 1)
	    usage(0);

	  strncpy(phcfg.nattype, argv[++i], sizeof(phcfg.nattype));
	}
      else if (!strcmp(arg, "-codecs"))
	{
	  if (i == argc - 1)
	    usage(0);

	  strncpy(phcfg.codecs, argv[++i], sizeof(phcfg.codecs));
	}

    }




  if (phDebugLevel > 0)
    {
#ifdef VERSION
      printf ("miniua: %s\n", VERSION);
#endif
      printf ("Debug level:        %i\n", phDebugLevel);
      if (phLogFileName == NULL)
        printf ("Log name:           Standard output\n");
      else
        printf ("Log name:           %s\n", phLogFileName);
    }


#ifdef SIGTRAP
  signal(SIGTRAP, SIG_IGN);
#endif
  if (phInit(&mycbk, server, 1))
    {
      fprintf (stderr, "miniua: could not initialize phoneapi\n");
      exit(0);
    }


  /*  
   * we've got -r parameters so we need to prepare for autentication and 
   * register ourselves with the registration server
   */
  if (needreg)
    {
      phAddAuthInfo(userid, userid, pass, 0, realm);
      phRegister(userid, regserver);
    }

  printf("Welcome To Miniua\n");
  
  

  cmdloop();
  phTerminate();
  exit(0);
  return(0);
}







static void 
callProgress(int cid, const phCallStateInfo_t *info) 
{
  switch (info->event)
    {
      case phDIALING:
	printf("Dialing cid=%d uri=%s\n", cid, info->u.remoteUri);
	break;

      case phRINGING:
	printf("RINGING cid=%d uri=%s\n", cid, info->u.remoteUri);
	break;

      case phNOANSWER:
	printf("NOANSWER cid=%d uri=%s\n", cid, info->u.remoteUri);
	break;

      case phCALLBUSY:	
	printf("BUSY cid=%d uri=%s\n", cid, info->u.remoteUri);
	break;

      case phCALLREDIRECTED:
	printf("REDIRECTED cid=%d to=%s\n", cid, info->u.remoteUri);
	break;

      case phCALLOK:
	printf("CALLOK cid=%d uri=%s\n", cid, info->u.remoteUri);
	break;

      case phCALLHELD: 
	printf("CALLHELD cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phCALLRESUMED:
	printf("CALLRESUMED cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phHOLDOK:
	printf("HOLDOK cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phRESUMEOK:
	printf("RESUMEOK cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phINCALL:
	printf("INCALL cid=%d to=%s from=%s\n", cid, info->localUri, info->u.remoteUri);
	break;

      case phCALLCLOSED:
	printf("CALLCLOSED cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phCALLERROR:
	printf("CALLERROR cid=%d  status=%d\n", cid, info->u.errorCode);
	break;

      case phDTMF:
	printf("DTMF cid=%d  digit=%c\n", cid, info->u.dtmfDigit);
	break;
    }

}

/**
 * transfer progress callback routine
 * @param cid    transfer id
 * @param info   transfer state information
 */
static void  
transferProgress(int cid, const phTransferStateInfo_t *info) 
{ 
}


/**
 * conference progress callback routine
 * @param cfid   conference id
 * @param info   conference state information
 */
static void  
confProgress(int cfid, const phConfStateInfo_t *info) 
{ 
}



static void  
regProgress(int regid, int status)
{
  printf("REG rid=%d, status=%d\n", regid, status);
}


/**
 * construct a valid uri from given input
 * @param buf   output buffer
 * @param uri   input
 * @param size  size of the output buffer
 */
static int 
geturi(char *buf, char* uri, int size)
{
  while(isspace(*buf)) 
    buf++;

  uri[0] = 0;
  if (strncasecmp(buf, "sip:", 4))
    {
    strncat(uri, "sip:", size);
    uri += 4; size -= 4;
    }

  if (strchr(buf, '@'))
    snprintf(uri, size, buf);
  else
    snprintf(uri, size, "%s@%s", buf, default_sip_target);

  return 0;

}


static void showhelp()
{
  printf(
"Valid commands:\n"
"\tc target          - place Call to target\n"
"\th callid          - Hangup the call\n"
"\ta callid          - Accept incoming call\n"
"\tr callid          - Reject incoming call\n"
"\tn callid          - send riNging event for a call\n"
"\to callid          - hOld the call \n"
"\tu callid          - resUme the call\n"
"\tb 0/1             - set Busy flag (when 1 all incoming calls are automatically rejected)\n"
"\tm callid dtmfchar - send dtMf signal\n"
"\t?         - show this message\n");
}

static void cmdloop()
{
  char buf[256];
  char uri[256];
  int cid;

  while(1)
    {
      char *p;
      char tmp[16];
      int ret = -1;
      int  dtmf, skipresult = 0;
#ifndef WIN32
      fgets(buf, sizeof(buf), stdin);
#else // !WIN32
	  gets(buf);
#endif // !WIN32
      p = strstr(buf, "\n");
      if (p)
	*p = 0;

      switch(buf[0])
	{
	case 'c':
	case 'C':
	  if (!geturi(buf+1, uri, 256))
	    {
	    cid = phPlaceCall(phcfg.identity, uri, 0);
	    printf("Call %d to %s\n", cid, uri);
	    skipresult = 1;
	    } 
	  break;
	case 'h':
	case 'H':
	  cid = atoi(buf+1);
	  ret = phCloseCall(cid);
	  break;
	case 'a':
	case 'A':
	  cid = atoi(buf+1);
	  ret = phAcceptCall(cid);
	  break;
	case 'r':
	case 'R':
	  cid = atoi(buf+1);
	  ret = phRejectCall(cid, 488);
	  break;
	case 'n':
	case 'N':
	  cid = atoi(buf+1);
	  ret = phRingingCall(cid);
	  break;

	case 'o':
	case 'O':
	  cid = atoi(buf+1);
	  ret = phHoldCall(cid);
	  break;

	case 'u':
	case 'U':
	  cid = atoi(buf+1);
	  ret = phResumeCall(cid);
	  break;

        case 'q':
        case 'Q':
	  return;
	case 'b':
	case 'B':
	  ret = phSetBusy(atoi(buf+1));
	  break;

	case 't':
	case 'T':
	  sscanf(buf+1, "%d %s", &cid, tmp);
	  ret = phSendDtmf(cid, tmp[0], 3);
	  break;

        case '?':
	  skipresult = 1;
	  showhelp();
	  break;

	}

      if (!skipresult)
	printf("result = %d\n", ret);
    }
}

	  
  
