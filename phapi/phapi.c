/*
 * phapi   phone api
 *
 * Copyright (C) 2004        Vadim Lebedev <vadim@mbdsys.com>
 * Parts Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
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
 * @file phapi.c
 * @brief softphone  API
 *
 * phapi is a library providing simplified api to create VOIP sessions
 * using eXosip library oSIP stack and oRTP stcak 
 * <P>
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#ifndef WIN32
#include "config.h"
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#else
#define snprintf _snprintf
#define strncasecmp strnicmp
#endif

#include <osip2/osip_mt.h>

#include <eXosip/eXosip.h>
#include <eXosip/eXosip_cfg.h>

#include "phapi.h"
#include "phcall.h"
#include "phrpc.h"
#include "phmedia.h"






phcall_t *ph_locate_call_by_cid(int cid);
phcall_t *ph_locate_call(eXosip_event_t *je, int creatit);
void ph_release_call(phcall_t *ca);
static  int timeout = 1;

static char * _get_local_sdp_port();



phCallbacks_t *phcb;

int phDebugLevel = 0;
char *phLogFileName = 0;


unsigned short phCallBackPort = PH_CALLBACK_PORT; 
unsigned short phServerPort = PH_SERVER_PORT; 

static int ph_busyFlag;


static FILE *ph_log_file;

#define ph_printf printf

void *
ph_api_thread(void *arg);

#define name(x) #x

static char *evtnames[] =
  {
  name(EXOSIP_REGISTRATION_NEW),              /* announce new registration.       */
  name(EXOSIP_REGISTRATION_SUCCESS),          /* user is successfully registred.  */
  name(EXOSIP_REGISTRATION_FAILURE),          /* user is not registred.           */
  name(EXOSIP_REGISTRATION_REFRESHED),        /* registration has been refreshed. */
  name(EXOSIP_REGISTRATION_TERMINATED),       /* UA is not registred any more.    */

  /* for UAC events */
  name(EXOSIP_CALL_NOANSWER),        /* announce no answer within the timeout */
  name(EXOSIP_CALL_PROCEEDING),      /* announce processing by a remote app   */
  name(EXOSIP_CALL_RINGING),         /* announce ringback                     */
  name(EXOSIP_CALL_ANSWERED),        /* announce start of call                */
  name(EXOSIP_CALL_REDIRECTED),      /* announce a redirection                */
  name(EXOSIP_CALL_REQUESTFAILURE),  /* announce a request failure            */
  name(EXOSIP_CALL_SERVERFAILURE),   /* announce a server failure             */
  name(EXOSIP_CALL_GLOBALFAILURE),   /* announce a global failure             */

  /* for UAS events */
  name(EXOSIP_CALL_NEW),             /* announce a new call                   */
  name(EXOSIP_CALL_ACK),             /* ACK received for 200ok to INVITE      */
  name(EXOSIP_CALL_CANCELLED),       /* announce that call has been cancelled */
  name(EXOSIP_CALL_TIMEOUT),         /* announce that call has failed         */
  name(EXOSIP_CALL_HOLD),            /* audio must be stopped                 */
  name(EXOSIP_CALL_OFFHOLD),         /* audio must be restarted               */
  name(EXOSIP_CALL_CLOSED),          /* a BYE was received for this call      */

  /* for both UAS & UAC events */
  name(EXOSIP_CALL_STARTAUDIO),         /* audio must be established           */
  name(EXOSIP_CALL_RELEASED),           /* call context is cleared.            */

  /* for UAC events */
  name( EXOSIP_OPTIONS_NOANSWER),        /* announce no answer within the timeout */
  name(EXOSIP_OPTIONS_PROCEEDING),      /* announce processing by a remote app   */
  name(EXOSIP_OPTIONS_ANSWERED),        /* announce a 200ok                      */
  name(EXOSIP_OPTIONS_REDIRECTED),      /* announce a redirection                */
  name(EXOSIP_OPTIONS_REQUESTFAILURE),  /* announce a request failure            */
  name(EXOSIP_OPTIONS_SERVERFAILURE),   /* announce a server failure             */
  name(EXOSIP_OPTIONS_GLOBALFAILURE),   /* announce a global failure             */

  name(EXOSIP_INFO_NOANSWER),        /* announce no answer within the timeout */
  name(EXOSIP_INFO_PROCEEDING),      /* announce processing by a remote app   */
  name(EXOSIP_INFO_ANSWERED),        /* announce a 200ok                      */
  name(EXOSIP_INFO_REDIRECTED),      /* announce a redirection                */
  name(EXOSIP_INFO_REQUESTFAILURE),  /* announce a request failure            */
  name(EXOSIP_INFO_SERVERFAILURE),   /* announce a server failure             */
  name(EXOSIP_INFO_GLOBALFAILURE),   /* announce a global failure             */

  /* for UAS events */
  name(EXOSIP_OPTIONS_NEW),             /* announce a new options method         */
  name(EXOSIP_INFO_NEW),               /* new info request received           */

  name(EXOSIP_MESSAGE_NEW),            /* announce new incoming MESSAGE. */
  name(EXOSIP_MESSAGE_SUCCESS),        /* announce a 200ok to a previous sent */
  name(EXOSIP_MESSAGE_FAILURE),        /* announce a failure. */


  /* Presence and Instant Messaging */
  name(EXOSIP_SUBSCRIPTION_NEW),          /* announce new incoming SUBSCRIBE.  */
  name(EXOSIP_SUBSCRIPTION_UPDATE),       /* announce incoming SUBSCRIBE.      */
  name(EXOSIP_SUBSCRIPTION_CLOSED),       /* announce end of subscription.     */

  name(EXOSIP_SUBSCRIPTION_NOANSWER),        /* announce no answer              */
  name(EXOSIP_SUBSCRIPTION_PROCEEDING),      /* announce a 1xx                  */
  name(EXOSIP_SUBSCRIPTION_ANSWERED),        /* announce a 200ok                */
  name(EXOSIP_SUBSCRIPTION_REDIRECTED),      /* announce a redirection          */
  name(EXOSIP_SUBSCRIPTION_REQUESTFAILURE),  /* announce a request failure      */
  name(EXOSIP_SUBSCRIPTION_SERVERFAILURE),   /* announce a server failure       */
  name(EXOSIP_SUBSCRIPTION_GLOBALFAILURE),   /* announce a global failure       */
  name(EXOSIP_SUBSCRIPTION_NOTIFY),          /* announce new NOTIFY request     */

  name(EXOSIP_SUBSCRIPTION_RELEASED),        /* call context is cleared.        */

  name(EXOSIP_IN_SUBSCRIPTION_NEW),          /* announce new incoming SUBSCRIBE.*/
  name(EXOSIP_IN_SUBSCRIPTION_RELEASED)     /* announce end of subscription.   */

  };

