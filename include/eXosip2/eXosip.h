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

#ifndef __EXOSIP_H__
#define __EXOSIP_H__

#include <eXosip2/eX_setup.h>
#include <eXosip2/eX_register.h>
#include <eXosip2/eX_call.h>
#include <eXosip2/eX_options.h>
#include <eXosip2/eX_subscribe.h>
#include <eXosip2/eX_refer.h>
#include <eXosip2/eX_message.h>
#include <eXosip2/eX_publish.h>

#include <osipparser2/osip_parser.h>
#include <osipparser2/sdp_message.h>
#include <time.h>

/**
 * @file eXosip.h
 * @brief eXosip API
 *
 * eXosip is a high layer library for rfc3261: the SIP protocol.
 * It offers a simple API to make it easy to use. eXosip2 offers
 * great flexibility for implementing SIP endpoint like:
 * <ul>
 * <li>SIP User-Agents</li>
 * <li>SIP Voicemail or IVR</li>
 * <li>SIP B2BUA</li>
 * <li>any SIP server acting as an endpoint (music server...)</li>
 * </ul>
 *
 * If you need to implement proxy or complex SIP applications,
 * you should consider using osip instead.
 *
 * Here are the eXosip capabilities:
 * <pre>
 *    REGISTER                 to handle registration.
 *    INVITE/BYE               to start/stop VoIP sessions.
 *    INFO                     to send DTMF within a VoIP sessions.
 *    OPTIONS                  to simulate VoIP sessions.
 *    re-INVITE                to modify VoIP sessions
 *    REFER/NOTIFY             to transfer calls.
 *    MESSAGE                  to send Instant Message.
 *    SUBSCRIBE/NOTIFY         to handle presence capabilities.
 *    any other request        to handle what you want!
 * </pre>
 * <P>
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Structure for event description.
 * @var eXosip_event_t
 */
  typedef struct eXosip_event eXosip_event_t;

/**
 * @defgroup eXosip2_authentication eXosip2 authentication API
 * @ingroup eXosip2_msg
 * @{
 */

/**
 * Add authentication credentials. These are used when an outgoing
 * request comes back with an authorization required response.
 *
 * @param username	username
 * @param userid	login (usually equals the username)
 * @param passwd	password
 * @param ha1		currently ignored
 * @param realm		realm within which credentials apply, or NULL
 *			to apply credentials to unrecognized realms
 */
  int eXosip_add_authentication_info (const char *username, const char *userid,
                                      const char *passwd, const char *ha1,
                                      const char *realm);

/**
 * Clear all authentication credentials stored in eXosip.
 *
 */
  int eXosip_clear_authentication_info (void);

/**
 * Initiate some default actions:
 *
 *  Retry with credentials upon reception of 401/407.
 *  Retry with Contact header upon reception of 3xx request.
 * 
 */
  int eXosip_default_action (eXosip_event_t * je);

/**
 *  Refresh REGISTER and SUBSCRIBE before the expiration delay.
 */
  void eXosip_automatic_refresh (void);

/**
 * Initiate some automatic actions:
 *
 *  Retry with credentials upon reception of 401/407.
 *  Refresh REGISTER and SUBSCRIBE before the expiration delay.
 *  Retry with Contact header upon reception of 3xx request.
 * 
 */
  void eXosip_automatic_action (void);

#ifndef MINISIZE
  /**
 * Automatic internal handling of known package.
 * 
 * @param evt          event related to an incoming subscription.
 */
  int eXosip_insubscription_automatic (eXosip_event_t * evt);
#endif

/**
 * Generate random string:
 *
 * @param buf	        destination buffer for random string.
 * @param buf_size      size of destination buffer
 */
  int eXosip_generate_random (char *buf, int buf_size);

/** @} */


/**
 * @defgroup eXosip2_sdp eXosip2 SDP helper API.
 * @ingroup eXosip2_msg
 * @{
 */

/**
 * Get remote SDP body for the latest INVITE of call.
 * 
 * @param did          dialog id of call.
 */
  sdp_message_t *eXosip_get_remote_sdp (int did);

