/*
 * phapi_server -  RPC server for minimalistic sip user agent
 *
 * Copyright (C) 2004        Vadim Lebedev <vadim@mbdsys.com>
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

/**
 * @file phapi_server.c
 * @brief minimalistic SIP User Agent server
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#else /* !WIN32 */
#include <windows.h>
#include <process.h>
#define snprintf _snprintf
#endif /* !WIN32 */




#include "phapi.h"
#include "phrpc.h"


#define josua_printf printf
#define FILE "phapi_server.c"

struct sockaddr  client_addr;
struct sockaddr  cbk_addr;
int    reqsock;
int    cbksock;


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
static void  regProgress(int rid, int regStatus);

phCallbacks_t mycbk = { callProgress, transferProgress, confProgress, regProgress };


void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   phapi_server args\n\
\n\
\t [-l <log file>]\n\
\t [-d <verbose level>]\n\
\t [-s <server port>]\n\
\t [-c <callback port>]\n\
\t [-k]\t keep running after phTerminate\n\
\t [-q]\t be quiet (don't print Welcome message)\n\
\t [-h]\n");
    exit (code);
}

static void cmdloop();
static void ph_server_init();


int main(int argc, const char *const *argv) {

  char c;
  int i;
  int keeprunning = 0;
  int quiet = 0;

  for( i = 1; i < argc; i++)
    {
      const char *arg = argv[i];

      if (!strcmp(arg, "-c"))
	{
	  if (i == argc - 1)
	    usage(0);

	  phCallBackPort = atoi(argv[++i]);
	}
      else if (!strcmp(arg, "-k"))
	keeprunning = 1;
      else if (!strcmp(arg, "-q"))
	quiet = 1;
      else if (!strcmp(arg, "-d"))
	{
	  if (i == argc - 1)
	    usage(0);
	     
	  phDebugLevel = atoi(argv[++i]);
	}
      else if (!strcmp(arg, "-s"))
	{
	  if (i == argc - 1)
	    usage(0);
	  
	  phServerPort = atoi(argv[++i]);
	}
      else if (!strcmp(arg, "-l"))
	{
	  if (i == argc - 1)
	    usage(0);

	  phLogFileName = (char*) argv[++i];
	}
      else if (!strcmp(arg, "-v") || !strcmp(arg, "-V"))
	{
#ifdef VERSION
            printf ("phapi_server: version:     %s\n", VERSION);
#endif
            printf ("build: %s\n", server_built);
	}
      else if (!strcmp(arg, "-h") || !strcmp(arg, "-?") || !strcmp(arg, "?"))
	{
#ifdef VERSION
            printf ("phapi_server: version:     %s\n", VERSION);
#endif
            printf ("build: %s\n", server_built);
            usage (0);
	}

    }

  if (keeprunning)
    {
#ifndef WIN32
      int pid;
      while (pid = fork())
	{
	  int status;
	  wait(&status);
	}
#else // !WIN32
 int ret = 1;
      while (!(ret = _spawnl(_P_WAIT, argv[0], "phapiserver.exe", NULL)))
	if (ret == -1)
	  perror(FILE);
#endif /* !WIN32 */
    }

  if (phDebugLevel > 0)
    {
#ifdef VERSION
      printf ("phapi_server: %s\n", VERSION);
#endif
      printf ("Debug level:        %i\n", phDebugLevel);
      if (phLogFileName == NULL)
        printf ("Log name:           Standard output\n");
      else
        printf ("Log name:           %s\n", phLogFileName);
    }






  printf("Welcome To phapi_server\n");

  ph_server_init();
  cmdloop();
  //  phTerminate();

  exit(0);
  return(0);
}