#undef name



phcall_t ph_calls[PH_MAX_CALLS];



phConfig_t phcfg = { "10600", "", "5060" };

static char *
_get_local_sdp_port()
{
  return phcfg.local_rtp_port;
}



phcall_t *
ph_locate_call_by_cid(int cid)
{
  phcall_t *ca;


  for(ca = ph_calls; ca < &ph_calls[PH_MAX_CALLS];  ca++)
    {
      if (ca->cid == cid)
	return ca;
    }

  return 0;
}

phcall_t *
ph_locate_call(eXosip_event_t *je, int creatit)
{
  phcall_t *ca, *found = 0, *newca = 0;


  /* lookup matching call descriptor */
  for(ca = ph_calls; ca < &ph_calls[PH_MAX_CALLS];  ca++)
    {
      if (ca->cid == -1 && !newca)
	newca = ca;

      if (ca->cid == je->cid)
	{
	  found  = ca;
	  break;
	}
    }


  ca = found;

  if (!ca)   /* we didn't find a matching call descriptor */
    {
    if (creatit)   
      {
	/* allocate a new one */
	ca = newca;
	memset(ca, 0, sizeof(*ca));
	ca->cid = -1;
      }
    }

 
  
  if (!ca)
    return 0;

  /* update the call information */
  ca->cid = je->cid;
  ca->did = je->did;

  if (je->remote_sdp_audio_ip[0])
    {
      strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, sizeof(ca->remote_sdp_audio_ip));
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      strncpy(ca->payload_name, je->payload_name, sizeof(ca->payload_name));
      ca->payload = je->payload;
    }


  return ca;
}


