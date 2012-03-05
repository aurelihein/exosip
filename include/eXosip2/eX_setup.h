/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard_at_osip.org
  
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

#ifndef __EX_SETUP_H__
#define __EX_SETUP_H__

#include <eXosip2/eXosip.h>
#include <osipparser2/osip_message.h>

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct eXosip_t;
  struct osip_srv_record;
  struct osip_naptr;

/**
 * @file eX_setup.h
 * @brief eXosip setup API
 *
 * This file provide the API needed to setup and configure
 * the SIP endpoint.
 *
 */

/**
 * @defgroup eXosip2_conf eXosip2 configuration API
 * @ingroup eXosip2_setup
 * @{
 */

/**
 * Allocate an eXosip context.
 * 
 */
  struct eXosip_t *eXosip_malloc(void);

/**
 * Initiate the eXtented oSIP library.
 * 
 */
  int eXosip_init (struct eXosip_t *excontext);

/**
 * Release ressource used by the eXtented oSIP library.
 * 
 */
  void eXosip_quit (struct eXosip_t *excontext);

/**
 * Lock the eXtented oSIP library.
 * 
 */
  int eXosip_lock (struct eXosip_t *excontext);

/**
 * UnLock the eXtented oSIP library.
 * 
 */
  int eXosip_unlock (struct eXosip_t *excontext);

/**
 * Process (non-threaded mode ONLY) eXosip events.
 * 
 */
  int eXosip_execute (struct eXosip_t *excontext);

#define EXOSIP_OPT_BASE_OPTION 0
#define EXOSIP_OPT_UDP_KEEP_ALIVE (EXOSIP_OPT_BASE_OPTION+1)
#define EXOSIP_OPT_UDP_LEARN_PORT (EXOSIP_OPT_BASE_OPTION+2)
#define EXOSIP_OPT_USE_RPORT (EXOSIP_OPT_BASE_OPTION+7)
#define EXOSIP_OPT_SET_IPV4_FOR_GATEWAY (EXOSIP_OPT_BASE_OPTION+8)
#define EXOSIP_OPT_ADD_DNS_CACHE (EXOSIP_OPT_BASE_OPTION+9)
#define EXOSIP_OPT_DELETE_DNS_CACHE (EXOSIP_OPT_BASE_OPTION+10)
#define EXOSIP_OPT_SET_IPV6_FOR_GATEWAY (EXOSIP_OPT_BASE_OPTION+12)
#define EXOSIP_OPT_ADD_ACCOUNT_INFO (EXOSIP_OPT_BASE_OPTION+13)
#define EXOSIP_OPT_DNS_CAPABILITIES (EXOSIP_OPT_BASE_OPTION+14)
#define EXOSIP_OPT_SET_DSCP (EXOSIP_OPT_BASE_OPTION+15)

  /* non standard option: need a compilation flag to activate */
#define EXOSIP_OPT_KEEP_ALIVE_OPTIONS_METHOD (EXOSIP_OPT_BASE_OPTION+1000)

  struct eXosip_dns_cache
  {
    char host[1024];
    char ip[256];
  };

  struct eXosip_account_info
  {
    char proxy[1024];
    char nat_ip[256];
    int nat_port;
  };

  struct eXosip_http_auth
  {
    char pszCallId[64];
    osip_proxy_authenticate_t *wa;
    char pszCNonce[64];
    int iNonceCount;
    int answer_code;
  };

/**
 * Set eXosip options.
 * See eXosip_option for available options.
 *
 * @param opt     option to configure.
 * @param value   value for options.
 * 
 */
  int eXosip_set_option (struct eXosip_t *excontext, int opt, const void *value);


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
  eXosip_tls_ctx_error eXosip_set_tls_ctx (struct eXosip_t *excontext, eXosip_tls_ctx_t * ctx);

/**
  *	Select by CN name the server certificate from OS store.
  *
  *	12/11/2009 -> implemented only for "Windows Certificate Store"
  *
  *	@param local_certificate_cn  CN name of the certificate to send on incoming connection
  *
  *	@return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_tls_use_server_certificate (struct eXosip_t *excontext, const char *local_certificate_cn);

/**
  *         Select by CN name the client certificate from OS store.
  *
  *         31/1/2011 -> implemented only for "Windows Certificate Store"
  *
  *         @param local_certificate_cn  CN name of the certificate to send on outgoing connection
  *
  *         @return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_tls_use_client_certificate (struct eXosip_t *excontext, const char *local_certificate_cn);

/**
  *	Configure to accept/reject self signed and expired certificates.
  * NOTE: default is to accept (0)
  *
  *	@param _tls_verify_client_certificate  ">0": refuse self signed and expired certificates
  *
  *	@return  the eXosip_tls_ctx_error code
  */
  eXosip_tls_ctx_error eXosip_tls_verify_certificate (struct eXosip_t *excontext, int _tls_verify_client_certificate);

