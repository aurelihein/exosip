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

#include "eXosip2.h"
#include <eXosip2/eXosip.h>

#include <osip2/osip_mt.h>
#include <osip2/osip_condv.h>

#if defined (_WIN32_WCE)
#include "inet_ntop.h"
#elif WIN32
#include "inet_ntop.h"
#endif

extern eXosip_t eXosip;

int ipv6_enable = 0;

#ifdef OSIP_MT
static void *_eXosip_thread(void *arg);
#endif
static void _eXosip_keep_alive(void);

#ifndef MINISIZE

void eXosip_enable_ipv6(int _ipv6_enable)
{
	ipv6_enable = _ipv6_enable;
}

#endif

const char *eXosip_get_version(void)
{
	return EXOSIP_VERSION;
}

int eXosip_set_cbsip_message(CbSipCallback cbsipCallback)
{
	eXosip.cbsipCallback = cbsipCallback;
	return 0;
}

void eXosip_masquerade_contact(const char *public_address, int port)
{
	eXtl_udp.tl_masquerade_contact(public_address, port);
	eXtl_tcp.tl_masquerade_contact(public_address, port);
#ifdef HAVE_OPENSSL_SSL_H
	eXtl_tls.tl_masquerade_contact(public_address, port);
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
	eXtl_dtls.tl_masquerade_contact(public_address, port);
#endif
#endif
	return;
}

int eXosip_guess_localip(int family, char *address, int size)
{
	return eXosip_guess_ip_for_via(family, address, size);
}

int eXosip_is_public_address(const char *c_address)
{
	return (0 != strncmp(c_address, "192.168", 7)
			&& 0 != strncmp(c_address, "10.", 3)
			&& 0 != strncmp(c_address, "172.16.", 7)
			&& 0 != strncmp(c_address, "172.17.", 7)
			&& 0 != strncmp(c_address, "172.18.", 7)
			&& 0 != strncmp(c_address, "172.19.", 7)
			&& 0 != strncmp(c_address, "172.20.", 7)
			&& 0 != strncmp(c_address, "172.21.", 7)
			&& 0 != strncmp(c_address, "172.22.", 7)
			&& 0 != strncmp(c_address, "172.23.", 7)
			&& 0 != strncmp(c_address, "172.24.", 7)
			&& 0 != strncmp(c_address, "172.25.", 7)
			&& 0 != strncmp(c_address, "172.26.", 7)
			&& 0 != strncmp(c_address, "172.27.", 7)
			&& 0 != strncmp(c_address, "172.28.", 7)
			&& 0 != strncmp(c_address, "172.29.", 7)
			&& 0 != strncmp(c_address, "172.30.", 7)
			&& 0 != strncmp(c_address, "172.31.", 7)
			&& 0 != strncmp(c_address, "169.254", 7));
}

void eXosip_set_user_agent(const char *user_agent)
{
	osip_free(eXosip.user_agent);
	eXosip.user_agent = osip_strdup(user_agent);
}

void eXosip_kill_transaction(osip_list_t * transactions)
{
	osip_transaction_t *transaction;

	if (!osip_list_eol(transactions, 0)) {
		/* some transaction are still used by osip,
		   transaction should be released by modules! */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"module sfp: _osip_kill_transaction transaction should be released by modules!\n"));
	}

	while (!osip_list_eol(transactions, 0)) {
		transaction = osip_list_get(transactions, 0);

		__eXosip_delete_jinfo(transaction);
		_eXosip_dnsutils_release(transaction->naptr_record);
		transaction->naptr_record=NULL;
		osip_transaction_free(transaction);
	}
}

void eXosip_quit(void)
{
	jauthinfo_t *jauthinfo;
	eXosip_call_t *jc;
	eXosip_reg_t *jreg;
#ifndef MINISIZE
	eXosip_notify_t *jn;
	eXosip_subscribe_t *js;
	eXosip_pub_t *jpub;
#endif
#ifdef OSIP_MT
	int i;
#endif

	if (eXosip.j_stop_ua == -1) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"eXosip: already stopped!\n"));
		return;
	}

	eXosip.j_stop_ua = 1;		/* ask to quit the application */
	__eXosip_wakeup();
	__eXosip_wakeup_event();

#ifdef OSIP_MT
	if (eXosip.j_thread != NULL) {
		i = osip_thread_join((struct osip_thread *) eXosip.j_thread);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: can't terminate thread!\n"));
		}
		osip_free((struct osip_thread *) eXosip.j_thread);
	}

	jpipe_close(eXosip.j_socketctl);
	jpipe_close(eXosip.j_socketctl_event);
