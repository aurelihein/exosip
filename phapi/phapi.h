#ifndef __PHAPI_H__
#define __PHAPI_H__ 1
/*
  The phapi module implements simple interface for eXosip stack
  Copyright (C) 2004  Vadim Lebedev  <vadim@mbdsys.com>
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * @file phoneapi.h
 * @brief softphone  API
 *
 * phoneapi is a library providing simplified api to create VOIP sessions
 * using eXosip library oSIP stack and oRTP stack 
 * <P>
 */


/**
 * @defgroup phAPI  Phone API
 * @{
 *  
 *  From the perspecitve of the phApi client the call can be in the following states:
 *   
 *   [INCOMING], [ACCEPTING], [OUTGOING], [ESTABLISHED], [ONHOLD], [CLOSED]
 *
 *                
 *                                              V
 *                           +--(INCALL)--------+----(phPlaceCall)-------------------+
 *                           |                                                       |
 *                           |                                                       |
 *                           v                                                       v           
 *  +--(phRejectCall)--<-[INCOMING]<----<------- +                               [OUTGOING]--(DIALING/RINGING)-->-+
 *  |                        v                   ^                                v   v  ^                        |
 *  |                        |                   |                                |   |  |                        |
 *  |                        +--(phRingingCall)--+                                |   |  |                        |
 *  |                        |                                                    |   |  |                        |
 *  |                        v                                                    |   |  |                        |
 *  |                   (phAcceptCall)            +--------+                      |   |  +------------------------+
 *  |                        |                 (DTMF)      |                      |   |
 *  |                        |                    ^        |                  (CALLOK)|
 *  |                        v                    |        v                      v   |
 *  |                  [ACCEPTING]->--(ret==0)-->[ESTABLISHED]<--+---------+------+   +-------->+
 *  |                        |                    v        v               ^                    v
 *  |                        |                    |        |               |                    |
 *  |                (CALLCLOSED/CALLERROR)       +   (CALLHELD/HOLDOK)    |    (CALLCLOSED/CALLERROR/CALLBUSY/NOANSWER/REDIRECTED)
 *  |                        |                    |        |               |                    |
 *  |                        |             (CALLCLOSED)    |               ^                    |
 *  |                        v                    |        +->[ONHOLD]->(CALLRESUMED/RESUMEOK)  |
 *  |                        |                    |             v                               |
 *  |                        |                    |             |                               |
 *  |                        |                    +<-----(CALLCLOSED)                           |
 *  |                        |                    |                                             |
 *  v                        |                    v                                             v
 *  +------------------------+------->[CLOSED]<---+---------------------------------------------+
 *
 *
 */


#define PHAPI_VERSION "0.0.1"