void ph_release_call(phcall_t *ca)
{
  if (ca->hasaudio)
    {
      ph_media_stop(ca);
      ca->cid = -1;
    }
}



void ph_wegot_dtmf(phcall_t *ca, int dtmfEvent);

void ph_wegot_dtmf(phcall_t *ca, int dtmfEvent)
{
    phCallStateInfo_t info;

    info.event = phDTMF;
    info.u.dtmfDigit = dtmfEvent;
    phcb->callProgress(ca->cid, &info);

}


int 
phGetVersion()
{
  static char version[] = PHAPI_VERSION;
  char *subv = strstr(version, ".");
  int v,s,r;

  v = atoi(version);
  s = atoi(subv+1);
  r = atoi(strstr(subv+1, ".")+1);

  return (v << 16) | (s << 8) | r;
} 

int 
phPlaceCall(const char *from, const char *uri, void *userdata)
{
  int i;
  osip_message_t *invite;

  i = eXosip_build_initial_invite(&invite,
				  uri,
				  from,
				  phcfg.proxy,
				  "");
  if (i!=0)
      return -1;

  eXosip_lock();
  i = eXosip_initiate_call(invite, userdata, NULL, _get_local_sdp_port());
  eXosip_unlock();  
  return i;

}


int 
phAcceptCall2(int cid, void *userData)
{
  int i;
  phcall_t *ca = ph_locate_call_by_cid(cid);
  
  if (!ca)
    return -1;

  eXosip_lock();
  i = eXosip_answer_call(ca->did, 200, _get_local_sdp_port());
  if (i == 0)
    i = eXosip_retrieve_negotiated_payload(ca->did, &ca->payload, ca->payload_name, sizeof(ca->payload_name));
  eXosip_unlock();

  if (i == 0)
  {
    if (ph_media_start(ca, atoi(_get_local_sdp_port()), ph_wegot_dtmf, phcfg.audio_dev))
      i = -1;
  }

  return i;
}

  


int 
phRejectCall(int cid, int reason)
{
  int i;
  phcall_t *ca = ph_locate_call_by_cid(cid);
  
  if (!ca)
    return -1;

  eXosip_lock();
  i = eXosip_answer_call(ca->did, reason, 0);
  eXosip_unlock();

  return i;

}


int 
phRingingCall(int cid)
{
  int i;
  phcall_t *ca = ph_locate_call_by_cid(cid);
  
  if (!ca)
    return -1;

  eXosip_lock();
  i = eXosip_answer_call(ca->did, 180, 0);
  eXosip_unlock();

  return i;

}



int 
phCloseCall(int cid)
{
  int i;
  phcall_t *ca = ph_locate_call_by_cid(cid);
  phCallStateInfo_t info;
  
  if (!ca)
    return -1;


  eXosip_lock();
  i = eXosip_terminate_call(ca->cid, ca->did);
  eXosip_unlock();

  if (ca)
    ph_release_call(ca);

  if (i)
    return i;

  info.userData = 0;
  info.event = phCALLCLOSED;
  info.u.errorCode = 0;
  phcb->callProgress(cid, &info);

  
  return i;

}



int 
phBlindTransferCall(int cid, const char *uri)
{
  int i;
  phcall_t *ca = ph_locate_call_by_cid(cid);

  if (!ca)
    return -1;

  eXosip_lock();
  i = eXosip_transfer_call(ca->did, uri);
  eXosip_unlock();

  return i;

}