#endif

	osip_free(eXosip.user_agent);

	for (jc = eXosip.j_calls; jc != NULL; jc = eXosip.j_calls) {
		REMOVE_ELEMENT(eXosip.j_calls, jc);
		eXosip_call_free(jc);
	}

#ifndef MINISIZE
	for (js = eXosip.j_subscribes; js != NULL; js = eXosip.j_subscribes) {
		REMOVE_ELEMENT(eXosip.j_subscribes, js);
		eXosip_subscribe_free(js);
	}

	for (jn = eXosip.j_notifies; jn != NULL; jn = eXosip.j_notifies) {
		REMOVE_ELEMENT(eXosip.j_notifies, jn);
		eXosip_notify_free(jn);
	}
#endif

#ifdef OSIP_MT
	osip_mutex_destroy((struct osip_mutex *) eXosip.j_mutexlock);
#if !defined (_WIN32_WCE)
	osip_cond_destroy((struct osip_cond *) eXosip.j_cond);
#endif
#endif

	for (jreg = eXosip.j_reg; jreg != NULL; jreg = eXosip.j_reg) {
		REMOVE_ELEMENT(eXosip.j_reg, jreg);
		eXosip_reg_free(jreg);
	}

#ifndef MINISIZE
	for (jpub = eXosip.j_pub; jpub != NULL; jpub = eXosip.j_pub) {
		REMOVE_ELEMENT(eXosip.j_pub, jpub);
		_eXosip_pub_free(jpub);
	}
#endif

	while (!osip_list_eol(&eXosip.j_transactions, 0)) {
		osip_transaction_t *tr =
			(osip_transaction_t *) osip_list_get(&eXosip.j_transactions, 0);
		if (tr->state == IST_TERMINATED || tr->state == ICT_TERMINATED
			|| tr->state == NICT_TERMINATED || tr->state == NIST_TERMINATED) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
								  "Release a terminated transaction\n"));
			osip_list_remove(&eXosip.j_transactions, 0);
			__eXosip_delete_jinfo(tr);
			_eXosip_dnsutils_release(tr->naptr_record);
			tr->naptr_record=NULL;
			osip_transaction_free(tr);
		} else {
			osip_list_remove(&eXosip.j_transactions, 0);
			__eXosip_delete_jinfo(tr);
			_eXosip_dnsutils_release(tr->naptr_record);
			tr->naptr_record=NULL;
			osip_transaction_free(tr);
		}
	}

	eXosip_kill_transaction(&eXosip.j_osip->osip_ict_transactions);
	eXosip_kill_transaction(&eXosip.j_osip->osip_nict_transactions);
	eXosip_kill_transaction(&eXosip.j_osip->osip_ist_transactions);
	eXosip_kill_transaction(&eXosip.j_osip->osip_nist_transactions);
	osip_release(eXosip.j_osip);

	{
		eXosip_event_t *ev;

		for (ev = osip_fifo_tryget(eXosip.j_events); ev != NULL;
			 ev = osip_fifo_tryget(eXosip.j_events))
			eXosip_event_free(ev);
	}

	osip_fifo_free(eXosip.j_events);

	for (jauthinfo = eXosip.authinfos; jauthinfo != NULL;
		 jauthinfo = eXosip.authinfos) {
		REMOVE_ELEMENT(eXosip.authinfos, jauthinfo);
		osip_free(jauthinfo);
	}

	{
		struct eXosip_http_auth *http_auth;
		int pos;

		/* update entries with same call_id */
		for (pos = 0; pos < MAX_EXOSIP_HTTP_AUTH; pos++) {
			http_auth = &eXosip.http_auths[pos];
			if (http_auth->pszCallId[0] == '\0')
				continue;
			osip_proxy_authenticate_free(http_auth->wa);
			memset(http_auth, 0, sizeof(struct eXosip_http_auth));
		}
	}

	eXtl_udp.tl_free();
	eXtl_tcp.tl_free();
#ifdef HAVE_OPENSSL_SSL_H
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
	eXtl_dtls.tl_free();
#endif
	eXtl_tls.tl_free();
#endif

	memset(&eXosip, 0, sizeof(eXosip));
	eXosip.j_stop_ua = -1;
	return;
}

int eXosip_set_socket(int transport, int socket, int port)
{
	eXosip.eXtl = NULL;
	if (transport == IPPROTO_UDP) {
		eXtl_udp.proto_port = port;
		eXtl_udp.tl_set_socket(socket);
		eXosip.eXtl = &eXtl_udp;
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "UDP");
	} else if (transport == IPPROTO_TCP) {
		eXtl_tcp.proto_port = port;
		eXtl_tcp.tl_set_socket(socket);
		eXosip.eXtl = &eXtl_tcp;
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "TCP");
	} else
		return OSIP_BADPARAMETER;