/**
 * Start and return osip_naptr context.
 * Note that DNS results might not yet be available.
 * 
 * @param domain         domain name for NAPTR record
 * @param protocol       protocol to use ("SIP")
 * @param transport      transport to use ("UDP")
 * @param keep_in_cache  keep result in cache if >0
 */
struct osip_naptr *eXosip_dnsutils_naptr(struct eXosip_t *excontext, const char *domain, const char *protocol,
                                         const char *transport, int keep_in_cache);

/**
 * Continue to process asynchronous DNS request (if implemented).
 * 
 * @param output_record  result structure.
 * @param force          force waiting for final answer if >0
 */
int eXosip_dnsutils_dns_process(struct osip_naptr *output_record, int force);

/**
 * Rotate first SRV entry to last SRV entry.
 * 
 * @param output_record  result structure.
 */
  int eXosip_dnsutils_rotate_srv(struct osip_srv_record *output_record);

/**
 * Listen on a specified socket.
 * 
 * @param transport IPPROTO_UDP for udp. (soon to come: TCP/TLS?)
 * @param addr      the address to bind (NULL for all interface)
 * @param port      the listening port. (0 for random port)
 * @param family    the IP family (AF_INET or AF_INET6).
 * @param secure    0 for UDP or TCP, 1 for TLS (with TCP).
 */
  int eXosip_listen_addr (struct eXosip_t *excontext, int transport, const char *addr, int port, int family,
                          int secure);

/**
 * Listen on a specified socket.
 * 
 * @param transport IPPROTO_UDP for udp. (soon to come: TCP/TLS?)
 * @param socket socket to use for listening to UDP sip messages.
 * @param port the listening port for masquerading.
 */
  int eXosip_set_socket (struct eXosip_t *excontext, int transport, int socket, int port);

/**
 * Set the SIP User-Agent: header string.
 *
 * @param user_agent the User-Agent header to insert in messages.
 */
  void eXosip_set_user_agent (struct eXosip_t *excontext, const char *user_agent);

 /**
  * Get the eXosip version as a sring
  *
  */
  const char *eXosip_get_version (void);

  typedef void (*CbSipCallback) (osip_message_t * msg, int received);

/**
 * Set a callback to get sent and received SIP messages.
 *
 * @param cbsipCallback the callback to retreive messages.
 */
  int eXosip_set_cbsip_message (struct eXosip_t *excontext, CbSipCallback cbsipCallback);

/**
 * Use IPv6 instead of IPv4.
 * 
 * @param ipv6_enable  This paramter should be set to 1 to enable IPv6 mode.
 */
  void eXosip_enable_ipv6 (int ipv6_enable);

/**
 * This method is used to replace contact address with
 * the public address of your NAT. The ip address should
 * be retreived manually (fixed IP address) or with STUN.
 * This address will only be used when the remote
 * correspondant appears to be on an DIFFERENT LAN.
 *
 * @param public_address 	the ip address.
 * @param port          	the port for masquerading.
 * 
 * If set to NULL, then the local ip address will be guessed 
 * automatically (returns to default mode).
 */
  void eXosip_masquerade_contact (struct eXosip_t *excontext, const char *public_address, int port);

/**
 * This method is used to find out an free IPPROTO_UDP or IPPROTO_TCP port.
 *
 * @param free_port          	initial port for search.
 * @param transport          	IPPROTO_UDP or IPPROTO_TCP protocol.
 * 
 */
  int eXosip_find_free_port (struct eXosip_t *excontext, int free_port, int transport);

#ifndef DOXYGEN

/**
 * Wake Up the eXosip_event_wait method.
 * 
 */
#ifndef OSIP_MONOTHREAD
  void eXosip_wakeup_event (struct eXosip_t *excontext);
#else
#define eXosip_wakeup_event(A)   ;
#endif

#endif

/** @} */


/**
 * @defgroup eXosip2_network eXosip2 network API
 * @ingroup eXosip2_setup
 * @{
 */

/**
 * Modify the transport protocol used to send SIP message.
 * 
 * @param msg         The SIP message to modify
 * @param transport   transport protocol to use ("UDP", "TCP" or "TLS")
 */
  int eXosip_transport_set (osip_message_t * msg, const char *transport);

/**
 * Find the current localip (interface with default route).
 * 
 * @param family    AF_INET or AF_INET6
 * @param address   a string containing the local IP address.
 * @param size      The size of the string
 */
  int eXosip_guess_localip (struct eXosip_t *excontext, int family, char *address, int size);

/** @} */


#ifdef __cplusplus
}
#endif

#endif