int 
phTransferCall(int cid, int tcid)
{
  return -1;
}


int 
phResumeCall(int cid)
{
  phcall_t *ca = ph_locate_call_by_cid(cid);
  int i;


  if (!ca)
    return -1;

  if (!ca->localhold)
    return -2;



  ca->localhold = 0;
  ca->localresume = 1;

  eXosip_lock();
  i = eXosip_off_hold_call(ca->did, 0, 0);
  eXosip_unlock();

  return i;
}


int 
phHoldCall(int cid)
{
  phcall_t *ca = ph_locate_call_by_cid(cid);
  int i;


  if (!ca)
    return -1;

  if (ca->localhold)
    return -2;

  ca->localhold = 1;

  eXosip_lock();
  i = eXosip_on_hold_call(ca->did);
  eXosip_unlock();

  if (!i && ca->hasaudio)
    ph_media_stop(ca);



  return i;
}


int 
phSetFollowMe(const char *uri)
{
  return -1;
}

int 
phSetBusy(int busyFlag)
{
  ph_busyFlag = busyFlag;
  return 0;
}


int
phAddAuthInfo(const char *username, const char *userid,
			       const char *passwd, const char *ha1,
	      const char *realm)
{
  int ret;

  eXosip_lock();

  ret = eXosip_add_authentication_info(username, userid, passwd, ha1, realm);

  eXosip_unlock();

  return ret;
}

int 
phRegister(const char *username, const char *server)
{
  int rid;
  int ret = -1;
  char tmp[256];
  char utmp[256];


  if (0 != strncasecmp(username, "sip:", 4)) 
    {
      snprintf(utmp, sizeof(utmp), "sip:%s@%s", username,server);
      username = utmp;
    }

  if (0 != strncasecmp(server, "sip:", 4)) 
    {
      snprintf(tmp, sizeof(tmp), "sip:%s", server);
      server = tmp;
    }



  eXosip_lock();

  rid = eXosip_register_init((char *) username, (char *) server, 0);

  if (rid >= 0)
    {
      ret = eXosip_register(rid, 3600);

      if (ret == 0)
	ret = rid;
    }

  eXosip_unlock();

  return ret;

}


 
int 
phSendDtmf(int cid, int dtmfEvent, int mode)
{
  phcall_t *ca = ph_locate_call_by_cid(cid);

  if (!ca || !ca->hasaudio)
    return -1;

  return ph_media_send_dtmf(ca, dtmfEvent, mode);

}





int 
phPlaySoundFile(const char *fileName , int loop)
{
  return -1;
}




int 
phStopSoundFile()
{
  return -1;
}


int phSetSpeakerVolume(int cid,  int volume)
{
  return -1;
}


int phSetRecLevel(int cid,  int level)
{
  return -1;
}


static void setup_payload(const char *ptsring)
{
  char  tmp[64];
  char  num[8];
  ph_media_payload_t  pt;

  if (ph_media_supported_payload(&pt, ptsring))
    {
      snprintf(num, sizeof(num), "%d", pt.number);
      snprintf(tmp, sizeof(tmp), "%d %s/%d/1", pt.number, pt.string, pt.rate);

      eXosip_sdp_negotiation_add_codec(osip_strdup(num),
				   NULL,
				   osip_strdup("RTP/AVP"),
				   NULL, NULL, NULL,
				   NULL,NULL,
				   osip_strdup(tmp));
    }
}




/**
 * Initialize the phoneapi module
 */