#ifdef OSIP_MT
	eXosip.j_thread = (void *) osip_thread_create(20000, _eXosip_thread, NULL);
	if (eXosip.j_thread == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Cannot start thread!\n"));
		return OSIP_UNDEFINED_ERROR;
	}
#endif
	return OSIP_SUCCESS;
}

#ifdef IPV6_V6ONLY
int setsockopt_ipv6only(int sock)
{
	int on = 1;

	return setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &on, sizeof(on));
}
#endif							/* IPV6_V6ONLY */

#ifndef MINISIZE
int eXosip_find_free_port(int free_port, int transport)
{
	int res1;
	int res2;
	struct addrinfo *addrinfo_rtp = NULL;
	struct addrinfo *curinfo_rtp;
	struct addrinfo *addrinfo_rtcp = NULL;
	struct addrinfo *curinfo_rtcp;
	int sock;
	int count;

	for (count = 0; count < 8; count++) {
		if (ipv6_enable == 0)
			res1 = eXosip_get_addrinfo(&addrinfo_rtp, "0.0.0.0", free_port + count * 2, transport);
		else
			res1 = eXosip_get_addrinfo(&addrinfo_rtp, "::", free_port + count * 2, transport);
		if (res1 != 0)
			return res1;
		if (ipv6_enable == 0)
			res2 = eXosip_get_addrinfo(&addrinfo_rtcp, "0.0.0.0", free_port + count * 2 + 1, transport);
		else
			res2 = eXosip_get_addrinfo(&addrinfo_rtcp, "::", free_port + count * 2 + 1, transport);
		if (res2 != 0) {
			eXosip_freeaddrinfo(addrinfo_rtp);
			return res2;
		}

		sock = -1;
		for (curinfo_rtp = addrinfo_rtp; curinfo_rtp;
			 curinfo_rtp = curinfo_rtp->ai_next) {
			if (curinfo_rtp->ai_protocol && curinfo_rtp->ai_protocol != transport) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO3, NULL,
							"eXosip: Skipping protocol %d\n",
							curinfo_rtp->ai_protocol));
				continue;
			}

			sock = (int) socket(curinfo_rtp->ai_family, curinfo_rtp->ai_socktype,
								curinfo_rtp->ai_protocol);
			if (sock < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot create socket!\n"));
				continue;
			}

			if (curinfo_rtp->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
				if (setsockopt_ipv6only(sock)) {
					close(sock);
					sock = -1;
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"eXosip: Cannot set socket option!\n"));
					continue;
				}
#endif							/* IPV6_V6ONLY */
			}

			res1 = bind(sock, curinfo_rtp->ai_addr, curinfo_rtp->ai_addrlen);
			if (res1 < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"eXosip: Cannot bind socket node: 0.0.0.0 family:%d\n",
							curinfo_rtp->ai_family));
				close(sock);
				sock = -1;
				continue;
			}
			break;
		}

		eXosip_freeaddrinfo(addrinfo_rtp);

		if (sock == -1) {
			eXosip_freeaddrinfo(addrinfo_rtcp);
			continue;
		}

		close(sock);
		sock = -1;
		for (curinfo_rtcp = addrinfo_rtcp; curinfo_rtcp;
			 curinfo_rtcp = curinfo_rtcp->ai_next) {
			if (curinfo_rtcp->ai_protocol
				&& curinfo_rtcp->ai_protocol != transport) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO3, NULL,
							"eXosip: Skipping protocol %d\n",
							curinfo_rtcp->ai_protocol));
				continue;
			}

			sock = (int) socket(curinfo_rtcp->ai_family, curinfo_rtcp->ai_socktype,
								curinfo_rtcp->ai_protocol);
			if (sock < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot create socket!\n"));
				continue;
			}

			if (curinfo_rtcp->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
				if (setsockopt_ipv6only(sock)) {
					close(sock);
					sock = -1;
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"eXosip: Cannot set socket option!\n"));
					continue;
				}