/**
 * Get local SDP body for the latest INVITE of call.
 * 
 * @param did          dialog id of call.
 */
  sdp_message_t *eXosip_get_local_sdp (int did);

/**
 * Get local SDP body for the previous latest INVITE of call.
 * 
 * @param did          dialog id of call.
 */
  sdp_message_t *eXosip_get_previous_local_sdp (int did);

/**
 * Get remote SDP body for the latest INVITE of call.
 * 
 * @param tid          transction id of transaction.
 */
  sdp_message_t *eXosip_get_remote_sdp_from_tid (int tid);

/**
 * Get local SDP body for the latest INVITE of call.
 * 
 * @param tid          transction id of transaction.
 */
  sdp_message_t *eXosip_get_local_sdp_from_tid (int tid);

/**
 * Get local SDP body for the given message.
 * 
 * @param message      message containing the SDP.
 */
  sdp_message_t *eXosip_get_sdp_info (osip_message_t * message);

/**
 * Get audio connection information for call.
 * 
 * @param sdp     sdp information.
 */
  sdp_connection_t *eXosip_get_audio_connection (sdp_message_t * sdp);

/**
 * Get audio media information for call.
 * 
 * @param sdp     sdp information.
 */
  sdp_media_t *eXosip_get_audio_media (sdp_message_t * sdp);

/**
 * Get video connection information for call.
 * 
 * @param sdp     sdp information.
 */
  sdp_connection_t *eXosip_get_video_connection (sdp_message_t * sdp);

/**
 * Get video media information for call.
 * 
 * @param sdp     sdp information.
 */
  sdp_media_t *eXosip_get_video_media (sdp_message_t * sdp);

/**
 * Get media connection information for call.
 * 
 * @param sdp     sdp information.
 * @param media   media to search.
 */
  sdp_connection_t *eXosip_get_connection (sdp_message_t * sdp, const char *media);

/**
 * Get media information for call.
 * 
 * @param sdp     sdp information.
 * @param media   media to search.
 */
  sdp_media_t *eXosip_get_media (sdp_message_t * sdp, const char *media);

/** @} */

/**
 * @defgroup eXosip2_event eXosip2 event API
 * @ingroup eXosip2_setup
 * @{
 */