int 
phInit(phCallbacks_t *cbk, char * server, int asyncmode)
{
  int i;


  if (phDebugLevel > 0)
    {
      ph_log_file = phLogFileName ? fopen (phLogFileName, "w+") : stdout;

      if (!ph_log_file)
        {
          perror ("phapi: log file");
          return -1;
        }
      TRACE_INITIALIZE (phDebugLevel, ph_log_file);
    }



  for( i = 0; i < PH_MAX_CALLS; i++)
    ph_calls[i].cid = -1;


  if (phcfg.proxy[0])
    {
      char tmp[256];

      if (!strchr(phcfg.proxy, '<'))
	{
	snprintf(tmp, sizeof(tmp), "<sip:%s;lr>", phcfg.proxy); 
	strncpy(phcfg.proxy, tmp, sizeof(phcfg.proxy));
	}
    }

  ph_media_init();

  i = eXosip_init(0, 0, atoi(phcfg.sipport));
  if (phcfg.force_proxy)
    eXosip_force_proxy(phcfg.proxy);

  {
    char contact[512];
   
    eXosip_guess_contact_uri(phcfg.identity, contact, sizeof(contact), 1);
    eXosip_set_answer_contact(contact);
  }


  if (i!=0)
    return i;

  /* reset all payload to fit application capabilities */
  eXosip_sdp_negotiation_remove_audio_payloads();

  if (phcfg.nattype[0])
    {
      if (!strncmp(phcfg.nattype, "auto", 4))
	return -1;
      if (!strncmp(phcfg.nattype, "fcone", 5) ||
	  !strncmp(phcfg.nattype, "rcone", 5) ||
	  !strncmp(phcfg.nattype, "prcone", 6) ||
	  !strncmp(phcfg.nattype, "sym", 3))
	eXosip_set_nattype(phcfg.nattype);

    }




  if (!phcfg.codecs[0])
    {
      setup_payload("PCMU/8000");
      setup_payload("PCMA/8000");
      setup_payload("GSM/8000");
      setup_payload("ILBC/8000");
    }
  else
    {
      char tmp[32];
      char *tok = strtok(phcfg.codecs, ",");

      while(tok)
	{
	  snprintf(tmp, sizeof(tmp), "%s/8000", tok);
	  setup_payload(tmp);
	  tok = strtok(0, ",");
	}
    }



      

  setup_payload("telephone-event/8000");



  /* register callbacks? */
  eXosip_set_mode(EVENT_MODE);
  phcb = cbk;
  phcfg.asyncmode = asyncmode;
  timeout = 1;

  if (asyncmode)
    osip_thread_create(20000, ph_api_thread, 0);

  return 0;
     
}


/**
 * terminate ph api
 */
void phTerminate()
{
  int i;

  for(i = 0; i < PH_MAX_CALLS; i++)
    if (ph_calls[i].cid != -1)
      ph_release_call(&ph_calls[i]);

  eXosip_quit();
  
  ph_media_cleanup();

  if (phLogFileName && phDebugLevel > 0)
    fclose(ph_log_file);
  if (phDebugLevel > 0)
    for (i = 0; i <= phDebugLevel && i < END_TRACE_LEVEL; ++i)
      TRACE_DISABLE_LEVEL(i);
}

 

/**
 * poll for phApi events
 */
void phPoll()
{
  int ph_event_get();

  if (!phcfg.asyncmode)
    ph_event_get();
}


void 
ph_call_new(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;

  if (ph_busyFlag)
    {
      eXosip_lock();
      eXosip_answer_call(je->did, 486, 0);
      eXosip_unlock();
      return;
    }


  ca = ph_locate_call(je, 1);
  if (ca)
  {
	  

    info.userData = je->external_reference;
    info.event = phINCALL;
    info.u.remoteUri = je->remote_uri; 
    info.localUri = je->local_uri;
    
    phcb->callProgress(ca->cid, &info);
  }

}


void 
ph_call_answered(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;



  ca = ph_locate_call(je, 1);
  if (je->remote_sdp_audio_ip[0] && !ca->localhold && !ca->remotehold)
      ph_media_start(ca, atoi(_get_local_sdp_port()), ph_wegot_dtmf, phcfg.audio_dev);


  info.localUri = je->local_uri;
  info.userData = je->external_reference;
  if (ca->localhold)
    info.event = phHOLDOK;
  else if (ca->localresume)
    {
      info.event = phRESUMEOK;
      ca->localresume = 0;
    }
  else
      info.event = phCALLOK;

  info.u.remoteUri = je->remote_uri; 

  phcb->callProgress(ca->cid, &info);



}