#endif							/* IPV6_V6ONLY */
			}

			res1 = bind(sock, curinfo_rtcp->ai_addr, curinfo_rtcp->ai_addrlen);
			if (res1 < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"eXosip: Cannot bind socket node: 0.0.0.0 family:%d\n",
							curinfo_rtp->ai_family));
				close(sock);
				sock = -1;
				continue;
			}
			break;
		}

		eXosip_freeaddrinfo(addrinfo_rtcp);

		/* the pair must be free */
		if (sock == -1)
			continue;

		close(sock);
		sock = -1;
		return free_port + count * 2;
	}

	/* just get a free port */
	if (ipv6_enable == 0)
		res1 = eXosip_get_addrinfo(&addrinfo_rtp, "0.0.0.0", 0, transport);
	else
		res1 = eXosip_get_addrinfo(&addrinfo_rtp, "::", 0, transport);

	if (res1)
		return res1;

	sock = -1;
	for (curinfo_rtp = addrinfo_rtp; curinfo_rtp;
		 curinfo_rtp = curinfo_rtp->ai_next) {
		socklen_t len;
		struct sockaddr_storage ai_addr;

		if (curinfo_rtp->ai_protocol && curinfo_rtp->ai_protocol != transport) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO3, NULL,
						"eXosip: Skipping protocol %d\n",
						curinfo_rtp->ai_protocol));
			continue;
		}

		sock = (int) socket(curinfo_rtp->ai_family, curinfo_rtp->ai_socktype,
							curinfo_rtp->ai_protocol);
		if (sock < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot create socket!\n"));
			continue;
		}

		if (curinfo_rtp->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot set socket option!\n"));
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}

		res1 = bind(sock, curinfo_rtp->ai_addr, curinfo_rtp->ai_addrlen);
		if (res1 < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_WARNING, NULL,
						"eXosip: Cannot bind socket node: 0.0.0.0 family:%d\n",
						curinfo_rtp->ai_family));
			close(sock);
			sock = -1;
			continue;
		}

		len = sizeof(ai_addr);
		res1 = getsockname(sock, (struct sockaddr *) &ai_addr, &len);
		if (res1 != 0) {
			close(sock);
			sock = -1;
			continue;
		}

		close(sock);
		sock = -1;
		eXosip_freeaddrinfo(addrinfo_rtp);

		if (curinfo_rtp->ai_family == AF_INET)
			return ntohs(((struct sockaddr_in *) &ai_addr)->sin_port);
		else
			return ntohs(((struct sockaddr_in6 *) &ai_addr)->sin6_port);
	}

	eXosip_freeaddrinfo(addrinfo_rtp);

	if (sock != -1) {
		close(sock);
		sock = -1;
	}

	return OSIP_UNDEFINED_ERROR;
}
#endif

int
eXosip_listen_addr(int transport, const char *addr, int port, int family,
				   int secure)
{
	int i = -1;
	struct eXtl_protocol *eXtl = NULL;

	if (eXosip.eXtl != NULL) {
		/* already set */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: already listening somewhere\n"));
		return OSIP_WRONG_STATE;
	}

	if (transport == IPPROTO_UDP && secure == 0)
		eXtl = &eXtl_udp;
	else if (transport == IPPROTO_TCP && secure == 0)
		eXtl = &eXtl_tcp;
#ifdef HAVE_OPENSSL_SSL_H
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
	else if (transport == IPPROTO_UDP)
		eXtl = &eXtl_dtls;
#endif
	else if (transport == IPPROTO_TCP)
		eXtl = &eXtl_tls;
#endif

	if (eXtl == NULL)
		return OSIP_BADPARAMETER;

	eXtl->proto_family = family;
	eXtl->proto_port = port;
	if (addr != NULL)
		snprintf(eXtl->proto_ifs, sizeof(eXtl->proto_ifs), "%s", addr);

#ifdef	AF_INET6
	if (family == AF_INET6 && !addr)
		snprintf(eXtl->proto_ifs, sizeof(eXtl->proto_ifs), "::0");
#endif

	i = eXtl->tl_open();

	if (i != 0)
		return i;

	eXosip.eXtl = eXtl;

	if (transport == IPPROTO_UDP && secure == 0)
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "UDP");
	else if (transport == IPPROTO_TCP && secure == 0)
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "TCP");
	else if (transport == IPPROTO_UDP)
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "DTLS-UDP");
	else if (transport == IPPROTO_TCP)
		snprintf(eXosip.transport, sizeof(eXosip.transport), "%s", "TLS");

#ifdef OSIP_MT
	if (eXosip.j_thread == NULL) {
		eXosip.j_thread = (void *) osip_thread_create(20000, _eXosip_thread, NULL);
		if (eXosip.j_thread == NULL) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot start thread!\n"));
			return OSIP_UNDEFINED_ERROR;
		}
	}
#endif

	return OSIP_SUCCESS;
}