/**
 * Structure for event type description
 * @var eXosip_event_type
 */
  typedef enum eXosip_event_type
  {
    /* REGISTER related events */
    EXOSIP_REGISTRATION_NEW,           /**< announce new registration.       */
    EXOSIP_REGISTRATION_SUCCESS,       /**< user is successfully registred.  */
    EXOSIP_REGISTRATION_FAILURE,       /**< user is not registred.           */
    EXOSIP_REGISTRATION_REFRESHED,     /**< registration has been refreshed. */
    EXOSIP_REGISTRATION_TERMINATED,    /**< UA is not registred any more.    */

    /* INVITE related events within calls */
    EXOSIP_CALL_INVITE,            /**< announce a new call                   */
    EXOSIP_CALL_REINVITE,          /**< announce a new INVITE within call     */

    EXOSIP_CALL_NOANSWER,          /**< announce no answer within the timeout */
    EXOSIP_CALL_PROCEEDING,        /**< announce processing by a remote app   */
    EXOSIP_CALL_RINGING,           /**< announce ringback                     */
    EXOSIP_CALL_ANSWERED,          /**< announce start of call                */
    EXOSIP_CALL_REDIRECTED,        /**< announce a redirection                */
    EXOSIP_CALL_REQUESTFAILURE,    /**< announce a request failure            */
    EXOSIP_CALL_SERVERFAILURE,     /**< announce a server failure             */
    EXOSIP_CALL_GLOBALFAILURE,     /**< announce a global failure             */
    EXOSIP_CALL_ACK,               /**< ACK received for 200ok to INVITE      */

    EXOSIP_CALL_CANCELLED,         /**< announce that call has been cancelled */
    EXOSIP_CALL_TIMEOUT,           /**< announce that call has failed         */

    /* request related events within calls (except INVITE) */
    EXOSIP_CALL_MESSAGE_NEW,              /**< announce new incoming request. */
    EXOSIP_CALL_MESSAGE_PROCEEDING,       /**< announce a 1xx for request. */
    EXOSIP_CALL_MESSAGE_ANSWERED,         /**< announce a 200ok  */
    EXOSIP_CALL_MESSAGE_REDIRECTED,       /**< announce a failure. */
    EXOSIP_CALL_MESSAGE_REQUESTFAILURE,   /**< announce a failure. */
    EXOSIP_CALL_MESSAGE_SERVERFAILURE,    /**< announce a failure. */
    EXOSIP_CALL_MESSAGE_GLOBALFAILURE,    /**< announce a failure. */

    EXOSIP_CALL_CLOSED,            /**< a BYE was received for this call      */

    /* for both UAS & UAC events */
    EXOSIP_CALL_RELEASED,             /**< call context is cleared.            */

    /* response received for request outside calls */
    EXOSIP_MESSAGE_NEW,              /**< announce new incoming request. */
    EXOSIP_MESSAGE_PROCEEDING,       /**< announce a 1xx for request. */
    EXOSIP_MESSAGE_ANSWERED,         /**< announce a 200ok  */
    EXOSIP_MESSAGE_REDIRECTED,       /**< announce a failure. */
    EXOSIP_MESSAGE_REQUESTFAILURE,   /**< announce a failure. */
    EXOSIP_MESSAGE_SERVERFAILURE,    /**< announce a failure. */
    EXOSIP_MESSAGE_GLOBALFAILURE,    /**< announce a failure. */

    /* Presence and Instant Messaging */
    EXOSIP_SUBSCRIPTION_UPDATE,         /**< announce incoming SUBSCRIBE.      */
    EXOSIP_SUBSCRIPTION_CLOSED,         /**< announce end of subscription.     */

    EXOSIP_SUBSCRIPTION_NOANSWER,          /**< announce no answer              */
    EXOSIP_SUBSCRIPTION_PROCEEDING,        /**< announce a 1xx                  */
    EXOSIP_SUBSCRIPTION_ANSWERED,          /**< announce a 200ok                */
    EXOSIP_SUBSCRIPTION_REDIRECTED,        /**< announce a redirection          */
    EXOSIP_SUBSCRIPTION_REQUESTFAILURE,    /**< announce a request failure      */
    EXOSIP_SUBSCRIPTION_SERVERFAILURE,     /**< announce a server failure       */
    EXOSIP_SUBSCRIPTION_GLOBALFAILURE,     /**< announce a global failure       */
    EXOSIP_SUBSCRIPTION_NOTIFY,            /**< announce new NOTIFY request     */

    EXOSIP_SUBSCRIPTION_RELEASED,          /**< call context is cleared.        */

    EXOSIP_IN_SUBSCRIPTION_NEW,            /**< announce new incoming SUBSCRIBE.*/
    EXOSIP_IN_SUBSCRIPTION_RELEASED,       /**< announce end of subscription.   */

    EXOSIP_NOTIFICATION_NOANSWER,          /**< announce no answer              */
    EXOSIP_NOTIFICATION_PROCEEDING,        /**< announce a 1xx                  */
    EXOSIP_NOTIFICATION_ANSWERED,          /**< announce a 200ok                */
    EXOSIP_NOTIFICATION_REDIRECTED,        /**< announce a redirection          */
    EXOSIP_NOTIFICATION_REQUESTFAILURE,    /**< announce a request failure      */
    EXOSIP_NOTIFICATION_SERVERFAILURE,     /**< announce a server failure       */
    EXOSIP_NOTIFICATION_GLOBALFAILURE,     /**< announce a global failure       */

    EXOSIP_EVENT_COUNT                  /**< MAX number of events              */
  } eXosip_event_type_t;