void 
ph_call_proceeding(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;



  ca = ph_locate_call(je, 1);

  if (je->remote_sdp_audio_ip[0] && !ca->localhold)
      ph_media_start(ca, atoi(_get_local_sdp_port()), ph_wegot_dtmf, phcfg.audio_dev);

  
  info.userData = je->external_reference;
  info.event = phDIALING;
  info.u.remoteUri = je->remote_uri; 

  phcb->callProgress(ca->cid, &info);



}

void 
ph_call_redirected(eXosip_event_t *je)
{
  phCallStateInfo_t info;

  
  info.localUri = je->local_uri;
  info.userData = je->external_reference;
  info.event = phCALLREDIRECTED;
  info.u.remoteUri = je->remote_uri; 

  phcb->callProgress(je->cid, &info);



}





void 
ph_call_ringing(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;


  ca = ph_locate_call(je, 1);

  if (je->remote_sdp_audio_ip[0] && !ca->localhold)
      ph_media_start(ca, atoi(_get_local_sdp_port()), ph_wegot_dtmf, phcfg.audio_dev);
  
  info.localUri = je->local_uri;
  info.userData = je->external_reference;
  info.event = phRINGING;
  info.u.remoteUri = je->remote_uri; 

  phcb->callProgress(je->cid, &info);


}


void 
ph_call_requestfailure(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;
  int i;


  if (je->status_code == 401 || 407 == je->status_code) /* need authetication? */
    {
      eXosip_lock();
      i = eXosip_retry_call(je->cid);
      eXosip_unlock();
      return;
    }



  ca = ph_locate_call(je, 0);
  if (ca)
    ph_release_call(ca);

  
  info.localUri = je->local_uri;
  info.userData = je->external_reference;
  if (je->status_code == 486)
    {
      info.event = phCALLBUSY;
      info.u.remoteUri = je->remote_uri;
    }
  else
    {
      info.event = phCALLERROR;
      info.u.errorCode = je->status_code;
    }

  
  phcb->callProgress(je->cid, &info);



}


void 
ph_call_serverfailure(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;

  ca = ph_locate_call(je, 0);
  if (ca)
    ph_release_call(ca);

  info.localUri = je->local_uri;
  info.userData = je->external_reference;
  info.event = phCALLERROR;
  info.u.errorCode = je->status_code;

  phcb->callProgress(je->cid, &info);





}

void 
ph_call_globalfailure(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;


  ca = ph_locate_call(je, 0);
  if (ca)
    ph_release_call(ca);


  info.userData = je->external_reference;
  info.localUri = je->local_uri;

  if (je->status_code == 600)
    {
      info.event = phCALLBUSY;
      info.u.remoteUri = je->remote_uri;
    }
  else
    {
      info.event = phCALLERROR;
      info.u.errorCode = je->status_code;
    }

  phcb->callProgress(je->cid, &info);


}

void 
ph_call_noanswer(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;
  
  ca = ph_locate_call(je, 1);
  if (ca)
    ph_release_call(ca);

  info.userData = je->external_reference;
  info.event = phNOANSWER;
  info.u.remoteUri = je->remote_uri;
  info.localUri = je->local_uri;

  phcb->callProgress(je->cid, &info);




}



void 
ph_call_closed(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;


  ca = ph_locate_call(je, 0);
  if (ca)
    ph_release_call(ca);

  
  info.userData = je->external_reference;
  info.event = phCALLCLOSED;
  info.u.errorCode = 0;
  phcb->callProgress(je->cid, &info);




}