int eXosip_init(void)
{
	osip_t *osip;
	int i;

	memset(&eXosip, 0, sizeof(eXosip));

	eXosip.dscp = 0x1A;

	snprintf(eXosip.ipv4_for_gateway, 256, "%s", "217.12.3.11");
	snprintf(eXosip.ipv6_for_gateway, 256, "%s",
			 "2001:638:500:101:2e0:81ff:fe24:37c6");
#ifndef MINISIZE
	snprintf(eXosip.event_package, 256, "%s", "dialog");
#endif

#ifdef WIN32
	/* Initializing windows socket library */
	{
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(1, 1);
		i = WSAStartup(wVersionRequested, &wsaData);
		if (i != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_WARNING, NULL,
						"eXosip: Unable to initialize WINSOCK, reason: %d\n", i));
			/* return -1; It might be already initilized?? */
		}
	}
#endif

	eXosip.user_agent = osip_strdup("eXosip/" EXOSIP_VERSION);
	if (eXosip.user_agent == NULL)
		return OSIP_NOMEM;

	eXosip.j_calls = NULL;
	eXosip.j_stop_ua = 0;
#ifdef OSIP_MT
	eXosip.j_thread = NULL;
#endif
	i = osip_list_init(&eXosip.j_transactions);
	eXosip.j_reg = NULL;

#ifdef OSIP_MT
#if !defined (_WIN32_WCE)
	eXosip.j_cond = (struct osip_cond *) osip_cond_init();
	if (eXosip.j_cond == NULL) {
		osip_free(eXosip.user_agent);
		eXosip.user_agent = NULL;
		return OSIP_NOMEM;
	}
#endif

	eXosip.j_mutexlock = (struct osip_mutex *) osip_mutex_init();
	if (eXosip.j_mutexlock == NULL) {
		osip_free(eXosip.user_agent);
		eXosip.user_agent = NULL;
#if !defined (_WIN32_WCE)
		osip_cond_destroy((struct osip_cond *) eXosip.j_cond);
		eXosip.j_cond = NULL;
#endif
		return OSIP_NOMEM;
	}
#endif

	i = osip_init(&osip);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Cannot initialize osip!\n"));
		return i;
	}

	osip_set_application_context(osip, &eXosip);

	eXosip_set_callbacks(osip);

	eXosip.j_osip = osip;

#ifdef OSIP_MT
	/* open a TCP socket to wake up the application when needed. */
	eXosip.j_socketctl = jpipe();
	if (eXosip.j_socketctl == NULL)
		return OSIP_UNDEFINED_ERROR;

	eXosip.j_socketctl_event = jpipe();
	if (eXosip.j_socketctl_event == NULL)
		return OSIP_UNDEFINED_ERROR;
#endif

	/* To be changed in osip! */
	eXosip.j_events = (osip_fifo_t *) osip_malloc(sizeof(osip_fifo_t));
	if (eXosip.j_events == NULL)
		return OSIP_NOMEM;
	osip_fifo_init(eXosip.j_events);

	eXosip.use_rport = 1;
	eXosip.dns_capabilities = 2;
	eXosip.keep_alive = 17000;
	eXosip.keep_alive_options = 0;

	eXtl_udp.tl_init();
	eXtl_tcp.tl_init();
#ifdef HAVE_OPENSSL_SSL_H
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
	eXtl_dtls.tl_init();
#endif
	eXtl_tls.tl_init();
#endif
	return OSIP_SUCCESS;
}


int eXosip_execute(void)
{
	struct timeval lower_tv;
	int i;

#ifdef OSIP_MT
	osip_timers_gettimeout(eXosip.j_osip, &lower_tv);
	if (lower_tv.tv_sec > 10) {
		eXosip_reg_t *jr;
		time_t now;
		now = time(NULL);

		lower_tv.tv_sec = 10;

		eXosip_lock();
		for (jr = eXosip.j_reg; jr != NULL; jr = jr->next) {
			if (jr->r_id >= 1 && jr->r_last_tr != NULL) {
				if (jr->r_reg_period == 0) {
					/* skip refresh! */
				} else if (now - jr->r_last_tr->birth_time > jr->r_reg_period - (jr->r_reg_period/10)) {
					/* automatic refresh at "timeout - 10%" */
					lower_tv.tv_sec = 1;
				}
			}
		}
		eXosip_unlock();

		if (lower_tv.tv_sec==1)
		{
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"eXosip: Reseting timer to 1s before waking up!\n"));
		}
		else
		{
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"eXosip: Reseting timer to 10s before waking up!\n"));
		}
	} else {
		/*  add a small amount of time on windows to avoid
		   waking up too early. (probably a bad time precision) */
		if (lower_tv.tv_usec < 990000)
			lower_tv.tv_usec += 10000;	/* add 10ms */
		else {
			lower_tv.tv_usec = 10000;	/* add 10ms */
			lower_tv.tv_sec++;
		}
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"eXosip: timer sec:%i usec:%i!\n",
					lower_tv.tv_sec, lower_tv.tv_usec));
	}