#ifdef __cplusplus
extern "C"
{
#endif




  
/**
 * Place an outgoing call
 * 
 * @param from         call originator address
 * @param uri          call destination address
 * @param userData     application specific data
 * @return             if positive the call id else error indication
 */
int phPlaceCall(const char *from, const char *uri, void *userData);



/**
 * Accept an incoming a call.
 * 
 * @param cid          call id of call.
 * @return             0 in case of success
 */
int phAcceptCall2(int cid, void *userData);

#define phAcceptCall(cid) phAcceptCall2(cid, 0)

/**
 * Reject the incoming call.
 * 
 * @param cid          call id of call.
 * @param reason       SIP reason code for the rejection
 *                     suggested values: 486 - Busy Here, 488 - Not acceptable here
 * @return             0 in case of success
 */
int phRejectCall(int cid, int reason);


/**
 * Signal reining event to the remote side.
 * 
 * @param cid          call id of call.
 * @return             0 in case of success
 */
int phRingingCall(int cid);



/**
 * Terminate a call.
 * 
 * @param cid          call id of call.
 * @return             0 in case of success
 */
int phCloseCall(int cid);


/**
 * Perform a blind call transfer
 * 
 * @param cid          call id of call.
 * @param uri          call transfer destination address
 * @return             txid  used in the subsequent transferProgress callback
 */
int phBlindTransferCall(int cid, const char *uri);


/**
 * Perform an assisted  call transfer
 * 
 * @param cid          call id of call.
 * @param targetCid    call id of the destination call
 * @return             txid  used in the subsequent transferProgress callback
 */
int phTransferCall(int cid, int targetCid);



/**
 * Resume previously held call
 * 
 * @param cid          call id of call.
 * @return             0 in case of success
 */
int phResumeCall(int cid);


/**
 * Put a call on hold
 * 
 * @param cid          call id of call.
 * @return             0 in case of success
 */
int phHoldCall(int cid);



/**
 * Send a DTMF to remote party
 * 
 * @param cid          call id of call.
 * @param dtmfChar     DTMF event to send 
 *                     ('0','1','2','3','4','5','6','7','8','9','0','#','A','B','C','D','!')
 * @mode               bitmask specifying DTMF geneartion mode 
 *                     INBAND - the DTMF signal is mixed in the outgoing
 *                     RTPPAYLOAD - the DTMF signal will be sent using telephone_event RTP payload 
 * @return             0 in case of success
 */
#define PH_DTMF_MODE_INBAND 1
#define PH_DTMF_MODE_RTPPAYLOAD 2
#define PH_DTMF_MODE_ALL 3  
int phSendDtmf(int cid, int dtmfChar, int mode);





/**
 * Play a sound file
 * 
 * @param fileName     file to play (the file externsion will determine the codec to use
 *                     .sw - 16bit signed PCM, .ul - uLaw, .al - aLaw, .gsm - GSM, (.wav on Windows)
 * @param loop         when TRUE play the file in loop
 * @return             0 in case of success
 */
int phPlaySoundFile(const char *fileName , int loop);



/**
 * Stop playing a sound file
 * 
 * @return             0 in case of success
 */
int phStopSoundFile( void );


/**
 * Set speaker volume  
 * 
 * @param      cid       call id (-1 for general volume, -2 for playing sounds)
 * @param      volume    0 - 100
 * @return             0 in case of success
 */
int phSetSpeakerVolume(int cid,  int volume);


/**
 * Set recording level  
 * 
 * @param      cid - call id (-1 for general recording level)
 * @param      level    0 - 100
 * @return             0 in case of success
 */
int phSetRecLevel(int cid,  int level);



/**
 * Configure follow me address. All 
 * incoming call will be redirected to this address
 * 
 * @param uri          destination of the forwarding
 * @return             0 in case of success
 */
int phSetFollowMe(const char *uri);


/**
 * Set busy mode
 * When activated all incoming calls will be answerd by busy signal
 * 
 * @param busyFlag          when 0 busy mode is deactivated else activated
 * @return             0 in case of success
 */
int phSetBusy(int busyFlag);




/**
 * Add authentication info
 * the given info will be to send as authentication information 
 * when server request it.
 * 
 * @param  username    username which will figure in the From: headers 
 *                     (usually the same as userid)
 * @param  userid      userid field value
 * @param  realm       authentication realm
 * @param  passwd      password correspoinding to the userid
 * @return             0 in case of success
 */
int
phAddAuthInfo(const char *username, const char *userid,
	      const char *passwd, const char *ha1,
	      const char *realm);



/**
 * Register to the server
 * Try to refister the given user name at the server
 * (This API is provisional, its fucntionality will be eventually subsumed by
 *  phAddAccount call which will specify user identity, registration server, and proxy server)
 * 
 * @param  server    the name or ip adderss of the registration server
 * @param  username  username to be used in the From: header
 * @return           if positive registration id which will be used in regProgress callback
 * 
 */
int 
phRegister(const char *username, const char *server);


  


/**
 * @enum phCallStateEvent
 *  @brief call progress events.
 *
 */ 
enum  phCallStateEvent { 
  phDIALING, phRINGING, phNOANSWER, phCALLBUSY, phCALLREDIRECTED, phCALLOK, 
  phCALLHELD, phCALLRESUMED, phHOLDOK, phRESUMEOK, 
  phINCALL, phCALLCLOSED, phCALLERROR, phDTMF
};

/**
 * @struct phCallStateInfo
 */
struct phCallStateInfo {
  enum phCallStateEvent event;
  void *userData;              /*!< used to match placeCall with callbacks */ 
  const char *localUri;        /*!< valid for all events execpt CALLCLOSED and DTMF */
  union {
    const char  *remoteUri;    /*!< valid for all events execpt CALLCLOSED, DTMF and CALLERROR */
    int   errorCode;           /*!< valid for CALLERROR */
    int   dtmfDigit;           /*!< valid for DTMF */  
  } u;
};




typedef struct phCallStateInfo phCallStateInfo_t;
typedef struct phTransferStateInfo phTransferStateInfo_t;
typedef struct phConfStateInfo phConfStateInfo_t;
typedef struct phRegStateInfo phRegStateInfo_t; 

/**
 * @struct phCallbacks
 * @brief  callbacks to the MMI
 */  
struct phCallbacks
{
  void  (*callProgress)(int cid, const phCallStateInfo_t *info);       /*!< call progress callback routine */
  void  (*transferProgress)(int cid, const phTransferStateInfo_t *info); /*!< transfer progress callback routine */
  void  (*confProgress)(int cfid, const phConfStateInfo_t *info);        /*!< conference progress callback routine */
  void  (*regProgress)(int regid, int regStatus);                       /*!< registration status (0 - OK, else SIP error code */
};


typedef struct phCallbacks phCallbacks_t;

/**
 * @var phcb
 * @brief pointer to callback structure
 * 
 */

extern phCallbacks_t *phcb;



/**
 * Initilize phApi
 *
 * @param cbk          pointer to callback descriptor
 * @param server       string containing an ip address of the phApi server
 *                     (ignored when in direct link mode)
 * @param asyncmode    when != 0 a thread will be created to deliver
 *                     callbacks asyncronously, othewise the client
 *                     is supposed to call phPoll periodically to get
 *                     phApi events delivered
 */
int phInit(phCallbacks_t *cbk, char *server, int asyncmode);




/**
 * Get the version of the phAPI module
 *
 * @return  encoded value corresponding to Version.Subversion.Release
 *
 */
int phGetVersion(void);


#define phRelease(v) ((v) & 0xff)
#define phSubversion(v) ((v >> 8) & 0xff)
#define phVersion(v) ((v >> 16) & 0xff)



/**
 * poll for phApi events
 */
void phPoll( void );


/**
 *  Terminate phApi
 */
void phTerminate( void );     


/**
 * @struct phConfig
 * @brief ph API configuration info
 */
struct phConfig
{
  char local_rtp_port[16]; /*!< port number used for RTP data */ 
  char local_rtcp_port[16]; /*!< port number used for RTCP data */
  char sipport[16];         /*!< sip port number */
  char identity[256];       /*!< my sip address (this field is temporary hack) */
  char proxy[256];          /*!< proxy address (this field is temporary hack)  */ 
  char nattype[16];         /*!< nat type (auto,none,fcone,rcone,prcone,sym)  */
  char codecs[128];         /*!< comma separate list of codecs in order of priority */
                            /* example: PCMU,PCMA,GSM,ILBC,SPEEX   */
  int  force_proxy;         /*!< when set to 1 causes all SIP packet go through specifed proxy */
  int  asyncmode;
  char audio_dev[64];       /*!< audio device identifier */
			    /* example: IN=2 OUT=1 ; 2 is input device and 1 is ouput device */
};


typedef struct phConfig phConfig_t;

/**
 * @var phconfig
 * @brief variable storing the ph API configuration
 */
extern phConfig_t phcfg;

/**
 * variable storing the name of the log file
 */ 
extern char *phLogFileName;


/**
 * Debugging level (between 0 and 9)
 */ 
extern int phDebugLevel;




#ifdef __cplusplus
}
#endif

/** @} */

#endif