void 
ph_call_onhold(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;


  ca = ph_locate_call(je, 0);
  
  if (!ca)
    return;

  if (ca->hasaudio)
    ph_media_stop(ca);

  //  ph_media_suspend(ca);

  ca->remotehold = 1;
  info.userData = je->external_reference;
  info.event = phCALLHELD;
  phcb->callProgress(je->cid, &info);

  


}

void 
ph_call_offhold(eXosip_event_t *je)
{
  phCallStateInfo_t info;
  phcall_t *ca;



  ca = ph_locate_call(je, 0);
  if (!ca)
    return;


  // ph_media_resume(ca);

  if (ca->hasaudio)
      ph_media_stop(ca);
  
  if (!ca->localhold)
    ph_media_start(ca, atoi(_get_local_sdp_port()), ph_wegot_dtmf, phcfg.audio_dev);
  



  if (ca->remotehold)
    {
      info.userData = je->external_reference;
      info.event = phCALLRESUMED;
      phcb->callProgress(je->cid, &info);
    }
  
  ca->remotehold = 0;

}


void ph_reg_progress(eXosip_event_t *je)
{
  if (je->type == EXOSIP_REGISTRATION_SUCCESS)
    phcb->regProgress(je->rid, 0);
  else if (je->type == EXOSIP_REGISTRATION_FAILURE)
    {
      if (je->status_code == 401 || je->status_code == 407)
	{
	  eXosip_lock();
	  eXosip_register(je->rid, 3600);
	  eXosip_unlock();
	  return;
	}
    phcb->regProgress(je->rid, je->status_code ? je->status_code : -1 );
    }
}



int
ph_event_get()
{
  int counter =0;
  /* use events to print some info */
  eXosip_event_t *je;
  for (;;)
    {
      je = eXosip_event_wait(0,timeout);
      if (je==NULL)
	break;
      counter++;

      ph_printf("\n<- %s (%i %i) [%i %s] %s requri=%s\n",
		   evtnames[je->type], je->cid, je->did, 
		   je->status_code,
		   je->reason_phrase,
		   je->remote_uri,
		   je->req_uri);

      switch(je->type)
	{
	case EXOSIP_CALL_NEW:
	  ph_call_new(je);
	  break;
	
	case EXOSIP_CALL_ANSWERED:
	  ph_call_answered(je);
	  break;
	case EXOSIP_CALL_PROCEEDING:
	  ph_call_proceeding(je);
	  break;

	case EXOSIP_CALL_RINGING:
	  ph_call_ringing(je);
	  break;

	case EXOSIP_CALL_REDIRECTED:
	  ph_call_redirected(je);
	  break;

	case EXOSIP_CALL_REQUESTFAILURE:
	  ph_call_requestfailure(je);
	  break;

	case EXOSIP_CALL_SERVERFAILURE:
	  ph_call_serverfailure(je);
	  break;

	case EXOSIP_CALL_GLOBALFAILURE:
	  ph_call_globalfailure(je);
	  break;

	case EXOSIP_CALL_NOANSWER:
	  ph_call_noanswer(je);
	  break;

	case EXOSIP_CALL_CLOSED:
	  ph_call_closed(je);
	  break;

	case EXOSIP_CALL_HOLD:
	  ph_call_onhold(je);
	  break;

	case EXOSIP_CALL_OFFHOLD:
	  ph_call_offhold(je);
	  break;

	case EXOSIP_REGISTRATION_SUCCESS:
	  ph_reg_progress(je);
	  break;

	case EXOSIP_REGISTRATION_FAILURE:
	  ph_reg_progress(je);
	  break;

	default:
	  ph_printf("event(%i %i %i %i) text=%s", je->cid, je->sid, je->nid, je->did, je->textinfo);
	  break;
	}
	
      eXosip_event_free(je);
    }

  if (counter>0)
    return 0;
  return -1;
}

/** 
 * eXosip event reader thread
 */
void *
ph_api_thread(void *arg)
{

  timeout = 50;
  while(1)
    ph_event_get();

  return 0;
}