#else
	lower_tv.tv_sec = 0;
	lower_tv.tv_usec = 0;
#endif
	i = eXosip_read_message(1, lower_tv.tv_sec, lower_tv.tv_usec);

	if (i == -2000) {
		return -2000;
	}

	eXosip_lock();
	osip_timers_ict_execute(eXosip.j_osip);
	osip_timers_nict_execute(eXosip.j_osip);
	osip_timers_ist_execute(eXosip.j_osip);
	osip_timers_nist_execute(eXosip.j_osip);

	osip_nist_execute(eXosip.j_osip);
	osip_nict_execute(eXosip.j_osip);
	osip_ist_execute(eXosip.j_osip);
	osip_ict_execute(eXosip.j_osip);

	/* free all Calls that are in the TERMINATED STATE? */
	eXosip_release_terminated_calls();
	eXosip_release_terminated_registrations();
	eXosip_release_terminated_publications();
#ifndef MINISIZE
	eXosip_release_terminated_subscriptions();
	eXosip_release_terminated_in_subscriptions();
#endif

	eXosip_unlock();

	_eXosip_keep_alive();

	return OSIP_SUCCESS;
}

int eXosip_set_option(int opt, const void *value)
{
	int val;
	char *tmp;

	switch (opt) {
	case EXOSIP_OPT_ADD_ACCOUNT_INFO:
		{
			struct eXosip_account_info *ainfo;
			int i;
			ainfo = (struct eXosip_account_info *) value;
			if (ainfo == NULL || ainfo->proxy[0] == '\0') {
				return OSIP_BADPARAMETER;
			}
			for (i = 0; i < MAX_EXOSIP_ACCOUNT_INFO; i++) {
				if (eXosip.account_entries[i].proxy[0] != '\0'
					&& 0 == osip_strcasecmp(eXosip.account_entries[i].proxy,
											ainfo->proxy)) {
					/* update ainfo */
					if (ainfo->nat_ip[0] != '\0') {
						snprintf(eXosip.account_entries[i].nat_ip,
								 sizeof(eXosip.account_entries[i].nat_ip), "%s",
								 ainfo->nat_ip);
						eXosip.account_entries[i].nat_port = ainfo->nat_port;
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO1, NULL,
									"eXosip option set: account info updated:%s -> %s:%i\n",
									ainfo->proxy, ainfo->nat_ip, ainfo->nat_port));
					} else {
						eXosip.account_entries[i].proxy[0] = '\0';
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO2, NULL,
									"eXosip option set: account info deleted :%s\n",
									ainfo->proxy));
					}
					return OSIP_SUCCESS;
				}
			}
			if (ainfo->nat_ip[0] == '\0') {
				return OSIP_BADPARAMETER;
			}
			/* not found case: */
			for (i = 0; i < MAX_EXOSIP_ACCOUNT_INFO; i++) {
				if (eXosip.account_entries[i].proxy[0] == '\0') {
					/* add ainfo */
					snprintf(eXosip.account_entries[i].proxy, sizeof(ainfo->proxy),
							 "%s", ainfo->proxy);
					snprintf(eXosip.account_entries[i].nat_ip,
							 sizeof(ainfo->nat_ip), "%s", ainfo->nat_ip);
					eXosip.account_entries[i].nat_port = ainfo->nat_port;
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"eXosip option set: account info added:%s -> %s:%i\n",
								ainfo->proxy, ainfo->nat_ip, ainfo->nat_port));
					return OSIP_SUCCESS;
				}
			}
			return OSIP_UNDEFINED_ERROR;
		}
		break;
	case EXOSIP_OPT_ADD_DNS_CACHE:
		{
			struct eXosip_dns_cache *entry;
			int i;
			entry = (struct eXosip_dns_cache *) value;
			if (entry == NULL || entry->host[0] == '\0') {
				return OSIP_BADPARAMETER;
			}
			for (i = 0; i < MAX_EXOSIP_DNS_ENTRY; i++) {
				if (eXosip.dns_entries[i].host[0] != '\0'
					&& 0 == osip_strcasecmp(eXosip.dns_entries[i].host,
											entry->host)) {
					/* update entry */
					if (entry->ip[0] != '\0') {
						snprintf(eXosip.dns_entries[i].ip,
								 sizeof(eXosip.dns_entries[i].ip), "%s",
								 entry->ip);
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO1, NULL,
									"eXosip option set: dns cache updated:%s -> %s\n",
									entry->host, entry->ip));
					} else {
						/* return previously added cache */
						snprintf(entry->ip, sizeof(entry->ip), "%s", eXosip.dns_entries[i].ip);
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO2, NULL,
									"eXosip option set: dns cache returned:%s ->%s\n",
									entry->host, entry->ip));
					}
					return OSIP_SUCCESS;
				}
			}
			if (entry->ip[0] == '\0') {
				char ipbuf[INET6_ADDRSTRLEN];
				struct __eXosip_sockaddr addr;
				struct addrinfo *addrinfo;

				/* create the A record */
				i = eXosip_get_addrinfo(&addrinfo, entry->host, 0, IPPROTO_UDP);
				if (i!=0)
					return OSIP_BADPARAMETER;

				memcpy(&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);

				eXosip_freeaddrinfo(addrinfo);
				switch (((struct sockaddr *) &addr)->sa_family) {
				case AF_INET:
					inet_ntop(((struct sockaddr *) &addr)->sa_family,
							  &(((struct sockaddr_in *) &addr)->sin_addr), ipbuf,
							  sizeof(ipbuf));
					break;
				case AF_INET6:
					inet_ntop(((struct sockaddr *) &addr)->sa_family,
							  &(((struct sockaddr_in6 *) &addr)->sin6_addr), ipbuf,
							  sizeof(ipbuf));
					break;
				default:
					return OSIP_BADPARAMETER;
				}

				if (osip_strcasecmp(ipbuf, entry->host)==0)
					return OSIP_SUCCESS;
				snprintf(entry->ip, sizeof(entry->ip), "%s", ipbuf);
			}
			/* not found case: */
			for (i = 0; i < MAX_EXOSIP_DNS_ENTRY; i++) {
				if (eXosip.dns_entries[i].host[0] == '\0') {
					/* add entry */
					snprintf(eXosip.dns_entries[i].host, sizeof(entry->host), "%s",
							 entry->host);
					snprintf(eXosip.dns_entries[i].ip, sizeof(entry->ip), "%s",
							 entry->ip);
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"eXosip option set: dns cache added:%s -> %s\n",
								entry->host, entry->ip));
					return OSIP_SUCCESS;
				}
			}
			return OSIP_UNDEFINED_ERROR;
		}
		break;
	case EXOSIP_OPT_DELETE_DNS_CACHE:
		{
			struct eXosip_dns_cache *entry;
			int i;
			entry = (struct eXosip_dns_cache *) value;
			if (entry == NULL || entry->host[0] == '\0') {
				return OSIP_BADPARAMETER;
			}
			for (i = 0; i < MAX_EXOSIP_DNS_ENTRY; i++) {
				if (eXosip.dns_entries[i].host[0] != '\0'
					&& 0 == osip_strcasecmp(eXosip.dns_entries[i].host,
											entry->host)) {
					eXosip.dns_entries[i].host[0] = '\0';
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"eXosip option set: dns cache deleted :%s\n",
								entry->host));
					return OSIP_SUCCESS;
				}
			}
			return OSIP_UNDEFINED_ERROR;
		}
		break;
	case EXOSIP_OPT_UDP_KEEP_ALIVE:
		val = *((int *) value);
		eXosip.keep_alive = val;	/* value in ms */
		break;