static void
ph_server_init()
{
  struct sockaddr_in serveraddr;
  struct
  {
    char msgcode[8];
    phConfig_t cfg;
  }  cfgmsg;
  int i;
#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(1,1);
  if(i = WSAStartup(wVersionRequested,  &wsaData))
       {
		   perror(FILE);
		   exit(1);
       }
#endif /* !WIN32 */

  reqsock = socket(AF_INET, SOCK_DGRAM, 0);
  cbksock = socket(AF_INET, SOCK_DGRAM, 0);


  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(phServerPort);
  serveraddr.sin_addr.s_addr = 0;

  if (bind(reqsock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
    perror("server init error:");
    exit(1);
    }


  /* wait for protocol initialization message */
  while(1)
    {
      unsigned int len = sizeof(client_addr);
      i = recvfrom(reqsock, &cfgmsg, sizeof(cfgmsg), 0, &client_addr, &len);
      if (i <= 0)
	{
	  perror("error reading init message");
	  exit(1);
	}

      if (!strncmp(cfgmsg.msgcode,"PHINIT0", 7))
	{
	  char ans[32];

	  phcfg = cfgmsg.cfg;

	  i = phInit(&mycbk, NULL, 1);

	  sprintf(ans, "RET %d", i);

	  connect(reqsock, &client_addr, sizeof(client_addr));
	  sendto(reqsock, ans, strlen(ans)+1, 0, &client_addr, sizeof(client_addr));

	  /* 
	     we've got our client address, so we can fix the address
	     for callback messages
	  */
	  cbk_addr = client_addr;
	  ((struct sockaddr_in *)&cbk_addr)->sin_port = htons(phCallBackPort); 
	  connect(cbksock, &cbk_addr, sizeof(cbk_addr));


	  if (!i)
	    return;
	}

    }
}  












#define xx(n) #n
static const char *ph_call_event_names[] =
{
  xx(DIALING), xx(RINGING), xx(NOANSWER), xx(CALLBUSY), xx(CALLREDIRECTED), xx(CALLOK), 
  xx(CALLHELD), xx(CALLRESUMED), xx(HOLDOK), xx(RESUMEOK), 
  xx(INCALL), xx(CALLCLOSED), xx(CALLERROR), xx(DTMF),

};
#undef xx

static void 
callProgress(int cid, const phCallStateInfo_t *info) 
{
  char  msg[1024];
  unsigned long  udata = (unsigned long) info->userData;
  int i;


  switch (info->event)
    {
      case phDIALING:
      case phRINGING:
      case phNOANSWER:
      case phCALLBUSY:	
      case phCALLREDIRECTED:
      case phCALLOK:
	snprintf(msg, sizeof(msg), "CL EVT=%s CID=%d UDATA=%08lx URI=%s", ph_call_event_names[info->event], cid, udata, info->u.remoteUri);
	break;

      case phINCALL:
	snprintf(msg, sizeof(msg), "CL EVT=%s CID=%d UDATA=%08lx URI2=%s URI=%s", ph_call_event_names[info->event], cid, udata, info->localUri, info->u.remoteUri);
	break;



      case phCALLHELD: 
      case phCALLRESUMED:
      case phHOLDOK:
      case phRESUMEOK:
      case phCALLCLOSED:
      case phCALLERROR:
      case phDTMF:
	snprintf(msg, sizeof(msg), "CL EVT=%s CID=%d ERR=%d", ph_call_event_names[info->event], cid, info->u.errorCode);
	break;


    }


  i = sendto(cbksock, msg, strlen(msg)+1, 0, &cbk_addr, sizeof(cbk_addr));
  if (i == -1)
    {
      phTerminate();
      exit(2);
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

/**
 * registration progress callback routine
 * @param regid   conference id
 * @param status  status code
 */
static void  
regProgress(int rid, int regStatus) 
{ 
  char  msg[256];

  snprintf(msg, sizeof(msg), "RG RID=%d ERR=%d", rid, regStatus);
  
  sendto(cbksock, msg, strlen(msg)+1, 0, &cbk_addr, sizeof(cbk_addr));

}


static int 
keywdcmp(const char *buf, const char *key)
{
  return strncmp(buf, key, strlen(key));
}



static char *
tokenize(char *buf)
{
  char *cp = buf;

  while(*buf && *buf != ' ')
    buf++;

  *buf = 0;
  return cp;
}


static void cmdloop()
{
  char buf[2048];
  char ans[32];
  struct sockaddr from;

  int i;
  int cid;
  int reason;
  char *uri;
  char *token;
  int termflag = 0;
  int ret = -1;
  int cidt;
  int gotfrom = 0;
  fd_set  infds;
  fd_set  errfds;
  int maxsock ;


#ifndef WIN32
  maxsock = reqsock;
  if (cbksock > maxsock)
    maxsock = cbksock;

  maxsock++;
#else
  maxsock = 2;
#endif
  while(1)
    {
      unsigned int len = sizeof(from);
      struct timeval tv;

      FD_ZERO(&infds);
      FD_SET(reqsock, &infds);
      FD_SET(cbksock, &infds);
      FD_ZERO(&errfds);
      FD_SET(reqsock, &errfds);
      FD_SET(cbksock, &errfds);
      tv.tv_sec = 0; tv.tv_usec = 500000;
      i = select(maxsock, &infds, NULL, &errfds, &tv);
      if (i == 0)
	{
	  i = send(cbksock, "PING", 5, 0);
	  if (i == -1)
	    {
	      phTerminate();
	      exit(2);
	    }
	  continue;
	}
      else if (FD_ISSET(reqsock, &errfds) || FD_ISSET(cbksock, &errfds))
	{
	      phTerminate();
	      exit(2);
	}

       
      if (FD_ISSET(cbksock, &infds))
	{
	  i = recv(cbksock, buf, sizeof(buf), 0);
	  if (i < 0)
	    {
	      phTerminate();
	      exit(2);
	    }
	}

      if (!FD_ISSET(reqsock, &infds))
	continue;

      i = recv(reqsock, buf, sizeof(buf), 0);
      if (i < 0)
	{
	  phTerminate();
	  exit(2);
	}

      gotfrom = 1;

      cid = -1;
      reason = -1;
      uri = NULL;

      token = strstr(buf, "CID=");
      if (token)
	cid = atoi(token+4);

      token = strstr(buf, "CIDT=");
      if (token)
	cidt = atoi(token+5);
      
      token = strstr(buf, "REASON=");
      if (token)
	reason = atoi(token+7);

      token = strstr(buf, "TO=");
      if (token)
	uri = token+3;

      if (!keywdcmp(buf, "CALL"))
	{
	  unsigned long udata;
	  char *from;
	  char *token;

	  token = strstr(buf, "UDATA=");
	  sscanf(token+7, "%08lx", &udata);
	  token = strstr(buf, "FROM=");
	  from = token+5;
	  for (token = from; *token != ' '; token++)
	    ;
	  *token = 0;

	  ret = phPlaceCall(from, uri, (void *) udata);
	}
      else if (!keywdcmp(buf, "CLOSE"))
	ret = phCloseCall(cid);
      else if (!keywdcmp(buf, "ACCEPT"))
	{
	  unsigned long udata;
	  char *token;

	  token = strstr(buf, "UDATA=");
	  sscanf(token+7, "%08lx", &udata);

	  ret = phAcceptCall2(cid, (void *) udata);
	}
      else if (!keywdcmp(buf, "REJECT"))
	ret = phRejectCall(cid, reason);
      else if (!keywdcmp(buf, "RINGING"))
	ret = phRingingCall(cid);
      else if (!keywdcmp(buf, "BTXCALL"))
	ret = phBlindTransferCall(cid, uri);
      else if (!keywdcmp(buf, "TXCALL"))
	ret = phTransferCall(cid, cidt);
      else if (!keywdcmp(buf, "RESUME"))
	ret = phResumeCall(cid);
      else if (!keywdcmp(buf, "HOLD"))
	ret = phHoldCall(cid);
      else if (!keywdcmp(buf, "FOLLOWME"))
	ret = phSetFollowMe(uri);
      else if (!keywdcmp(buf, "SETBUSY"))
	ret = phSetBusy(atoi(buf + 8));
      else if (!keywdcmp(buf, "AUTH"))
	{
	  char *u = strstr(buf, "U=");
	  char *id = strstr(buf, "ID=");
	  char *pass = strstr(buf, "P=");
	  char *ha1 = strstr(buf, "HA1=");
	  char *r = strstr(buf, "R=");
	  
	  u = tokenize(u+2);
	  id = tokenize(id+3);
	  pass = tokenize(pass+2);
	  r = tokenize(r+2);

	  ret = phAddAuthInfo(u, id, pass, NULL, r);
	  
	}
      else if (!keywdcmp(buf, "REG"))
	{
	  char *u = strstr(buf, "U=");
	  char *s = strstr(buf, "S=");
	  
	  u = tokenize(u+2);

	  ret = phRegister(u, s+2);
	}
      else if (!keywdcmp(buf, "DTMF"))
	{
	  char *evt = strstr(buf, "E=");
	  char *mode = strstr(buf, "MODE=");
	  
	  ret = phSendDtmf(cid, atoi(evt+2), atoi(mode+5));
	}
      else if (!keywdcmp(buf, "SETVOL"))
	{
	  char *vol = strstr(buf, "V=");
	  
	  ret = phSetSpeakerVolume(cid, atoi(vol+2));
	}
      else if (!keywdcmp(buf, "SETLVL"))
	{
	  char *lev = strstr(buf, "L=");
	  
	  ret = phSetRecLevel(cid, atoi(lev+2));
	}
      else if (!keywdcmp(buf, "PLAY"))
	{
	  char *loop = strstr(buf, "LOOP=");
	  char *file = strstr(buf, "FILE=");

	  ret = phPlaySoundFile(file+5, atoi(loop+5));
	}
      else if (!keywdcmp(buf, "PLSTOP"))
	{
	  ret = phStopSoundFile();
	}
      else if (!keywdcmp(buf, "TERM"))
	{
	ret = 0;
	termflag = 1;
	phTerminate();
	}
      if (!keywdcmp(buf, "VER"))
	ret = phGetVersion();


      snprintf(ans, sizeof(ans), "RET %d", ret);

      i = send(reqsock, ans, strlen(ans)+1, 0);
      if (i < 0)
	{
	  phTerminate();
	  exit(2);
	}

      if (termflag)
	return;
    }
 

}

	  
  