/**
 * Structure for event description
 * @struct eXosip_event
 */
  struct eXosip_event
  {
    eXosip_event_type_t type;               /**< type of the event */
    char textinfo[256];                     /**< text description of event */
    void *external_reference;               /**< external reference (for calls) */

    osip_message_t *request;       /**< request within current transaction */
    osip_message_t *response;      /**< last response within current transaction */
    osip_message_t *ack;           /**< ack within current transaction */

    int tid; /**< unique id for transactions (to be used for answers) */
    int did; /**< unique id for SIP dialogs */

    int rid; /**< unique id for registration */
    int cid; /**< unique id for SIP calls (but multiple dialogs!) */
    int sid; /**< unique id for outgoing subscriptions */
    int nid; /**< unique id for incoming subscriptions */

    int ss_status;  /**< current Subscription-State for subscription */
    int ss_reason;  /**< current Reason status for subscription */
  };

/**
 * Free ressource in an eXosip event.
 * 
 * @param je    event to work on.
 */
  void eXosip_event_free (eXosip_event_t * je);

/**
 * Wait for an eXosip event.
 * 
 * @param tv_s      timeout value (seconds).
 * @param tv_ms     timeout value (mseconds).
 */
  eXosip_event_t *eXosip_event_wait (int tv_s, int tv_ms);


/**
 * Wait for next eXosip event.
 * 
 */
  eXosip_event_t *eXosip_event_get (void);

/**
 * This socket receive some data yhen an event happens internally.
 * NOTE: you must call eXosip_event_wait until there is no more events
 * in the fifo.
 * 
 */
  int eXosip_event_geteventsocket (void);


 /**
  * structure used to describe credentials for a client or server
  * consists of a certificate, a corresponding private key and its password
  * @struct eXosip_tls_credentials_s
  */
  typedef struct eXosip_tls_credentials_s
  {
    char priv_key[1024];
    char priv_key_pw[1024];
    char cert[1024];
  } eXosip_tls_credentials_t;

 /**
  * structure to describe the whole TLS-context for eXosip
  * consists of a certificate, a corresponding private key and its password
  * @struct eXosip_tls_ctx_s
  */
  typedef struct eXosip_tls_ctx_s
  {
    char random_file[1024];                                             /**< absolute path to a file with random(!) data */
    char dh_param[1024];                                                /**< absolute path to a file necessary for diffie hellman key exchange */
    char root_ca_cert[1024];                                            /**< absolute path to the file with known rootCAs */
    eXosip_tls_credentials_t client;            /**< credential of the client */
    eXosip_tls_credentials_t server;            /**< credential of the server */
  } eXosip_tls_ctx_t;

 /**
  * An enumeration which describes the error which can occur while setting the eXosip_tls_ctx
  */
  typedef enum
  {
    TLS_OK = 0,                                                         /**< yippieh, everything is fine :) */
    TLS_ERR_NO_RAND = -1,                               /**< no absolute path to the random file was specified */
    TLS_ERR_NO_DH_PARAM = -2,                           /**< no absolute path to the diifie hellman file was specified */
    TLS_ERR_NO_PW = -3,                                         /**< no password was specified */
    TLS_ERR_NO_ROOT_CA = -4,                            /**< no absolute path to the rootCA file was specified */
    TLS_ERR_MISSING_AUTH_PART = -5              /**< something is missing: the private key or the certificate */
  } eXosip_tls_ctx_error;

 /**
  *	sets the parameters for the TLS context, which is used for encrypted connections
  *	
  *	@param ctx, IN
  *		a struct which holds the necessary parameters
  *
  *	@return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_set_tls_ctx (eXosip_tls_ctx_t * ctx);

/**
  *	Select by CN name the server certificate from OS store.
  *
  *	12/11/2009 -> implemented only for "Windows Certificate Store"
  *
  *	@param local_certificate_cn  CN name of the certificate to send on incoming connection
  *
  *	@return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_tls_use_server_certificate (const char *local_certificate_cn);

/**
  *	Configure to accept/reject self signed and expired certificates.
  * NOTE: default is to accept (0)
  *
  *	@param _tls_verify_client_certificate  ">0": refuse self signed and expired certificates
  *
  *	@return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_tls_verify_certificate (int _tls_verify_client_certificate);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