#ifdef ENABLE_KEEP_ALIVE_OPTIONS_METHOD
	case EXOSIP_OPT_KEEP_ALIVE_OPTIONS_METHOD:
		val = *((int *) value);
		eXosip.keep_alive_options = val;	/* value 0 or 1 */
		break;
#endif
	case EXOSIP_OPT_UDP_LEARN_PORT:
		val = *((int *) value);
		eXosip.learn_port = val;	/* 1 to learn port */
		break;
#ifndef MINISIZE
	case EXOSIP_OPT_SET_HTTP_TUNNEL_PORT:
		val = *((int *) value);
		eXosip.http_port = val;	/* value in ms */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: http_port:%i!\n", eXosip.http_port));
		break;
	case EXOSIP_OPT_SET_HTTP_TUNNEL_PROXY:
		tmp = (char *) value;
		memset(eXosip.http_proxy, '\0', sizeof(eXosip.http_proxy));
		if (tmp != NULL && tmp[0] != '\0')
			osip_strncpy(eXosip.http_proxy, tmp, sizeof(eXosip.http_proxy)-1);	/* value in proxy:port */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: http_proxy:%s!\n", eXosip.http_proxy));
		break;
	case EXOSIP_OPT_SET_HTTP_OUTBOUND_PROXY:
		tmp = (char *) value;
		memset(eXosip.http_outbound_proxy, '\0',
			   sizeof(eXosip.http_outbound_proxy));
		if (tmp != NULL && tmp[0] != '\0')
			osip_strncpy(eXosip.http_outbound_proxy, tmp, sizeof(eXosip.http_outbound_proxy)-1);	/* value in proxy:port */
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: http_outbound_proxy:%s!\n",
					eXosip.http_outbound_proxy));
		break;

	case EXOSIP_OPT_DONT_SEND_101:
		val = *((int *) value);
		eXosip.dontsend_101 = val;	/* 0 to disable */
		break;
