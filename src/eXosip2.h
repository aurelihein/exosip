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


#ifndef __EXOSIP2_H__
#define __EXOSIP2_H__

#if defined (HAVE_CONFIG_H)
#include <exosip-config.h>
#endif

#if defined(__PALMOS__) && (__PALMOS__ >= 0x06000000)
#define HAVE_CTYPE_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_STDARG_H 1

#elif defined(__VXWORKS_OS__) || defined(__rtems__)
#define HAVE_STRING_H 1
#define HAVE_TIME_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDARG_H 1

#elif defined _WIN32_WCE

#define HAVE_CTYPE_H 1
#define HAVE_STRING_H 1
#define HAVE_TIME_H 1
#define HAVE_STDARG_H 1

#define snprintf  _snprintf

#elif defined(WIN32)

#define HAVE_CTYPE_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_STDARG_H 1
#define HAVE_SYS_STAT_H

#define snprintf _snprintf

/* use win32 crypto routines for random number generation */
/* only use for vs .net (compiler v. 1300) or greater */
#if _MSC_VER >= 1300
#define WIN32_USE_CRYPTO 1
#endif

#endif

#if defined (HAVE_STRING_H)
#include <string.h>
#elif defined (HAVE_STRINGS_H)
#include <strings.h>
#else
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined (HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif

#ifdef HAVE_TIME_H
#  include <time.h>
#endif

#if defined (HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#if defined(__arc__)
#include "includes_api.h"
#include "os_cfg_pub.h"
#include <posix_time_pub.h>
#define USE_GETHOSTBYNAME
#endif

#ifdef __PSOS__
#define VA_START(a, f)  va_start(a, f)
#include "pna.h"
#include "stdlib.h"
#include "time.h"
#define timercmp(tvp, uvp, cmp) \
((tvp)->tv_sec cmp (uvp)->tv_sec || \
(tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define snprintf  osip_snprintf
#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif
#endif

#ifdef _WIN32_WCE
#include <winsock2.h>
#include <osipparser2/osip_port.h>
#include <ws2tcpip.h>
#define close(s) closesocket(s)
#elif WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(s) closesocket(s)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <osip2/osip.h>
#include <osip2/osip_dialog.h>

#include <eXosip2/eXosip.h>
#include "eXtransport.h"

#include "jpipe.h"

#define EXOSIP_VERSION	"4.0.0"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_GETHOSTBYNAME)

#define NI_MAXHOST      1025
#define NI_MAXSERV      32
#define NI_NUMERICHOST  1

#define PF_INET6        AF_INET6

	struct sockaddr_storage {
		unsigned char sa_len;
		unsigned char sa_family;	/* Address family AF_XXX */
		char sa_data[14];		/* Protocol specific address */
	};

	struct addrinfo {
		int ai_flags;			/* Input flags.  */
		int ai_family;			/* Protocol family for socket.  */
		int ai_socktype;		/* Socket type.  */
		int ai_protocol;		/* Protocol for socket.  */
		socklen_t ai_addrlen;	/* Length of socket address.  */
		struct sockaddr *ai_addr;	/* Socket address for socket.  */
		char *ai_canonname;		/* Canonical name for service location.  */
		struct addrinfo *ai_next;	/* Pointer to next in list.  */
	};

	void _eXosip_freeaddrinfo(struct addrinfo *ai);

#else

#define _eXosip_freeaddrinfo freeaddrinfo

#endif

	void _eXosip_update(struct eXosip_t *excontext);
#ifndef OSIP_MONOTHREAD
	void _eXosip_wakeup(struct eXosip_t *excontext);
#else
#define _eXosip_wakeup(A)  ;
#endif

#ifndef DEFINE_SOCKADDR_STORAGE
#define __eXosip_sockaddr sockaddr_storage
#else
	struct __eXosip_sockaddr {
		u_char ss_len;
		u_char ss_family;
		u_char padding[128 - 2];
	};
#endif

	typedef struct eXosip_dialog_t eXosip_dialog_t;

	struct eXosip_dialog_t {

		int d_id;
		osip_dialog_t *d_dialog;	/* active dialog */

		time_t d_session_timer_start;	/* session-timer helper */
		int d_session_timer_length;
		int d_refresher;

		time_t d_timer;
		int d_count;
		osip_message_t *d_200Ok;
		osip_message_t *d_ack;

		osip_list_t *d_inc_trs;
		osip_list_t *d_out_trs;
		int d_retry;			/* avoid too many unsuccessfull retry */
		int d_mincseq;			/* remember cseq after PRACK and UPDATE during setup */

		eXosip_dialog_t *next;
		eXosip_dialog_t *parent;
	};

	typedef struct eXosip_call_t eXosip_call_t;

	struct eXosip_call_t {

		int c_id;
		eXosip_dialog_t *c_dialogs;
		osip_transaction_t *c_inc_tr;
		osip_transaction_t *c_out_tr;
		int c_retry;			/* avoid too many unsuccessfull retry */
		void *external_reference;

		time_t expire_time;

		eXosip_call_t *next;
		eXosip_call_t *parent;
	};


	typedef struct eXosip_reg_t eXosip_reg_t;

	struct eXosip_reg_t {

		int r_id;

		int r_reg_period;		/* delay between registration */
		char *r_aor;			/* sip identity */
		char *r_registrar;		/* registrar */
		char *r_contact;		/* list of contacts string */

		char r_line[16];		/* line identifier */
		char r_qvalue[16];		/* the q value used for routing */
		
		osip_transaction_t *r_last_tr;
		int r_retry;			/* avoid too many unsuccessfull retry */

		struct __eXosip_sockaddr addr;
		int len;

		eXosip_reg_t *next;
		eXosip_reg_t *parent;
	};


#ifndef MINISIZE

	typedef struct eXosip_subscribe_t eXosip_subscribe_t;
	
	struct eXosip_subscribe_t {
		
		int s_id;
		int s_ss_status;
		int s_ss_reason;
		int s_reg_period;
		eXosip_dialog_t *s_dialogs;
		
		int s_retry;			/* avoid too many unsuccessfull retry */
		osip_transaction_t *s_inc_tr;
		osip_transaction_t *s_out_tr;
		
		eXosip_subscribe_t *next;
		eXosip_subscribe_t *parent;
	};
	
	typedef struct eXosip_notify_t eXosip_notify_t;
	
	struct eXosip_notify_t {
		
		int n_id;
		int n_online_status;
		
		int n_ss_status;
		int n_ss_reason;
		time_t n_ss_expires;
		eXosip_dialog_t *n_dialogs;
		
		osip_transaction_t *n_inc_tr;
		osip_transaction_t *n_out_tr;
		
		eXosip_notify_t *next;
		eXosip_notify_t *parent;
	};

	typedef struct eXosip_pub_t eXosip_pub_t;

	struct eXosip_pub_t {
		int p_id;

		int p_period;			/* delay between registration */
		char p_aor[256];		/* sip identity */
		char p_sip_etag[64];	/* sip_etag from 200ok */

		osip_transaction_t *p_last_tr;
		int p_retry;
		eXosip_pub_t *next;
		eXosip_pub_t *parent;
	};

	int _eXosip_pub_update(struct eXosip_t *excontext, eXosip_pub_t ** pub, osip_transaction_t * tr,
						   osip_message_t * answer);
	int _eXosip_pub_find_by_aor(struct eXosip_t *excontext, eXosip_pub_t ** pub, const char *aor);
	int _eXosip_pub_find_by_tid(struct eXosip_t *excontext, eXosip_pub_t ** pjp, int tid);
	int _eXosip_pub_init(eXosip_pub_t ** pub, const char *aor, const char *exp);
	void _eXosip_pub_free(struct eXosip_t *excontext, eXosip_pub_t * pub);

#endif

	typedef struct jauthinfo_t jauthinfo_t;

	struct jauthinfo_t {
		char username[50];
		char userid[50];
		char passwd[50];
		char ha1[50];
		char realm[50];
		jauthinfo_t *parent;
		jauthinfo_t *next;
	};

	int
	 _eXosip_create_authorization_header(osip_www_authenticate_t * wa,
										  const char *rquri,
										  const char *username,
										  const char *passwd,
										  const char *ha1,
										  osip_authorization_t ** auth,
										  const char *method,
										  const char *pszCNonce, int iNonceCount);
	int _eXosip_create_proxy_authorization_header(osip_proxy_authenticate_t * wa,
												   const char *rquri,
												   const char *username,
												   const char *passwd,
												   const char *ha1,
												   osip_proxy_authorization_t
												   ** auth, const char *method,
												   const char *pszCNonce,
												   int iNonceCount);
	int _eXosip_store_nonce(struct eXosip_t *excontext, const char *call_id, osip_proxy_authenticate_t * wa,
							int answer_code);
	int _eXosip_delete_nonce(struct eXosip_t *excontext, const char *call_id);

	eXosip_event_t *_eXosip_event_init_for_call(int type, eXosip_call_t * jc,
											   eXosip_dialog_t * jd,
											   osip_transaction_t * tr);

#ifndef MINISIZE
	eXosip_event_t *_eXosip_event_init_for_subscribe(int type,
													eXosip_subscribe_t * js,
													eXosip_dialog_t * jd,
													osip_transaction_t * tr);
	eXosip_event_t *_eXosip_event_init_for_notify(int type,
												 eXosip_notify_t * jn,
												 eXosip_dialog_t * jd,
												 osip_transaction_t * tr);
#endif

	eXosip_event_t *_eXosip_event_init_for_reg(int type, eXosip_reg_t * jr,
											  osip_transaction_t * tr);
	eXosip_event_t *_eXosip_event_init_for_message(int type,
												  osip_transaction_t * tr);

	int _eXosip_event_init(eXosip_event_t ** je, int type);
	void _eXosip_report_call_event(struct eXosip_t *excontext, int evt, eXosip_call_t * jc, eXosip_dialog_t * jd,
						   osip_transaction_t * tr);
	void _eXosip_report_event(struct eXosip_t *excontext, eXosip_event_t * je, osip_message_t * sip);
	int _eXosip_event_add(struct eXosip_t *excontext, eXosip_event_t * je);

	typedef void (*eXosip_callback_t) (int type, eXosip_event_t *);

	char *_eXosip_strdup_printf(const char *fmt, ...);

	char *_eXosip_transport_protocol(osip_message_t * msg);
	int _eXosip_find_protocol(osip_message_t * msg);
	int _eXosip_tcp_find_socket(char *host, int port);
	int _eXosip_tcp_connect_socket(char *host, int port);
	int setsockopt_ipv6only(int sock);


#ifndef MAX_EXOSIP_DNS_ENTRY
#define MAX_EXOSIP_DNS_ENTRY 10
#endif

#ifndef MAX_EXOSIP_ACCOUNT_INFO
#define MAX_EXOSIP_ACCOUNT_INFO 10
#endif

#ifndef MAX_EXOSIP_HTTP_AUTH
#define MAX_EXOSIP_HTTP_AUTH 100
#endif

	typedef struct eXosip_t eXosip_t;

	struct eXosip_t {
		struct eXtl_protocol *eXtl;
		void *eXtludp_reserved;
		void *eXtltcp_reserved;
#ifndef DISABLE_TLS
		void *eXtltls_reserved;
		void *eXtldtls_reserved;
#endif
		char transport[10];
		char *user_agent;

		eXosip_reg_t *j_reg;	/* my registrations */
		eXosip_call_t *j_calls;	/* my calls        */
#ifndef MINISIZE
		eXosip_subscribe_t *j_subscribes;	/* my friends      */
		eXosip_notify_t *j_notifies;	/* my susbscribers */
		eXosip_pub_t *j_pub;	/* my publications  */
#endif
		osip_list_t j_transactions;

		osip_t *j_osip;
		int j_stop_ua;
#ifndef OSIP_MONOTHREAD
		void *j_cond;
		void *j_mutexlock;
		void *j_thread;
		jpipe_t *j_socketctl;
		jpipe_t *j_socketctl_event;
#endif

		osip_fifo_t *j_events;

		jauthinfo_t *authinfos;

		int keep_alive;
		int keep_alive_options;
		int learn_port;
		int use_rport;
		int dns_capabilities;
		int dscp;
		char ipv4_for_gateway[256];
		char ipv6_for_gateway[256];
		struct eXosip_dns_cache dns_entries[MAX_EXOSIP_DNS_ENTRY];
		struct eXosip_account_info account_entries[MAX_EXOSIP_ACCOUNT_INFO];
		struct eXosip_http_auth http_auths[MAX_EXOSIP_HTTP_AUTH];

		CbSipCallback cbsipCallback;
	};

	int _eXosip_guess_ip_for_via(struct eXosip_t *excontext, int family, char *address, int size);

	int _eXosip_get_addrinfo(struct eXosip_t *excontext, struct addrinfo **addrinfo, const char *hostname,
							int service, int protocol);

	int _eXosip_set_callbacks(osip_t * osip);
	int _eXosip_snd_message(struct eXosip_t *excontext, osip_transaction_t * tr, osip_message_t * sip, char *host,
			   int port, int out_socket);
	char *_eXosip_malloc_new_random(void);
	void _eXosip_delete_reserved(osip_transaction_t * transaction);

	int _eXosip_dialog_init_as_uac(eXosip_dialog_t ** jd, osip_message_t * _200Ok);
	int _eXosip_dialog_init_as_uas(eXosip_dialog_t ** jd,
								  osip_message_t * _invite,
								  osip_message_t * _200Ok);
	void _eXosip_dialog_free(struct eXosip_t *excontext,eXosip_dialog_t * jd);

	int _eXosip_generating_request_out_of_dialog(struct eXosip_t *excontext, osip_message_t ** dest,
										 const char *method, const char *to,
										 const char *transport,
										 const char *from, const char *proxy);
	int _eXosip_generating_publish(struct eXosip_t *excontext, osip_message_t ** message, const char *to,
						   const char *from, const char *route);
	int _eXosip_generating_cancel(struct eXosip_t *excontext, osip_message_t ** dest,
						  osip_message_t * request_cancelled);
	int _eXosip_generating_bye(struct eXosip_t *excontext, osip_message_t ** bye, osip_dialog_t * dialog,
					   char *transport);

	int _eXosip_update_top_via(osip_message_t * sip);
	int _eXosip_request_add_via(struct eXosip_t *excontext, osip_message_t * request, const char *transport,
								const char *locip);

	void _eXosip_mark_all_registrations_expired (struct eXosip_t *excontext);

	int _eXosip_add_authentication_information(struct eXosip_t *excontext, osip_message_t * req,
											  osip_message_t * last_response);
	int _eXosip_reg_find(struct eXosip_t *excontext, eXosip_reg_t ** reg, osip_transaction_t * tr);
	int _eXosip_reg_find_id(struct eXosip_t *excontext, eXosip_reg_t ** reg, int rid);
	int _eXosip_reg_init(struct eXosip_t *excontext, eXosip_reg_t ** jr, const char *from,
						const char *proxy, const char *contact);
	void _eXosip_reg_free(struct eXosip_t *excontext, eXosip_reg_t * jreg);
	int _eXosip_generating_register(struct eXosip_t *excontext, eXosip_reg_t * jreg, osip_message_t ** reg,
							char *transport, char *from, char *proxy,
							char *contact, int expires);

	int _eXosip_call_transaction_find(struct eXosip_t *excontext, int tid, eXosip_call_t ** jc,
									  eXosip_dialog_t ** jd,
									  osip_transaction_t ** tr);
	int _eXosip_call_retry_request(struct eXosip_t *excontext, eXosip_call_t * jc,
								   eXosip_dialog_t * jd,
								   osip_transaction_t * out_tr);
	int _eXosip_transaction_find(struct eXosip_t *excontext, int tid, osip_transaction_t ** transaction);
	int _eXosip_call_dialog_find(struct eXosip_t *excontext, int jid, eXosip_call_t ** jc,
								eXosip_dialog_t ** jd);
	int _eXosip_call_find(struct eXosip_t *excontext, int cid, eXosip_call_t ** jc);
	int _eXosip_dialog_set_200ok(eXosip_dialog_t * _jd, osip_message_t * _200Ok);

	int _eXosip_answer_invite_123456xx(struct eXosip_t *excontext, eXosip_call_t * jc, eXosip_dialog_t * jd,
									   int code, osip_message_t ** answer,
									   int send);

	int _eXosip_build_response_default(struct eXosip_t *excontext, osip_message_t ** dest,
									   osip_dialog_t * dialog, int status,
									   osip_message_t * request);
	int _eXosip_complete_answer_that_establish_a_dialog(struct eXosip_t *excontext, osip_message_t * response,
												osip_message_t * request);
	int _eXosip_build_request_within_dialog(struct eXosip_t *excontext, osip_message_t ** dest,
											const char *method,
											osip_dialog_t * dialog,
											const char *transport);
	void _eXosip_kill_transaction(osip_list_t * transactions);
	int _eXosip_remove_transaction_from_call(osip_transaction_t * tr,
											eXosip_call_t * jc);

	osip_transaction_t *_eXosip_find_last_transaction(eXosip_call_t * jc,
													 eXosip_dialog_t * jd,
													 const char *method);
	osip_transaction_t *_eXosip_find_last_inc_transaction(eXosip_call_t * jc,
														 eXosip_dialog_t * jd,
														 const char *method);
	osip_transaction_t *_eXosip_find_last_out_transaction(eXosip_call_t * jc,
														 eXosip_dialog_t * jd,
														 const char *method);
	osip_transaction_t *_eXosip_find_last_invite(eXosip_call_t * jc,
												eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_last_inc_invite(eXosip_call_t * jc,
													eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_last_out_invite(eXosip_call_t * jc,
													eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_previous_invite(eXosip_call_t * jc,
													eXosip_dialog_t * jd,
													osip_transaction_t *
													last_invite);


	int _eXosip_call_init(eXosip_call_t ** jc);
	void _eXosip_call_renew_expire_time(eXosip_call_t * jc);
	void _eXosip_call_free(struct eXosip_t *excontext, eXosip_call_t * jc);
	void _eXosip_call_remove_dialog_reference_in_call(eXosip_call_t * jc,
													   eXosip_dialog_t * jd);
	int _eXosip_read_message(struct eXosip_t *excontext, int max_message_nb, int sec_max, int usec_max);
	void _eXosip_release_terminated_calls(struct eXosip_t *excontext);
	void _eXosip_release_terminated_registrations(struct eXosip_t *excontext);
	void _eXosip_release_terminated_publications(struct eXosip_t *excontext);

#ifndef MINISIZE
	int _eXosip_insubscription_transaction_find(struct eXosip_t *excontext, 
																							int tid,
																							eXosip_notify_t ** jn,
																							eXosip_dialog_t ** jd,
																							osip_transaction_t ** tr);
	int _eXosip_notify_dialog_find(struct eXosip_t *excontext, int nid, eXosip_notify_t ** jn,
																 eXosip_dialog_t ** jd);
	int _eXosip_subscribe_transaction_find(struct eXosip_t *excontext, int tid, eXosip_subscribe_t ** js,
																				 eXosip_dialog_t ** jd,
																				 osip_transaction_t ** tr);
	int _eXosip_subscribe_dialog_find(struct eXosip_t *excontext, int nid, eXosip_subscribe_t ** js,
																		eXosip_dialog_t ** jd);
	int _eXosip_insubscription_answer_1xx(struct eXosip_t *excontext, eXosip_notify_t * jc,
																				eXosip_dialog_t * jd, int code);
	int _eXosip_insubscription_answer_2xx(eXosip_notify_t * jn,
																				eXosip_dialog_t * jd, int code);
	int _eXosip_insubscription_answer_3456xx(struct eXosip_t *excontext, eXosip_notify_t * jn,
																					 eXosip_dialog_t * jd, int code);
	osip_transaction_t *_eXosip_find_last_inc_notify(eXosip_subscribe_t * jn,
																									 eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_last_out_notify(eXosip_notify_t * jn,
																									 eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_last_inc_subscribe(eXosip_notify_t * jn,
																											eXosip_dialog_t * jd);
	osip_transaction_t *_eXosip_find_last_out_subscribe(eXosip_subscribe_t * js,
																											eXosip_dialog_t * jd);
	void _eXosip_release_terminated_subscriptions(struct eXosip_t *excontext);
	void _eXosip_release_terminated_in_subscriptions(struct eXosip_t *excontext);
	int _eXosip_subscribe_init(eXosip_subscribe_t ** js);
	void _eXosip_subscribe_free(struct eXosip_t *excontext, eXosip_subscribe_t * js);
	int _eXosip_subscribe_set_refresh_interval(eXosip_subscribe_t * js,
											   osip_message_t * inc_subscribe);
	int _eXosip_subscribe_need_refresh(eXosip_subscribe_t * js,
									  eXosip_dialog_t * jd, int now);
	int _eXosip_subscribe_send_request_with_credential(struct eXosip_t *excontext, eXosip_subscribe_t * js,
													   eXosip_dialog_t * jd,
													   osip_transaction_t *
													   out_tr);
	int _eXosip_subscribe_automatic_refresh(struct eXosip_t *excontext, eXosip_subscribe_t * js,
											eXosip_dialog_t * jd,
											osip_transaction_t * out_tr);
	int _eXosip_notify_init(eXosip_notify_t ** jn, osip_message_t * inc_subscribe);
	void _eXosip_notify_free(struct eXosip_t *excontext, eXosip_notify_t * jn);
	int _eXosip_notify_set_contact_info(eXosip_notify_t * jn, char *uri);
	int _eXosip_notify_set_refresh_interval(eXosip_notify_t * jn,
											osip_message_t * inc_subscribe);
	void _eXosip_notify_add_expires_in_2XX_for_subscribe(eXosip_notify_t * jn,
														 osip_message_t * answer);
	int _eXosip_insubscription_send_request_with_credential(struct eXosip_t *excontext, 
															eXosip_notify_t *
															jn,
															eXosip_dialog_t *
															jd,
															osip_transaction_t
															* out_tr);
#endif

	int _eXosip_is_public_address(const char *addr);

	void _eXosip_retransmit_lost200ok(struct eXosip_t *excontext);
	int _eXosip_dialog_add_contact(struct eXosip_t *excontext, osip_message_t * request,
								   osip_message_t * answer);

	int _eXosip_transaction_init(struct eXosip_t *excontext, osip_transaction_t ** transaction,
								 osip_fsm_type_t ctx_type, osip_t * osip,
								 osip_message_t * message);

	int _eXosip_srv_lookup(struct eXosip_t *excontext, osip_message_t * sip, osip_naptr_t **naptr_record);

	void _eXosip_dnsutils_release(osip_naptr_t *naptr_record);

	int _eXosip_handle_incoming_message(struct eXosip_t *excontext, char *buf, size_t len, int socket,
										char *host, int port);

#ifdef __cplusplus
}
#endif
#endif