#endif

	case EXOSIP_OPT_USE_RPORT:
		val = *((int *) value);
		eXosip.use_rport = val;	/* 0 to disable (for broken NAT only?) */
		break;

	case EXOSIP_OPT_SET_IPV4_FOR_GATEWAY:
		tmp = (char *) value;
		memset(eXosip.ipv4_for_gateway, '\0', sizeof(eXosip.ipv4_for_gateway));
		if (tmp != NULL && tmp[0] != '\0')
			osip_strncpy(eXosip.ipv4_for_gateway, tmp, sizeof(eXosip.ipv4_for_gateway)-1);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: ipv4_for_gateway:%s!\n",
					eXosip.ipv4_for_gateway));
		break;
#ifndef MINISIZE
	case EXOSIP_OPT_SET_IPV6_FOR_GATEWAY:
		tmp = (char *) value;
		memset(eXosip.ipv6_for_gateway, '\0', sizeof(eXosip.ipv6_for_gateway));
		if (tmp != NULL && tmp[0] != '\0')
			osip_strncpy(eXosip.ipv6_for_gateway, tmp, sizeof(eXosip.ipv6_for_gateway)-1);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: ipv6_for_gateway:%s!\n",
					eXosip.ipv6_for_gateway));
		break;
	case EXOSIP_OPT_EVENT_PACKAGE:
		tmp = (char *) value;
		memset(eXosip.event_package, '\0', sizeof(eXosip.event_package));
		if (tmp != NULL && tmp[0] != '\0')
			osip_strncpy(eXosip.event_package, tmp, sizeof(eXosip.event_package)-1);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip option set: event_package:%s!\n",
					eXosip.event_package));
		break;
#endif
	case EXOSIP_OPT_DNS_CAPABILITIES:	/*EXOSIP_OPT_SRV_WITH_NAPTR: */
		val = *((int *) value);
		/* 0: A request, 1: SRV support, 2: NAPTR+SRV support */
		eXosip.dns_capabilities = val;
		break;
	case EXOSIP_OPT_SET_DSCP:
		val = *((int *) value);
		/* 0x1A by default */
		eXosip.dscp = val;
		break;
	default:
		return OSIP_BADPARAMETER;
	}
	return OSIP_SUCCESS;
}

static void _eXosip_keep_alive(void)
{
	static struct timeval mtimer = { 0, 0 };

	struct timeval now;

	osip_gettimeofday(&now, NULL);

	if (mtimer.tv_sec == 0 && mtimer.tv_usec == 0) {
		/* first init */
		osip_gettimeofday(&mtimer, NULL);
		add_gettimeofday(&mtimer, eXosip.keep_alive);
	}

	if (osip_timercmp(&now, &mtimer, <)) {
		return;					/* not yet time */
	}

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"keep alive: %i\n", now.tv_sec - mtimer.tv_sec));

	/* reset timer */
	osip_gettimeofday(&mtimer, NULL);
	add_gettimeofday(&mtimer, eXosip.keep_alive);

	eXtl_udp.tl_keepalive();
	eXtl_tcp.tl_keepalive();
#ifdef HAVE_OPENSSL_SSL_H
	eXtl_tls.tl_keepalive();
#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)
	eXtl_dtls.tl_keepalive();
#endif
#endif
}

#ifdef OSIP_MT
void *_eXosip_thread(void *arg)
{
	int i;

	while (eXosip.j_stop_ua == 0) {
		i = eXosip_execute();
		if (i == -2000)
			osip_thread_exit();
	}
	osip_thread_exit();
	return NULL;
}

#endif
