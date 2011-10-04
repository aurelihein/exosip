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
#include "eXtransport.h"

#ifdef HAVE_OPENSSL_SSL_H

#include <openssl/ssl.h>

#if !(OPENSSL_VERSION_NUMBER < 0x00908000L)

#define SPROTO_TLS 500
#define SPROTO_DTLS 501
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>

#define SSLDEBUG 1

#define PASSWORD "password"
#define CLIENT_KEYFILE "ckey.pem"
#define CLIENT_CERTFILE "c.pem"
#define SERVER_KEYFILE "skey.pem"
#define SERVER_CERTFILE "s.pem"
#define CA_LIST "cacert.pem"
#define RANDOM  "random.pem"
#define DHFILE "dh1024.pem"

#if defined(_WIN32_WCE)
#define strerror(X) "-1"
#endif

#ifdef _WIN32_WCE
#include "inet_ntop.h"
#elif WIN32
#include "inet_ntop.h"
#endif

SSL_CTX *initialize_client_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport);

SSL_CTX *initialize_server_ctx(const char *keyfile, const char *certfile,
							   const char *password, int transport);

extern eXosip_t eXosip;

static int dtls_socket;
static struct sockaddr_storage ai_addr;

static char dtls_firewall_ip[64];
static char dtls_firewall_port[10];

static SSL_CTX *server_ctx;
static SSL_CTX *client_ctx;

/* persistent connection */
struct socket_tab {
	char remote_ip[65];
	int remote_port;
	SSL *ssl_conn;
	int ssl_state;
	int ssl_type;
};


#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS
#endif

static struct socket_tab dtls_socket_tab[EXOSIP_MAX_SOCKETS];

static int dtls_tl_init(void)
{
	dtls_socket = 0;
	server_ctx = NULL;
	client_ctx = NULL;
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	memset(&dtls_socket_tab, 0, sizeof(struct socket_tab) * EXOSIP_MAX_SOCKETS);
	memset(dtls_firewall_ip, 0, sizeof(dtls_firewall_ip));
	memset(dtls_firewall_port, 0, sizeof(dtls_firewall_port));
	return OSIP_SUCCESS;
}

int static print_ssl_error(int err)
{
	switch (err) {
	case SSL_ERROR_NONE:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"SSL ERROR NONE - OK\n"));
		break;
	case SSL_ERROR_ZERO_RETURN:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"SSL ERROR ZERO RETURN - SHUTDOWN\n"));
		break;
	case SSL_ERROR_WANT_READ:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want read\n"));
		break;
	case SSL_ERROR_WANT_WRITE:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL want write\n"));
		break;
	case SSL_ERROR_SSL:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR\n"));
		break;
	case SSL_ERROR_SYSCALL:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL ERROR SYSCALL\n"));
		break;
	default:
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL, "SSL problem\n"));
	}
	return OSIP_SUCCESS;
}

static int shutdown_free_server_dtls(int pos)
{
	int i, err;

	if (dtls_socket_tab[pos].ssl_type == 1) {
		if (dtls_socket_tab[pos].ssl_conn != NULL) {
#ifdef SSLDEBUG
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
								  "DTLS-UDP server SSL_shutdown\n"));
#endif

			i = SSL_shutdown(dtls_socket_tab[pos].ssl_conn);

			if (i <= 0) {
				err = SSL_get_error(dtls_socket_tab[pos].ssl_conn, i);
				print_ssl_error(err);
#ifdef SSLDEBUG

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "DTLS-UDP server shutdown <= 0\n"));
#endif
			} else {
#ifdef SSLDEBUG
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
									  "DTLS-UDP server shutdown > 0\n"));
#endif

			}

			SSL_free(dtls_socket_tab[pos].ssl_conn);

#if 0
			if (dtls_socket_tab[pos].ssl_ctx != NULL)
				SSL_CTX_free(dtls_socket_tab[pos].ssl_ctx);
#endif

			memset(&(dtls_socket_tab[pos]), 0, sizeof(struct socket_tab));

			return OSIP_SUCCESS;
		} else {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "DTLS-UDP server shutdown: invalid SSL object!\n"));
			return -1;
		}
	}
	return -1;
}

static int shutdown_free_client_dtls(int pos)
{
	int i, err;
	BIO *rbio;

	struct addrinfo *addrinfo;
	struct __eXosip_sockaddr addr;

	if (dtls_socket_tab[pos].ssl_type == 2) {
		if (dtls_socket_tab[pos].ssl_conn != NULL) {

			i = eXosip_get_addrinfo(&addrinfo,
									dtls_socket_tab[pos].remote_ip,
									dtls_socket_tab[pos].remote_port, IPPROTO_UDP);
			if (i != 0) {
				return -1;
			}

			memcpy(&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
			eXosip_freeaddrinfo(addrinfo);

			rbio = BIO_new_dgram(dtls_socket, BIO_NOCLOSE);

			BIO_dgram_set_peer(rbio, &addr);

			(dtls_socket_tab[pos].ssl_conn)->rbio = rbio;

			i = SSL_shutdown(dtls_socket_tab[pos].ssl_conn);

			if (i <= 0) {
				err = SSL_get_error(dtls_socket_tab[pos].ssl_conn, i);
#ifdef SSLDEBUG

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "DTLS-UDP client shutdown error %d <= 0\n", i));
#endif

				print_ssl_error(err);
			} else {
#ifdef SSLDEBUG
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
									  "DTLS-UDP client shutdown > 0\n"));
#endif

			}

			SSL_free(dtls_socket_tab[pos].ssl_conn);

#if 0
			if (dtls_socket_tab[pos].ssl_ctx != NULL)
				SSL_CTX_free(dtls_socket_tab[pos].ssl_ctx);
#endif

			memset(&(dtls_socket_tab[pos]), 0, sizeof(struct socket_tab));

			return OSIP_SUCCESS;
		} else {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "DTLS-UDP client shutdown: invalid SSL object!\n"));
			return -1;
		}
	}
	return -1;
}

static int dtls_tl_free(void)
{
	int pos;
	if (server_ctx != NULL)
		SSL_CTX_free(server_ctx);

	if (client_ctx != NULL)
		SSL_CTX_free(client_ctx);

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (dtls_socket_tab[pos].ssl_conn != NULL) {
			shutdown_free_client_dtls(pos);
			shutdown_free_server_dtls(pos);
		}
	}
	memset(&dtls_socket_tab, 0, sizeof(struct socket_tab) * EXOSIP_MAX_SOCKETS);

	memset(dtls_firewall_ip, 0, sizeof(dtls_firewall_ip));
	memset(dtls_firewall_port, 0, sizeof(dtls_firewall_port));
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	if (dtls_socket > 0)
		close(dtls_socket);
	dtls_socket = 0;
	return OSIP_SUCCESS;
}

static int dtls_tl_open(void)
{
	int res;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *curinfo;
	int sock = -1;

	if (eXtl_dtls.proto_port < 0)
		eXtl_dtls.proto_port = 5061;

	server_ctx = initialize_server_ctx(SERVER_KEYFILE, SERVER_CERTFILE, PASSWORD,
									   IPPROTO_UDP);
	client_ctx = initialize_client_ctx(CLIENT_KEYFILE, CLIENT_CERTFILE, PASSWORD,
									   IPPROTO_UDP);

	res = eXosip_get_addrinfo(&addrinfo,
							  eXtl_dtls.proto_ifs,
							  eXtl_dtls.proto_port, eXtl_dtls.proto_num);
	if (res)
		return -1;

	for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next) {
		socklen_t len;

		if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_dtls.proto_num) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO3, NULL,
						"eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
			continue;
		}

		sock = (int) socket(curinfo->ai_family, curinfo->ai_socktype,
							curinfo->ai_protocol);
		if (sock < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot create socket %s!\n", strerror(errno)));
			continue;
		}

		if (curinfo->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				close(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot set socket option %s!\n",
							strerror(errno)));
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}

		res = bind(sock, curinfo->ai_addr, curinfo->ai_addrlen);
		if (res < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot bind socket node:%s family:%d %s\n",
						eXtl_dtls.proto_ifs, curinfo->ai_family, strerror(errno)));
			close(sock);
			sock = -1;
			continue;
		}
		len = sizeof(ai_addr);
		res = getsockname(sock, (struct sockaddr *) &ai_addr, &len);
		if (res != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"eXosip: Cannot get socket name (%s)\n", strerror(errno)));
			memcpy(&ai_addr, curinfo->ai_addr, curinfo->ai_addrlen);
		}

		if (eXtl_dtls.proto_num == IPPROTO_TCP) {
			res = listen(sock, SOMAXCONN);
			if (res < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"eXosip: Cannot bind socket node:%s family:%d %s\n",
							eXtl_dtls.proto_ifs, curinfo->ai_family,
							strerror(errno)));
				close(sock);
				sock = -1;
				continue;
			}
		}

		break;
	}

	eXosip_freeaddrinfo(addrinfo);

	if (sock < 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"eXosip: Cannot bind on port: %i\n", eXtl_dtls.proto_port));
		return -1;
	}

	dtls_socket = sock;

	if (eXtl_dtls.proto_port == 0) {
		/* get port number from socket */
		if (eXtl_dtls.proto_family == AF_INET)
			eXtl_dtls.proto_port =
				ntohs(((struct sockaddr_in *) &ai_addr)->sin_port);
		else
			eXtl_dtls.proto_port =
				ntohs(((struct sockaddr_in6 *) &ai_addr)->sin6_port);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"eXosip: Binding on port %i!\n", eXtl_dtls.proto_port));
	}

	snprintf(dtls_firewall_port, sizeof(dtls_firewall_port), "%i",
			 eXtl_dtls.proto_port);
	return OSIP_SUCCESS;
}

#define EXOSIP_AS_A_SERVER 1
#define EXOSIP_AS_A_CLIENT 2

static int dtls_tl_set_fdset(fd_set * osip_fdset, fd_set * osip_wrset, int *fd_max)
{
	if (dtls_socket <= 0)
		return -1;

	eXFD_SET(dtls_socket, osip_fdset);

	if (dtls_socket > *fd_max)
		*fd_max = dtls_socket;

	return OSIP_SUCCESS;
}

static int dtls_tl_read_message(fd_set * osip_fdset, fd_set * osip_wrset)
{
	char *enc_buf;
	char *dec_buf;
	int i;
	int enc_buf_len;

	if (dtls_socket <= 0)
		return -1;

	if (FD_ISSET(dtls_socket, osip_fdset)) {
		struct sockaddr_storage sa;

#ifdef __linux
		socklen_t slen;
#else
		int slen;
#endif
		if (eXtl_dtls.proto_family == AF_INET)
			slen = sizeof(struct sockaddr_in);
		else
			slen = sizeof(struct sockaddr_in6);

		enc_buf = (char *) osip_malloc(SIP_MESSAGE_MAX_LENGTH * sizeof(char) + 1);
		if (enc_buf == NULL)
			return OSIP_NOMEM;

		enc_buf_len = recvfrom(dtls_socket, enc_buf,
							   SIP_MESSAGE_MAX_LENGTH, 0,
							   (struct sockaddr *) &sa, &slen);

		if (enc_buf_len > 5) {
			char src6host[NI_MAXHOST];
			int recvport = 0;
			int err;

			BIO *rbio;
			struct socket_tab *socket_tab_used = NULL;
			int pos;

			enc_buf[enc_buf_len] = '\0';
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
								  "Received message: \n%s\n", enc_buf));

			memset(src6host, 0, sizeof(src6host));

			if (eXtl_dtls.proto_family == AF_INET)
				recvport = ntohs(((struct sockaddr_in *) &sa)->sin_port);
			else
				recvport = ntohs(((struct sockaddr_in6 *) &sa)->sin6_port);

#if defined(__arc__)
			{
				struct sockaddr_in *fromsa = (struct sockaddr_in *) &sa;
				char *tmp;
				tmp = inet_ntoa(fromsa->sin_addr);
				if (tmp == NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"Message received from: NULL:%i inet_ntoa failure\n",
								recvport));
				} else {
					snprintf(src6host, sizeof(src6host), "%s", tmp);
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"Message received from: %s:%i\n", src6host,
								recvport));
				}
			}
#else
			err = getnameinfo((struct sockaddr *) &sa, slen,
							  src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (err != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Message received from: NULL:%i getnameinfo failure\n",
							recvport));
				snprintf(src6host, sizeof(src6host), "127.0.0.1");
			} else {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"Message received from: %s:%i\n", src6host, recvport));
			}
#endif

			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO1, NULL,
						"Message received from: %s:%i\n", src6host, recvport));

			for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
				if (dtls_socket_tab[pos].ssl_conn != NULL) {
					if (dtls_socket_tab[pos].remote_port == recvport &&
						(strcmp(dtls_socket_tab[pos].remote_ip, src6host) == 0)) {
						socket_tab_used = &dtls_socket_tab[pos];
						break;
					}
				}
			}

			if (socket_tab_used == NULL) {
				for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
					if (dtls_socket_tab[pos].ssl_conn == NULL) {
						/* should accept this connection? */
						break;
					}
				}

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
									  "creating DTLS-UDP socket at index: %i\n", pos));
				if (pos < 0) {
					/* delete an old one! */
					pos = 0;
					if (dtls_socket_tab[pos].ssl_conn != NULL) {
						shutdown_free_client_dtls(pos);
						shutdown_free_server_dtls(pos);
					}

					memset(&dtls_socket_tab[pos], 0, sizeof(struct socket_tab));
				}
			}

			if (dtls_socket_tab[pos].ssl_conn == NULL) {
				BIO *wbio;
				if (!SSL_CTX_check_private_key(server_ctx)) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"SSL CTX private key check error\n"));
					osip_free(enc_buf);
					return -1;
				}

				/* behave as a server: */
				dtls_socket_tab[pos].ssl_conn = SSL_new(server_ctx);
				if (dtls_socket_tab[pos].ssl_conn == NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"SSL_new error\n"));
					osip_free(enc_buf);
					return -1;
				}

				/* No MTU query */
#ifdef	SSL_OP_NO_QUERY_MTU
				SSL_set_options(dtls_socket_tab[pos].ssl_conn,
								SSL_OP_NO_QUERY_MTU);
				SSL_set_mtu(dtls_socket_tab[pos].ssl_conn, 2000);
#endif
				/* MTU query */
				/* BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL); */
#ifdef	SSL_OP_COOKIE_EXCHANGE
				SSL_set_options(dtls_socket_tab[pos].ssl_conn,
								SSL_OP_COOKIE_EXCHANGE);
#endif
				wbio = BIO_new_dgram(dtls_socket, BIO_NOCLOSE);
				BIO_dgram_set_peer(wbio, &sa);
				SSL_set_bio(dtls_socket_tab[pos].ssl_conn, NULL, wbio);

				SSL_set_accept_state(dtls_socket_tab[pos].ssl_conn);

				dtls_socket_tab[pos].ssl_state = 0;
				dtls_socket_tab[pos].ssl_type = EXOSIP_AS_A_SERVER;

				osip_strncpy(dtls_socket_tab[pos].remote_ip, src6host,
							 sizeof(dtls_socket_tab[pos].remote_ip) - 1);
				dtls_socket_tab[pos].remote_port = recvport;

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "New DTLS-UDP connection accepted\n"));

			}

			dec_buf =
				(char *) osip_malloc(SIP_MESSAGE_MAX_LENGTH * sizeof(char) + 1);
			if (dec_buf == NULL) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Allocation error\n"));
				osip_free(enc_buf);
				return OSIP_NOMEM;
			}
			rbio = BIO_new_mem_buf(enc_buf, enc_buf_len);
			BIO_set_mem_eof_return(rbio, -1);

			dtls_socket_tab[pos].ssl_conn->rbio = rbio;

			i = SSL_read(dtls_socket_tab[pos].ssl_conn, dec_buf,
						 SIP_MESSAGE_MAX_LENGTH);
			/* done with the rbio */
			BIO_free(dtls_socket_tab[pos].ssl_conn->rbio);
			dtls_socket_tab[pos].ssl_conn->rbio = BIO_new(BIO_s_mem());

			if (i > 5) {
				dec_buf[i] = '\0';

				_eXosip_handle_incoming_message(dec_buf, i, dtls_socket, src6host,
												recvport);

			}
#ifndef MINISIZE
			else if (i <= 0) {
				err = SSL_get_error(dtls_socket_tab[pos].ssl_conn, i);
				print_ssl_error(err);
				if (err == SSL_ERROR_SYSCALL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_WARNING,
								NULL, "DTLS-UDP SYSCALL on SSL_read\n"));
				} else if (err == SSL_ERROR_SSL || err == SSL_ERROR_ZERO_RETURN) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_WARNING,
								NULL, "DTLS-UDP closed\n"));

					shutdown_free_client_dtls(pos);
					shutdown_free_server_dtls(pos);

					memset(&(dtls_socket_tab[pos]), 0,
						   sizeof(dtls_socket_tab[pos]));
				}
			} else {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									  "Dummy SIP message received\n"));
			}
#endif

			osip_free(dec_buf);
			osip_free(enc_buf);

		}
	}

	return OSIP_SUCCESS;
}

static int eXtl_update_local_target(osip_message_t * req)
{
	int pos = 0;

	if (dtls_firewall_ip != '\0') {

		while (!osip_list_eol(&req->contacts, pos)) {
			osip_contact_t *co;

			co = (osip_contact_t *) osip_list_get(&req->contacts, pos);
			pos++;
			if (co != NULL && co->url != NULL && co->url->host != NULL
				&& 0 == osip_strcasecmp(co->url->host, dtls_firewall_ip)) {
				if (co->url->port == NULL &&
					0 != osip_strcasecmp(dtls_firewall_port, "5061")) {
					co->url->port = osip_strdup(dtls_firewall_port);
				} else if (co->url->port != NULL &&
						   0 != osip_strcasecmp(dtls_firewall_port, co->url->port))
				{
					osip_free(co->url->port);
					co->url->port = osip_strdup(dtls_firewall_port);
				}
			}
		}
	}

	return OSIP_SUCCESS;
}

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static int
dtls_tl_send_message(osip_transaction_t * tr, osip_message_t * sip, char *host,
					 int port, int out_socket)
{
	int len = 0;
	size_t length = 0;
	struct addrinfo *addrinfo;
	struct __eXosip_sockaddr addr;
	char *message;

	char ipbuf[INET6_ADDRSTRLEN];
	int i;
	osip_naptr_t *naptr_record=NULL;

	int pos;
	struct socket_tab *socket_tab_used = NULL;
	BIO *sbio = NULL;

	if (dtls_socket <= 0)
		return -1;

	if (host == NULL) {
		host = sip->req_uri->host;
		if (sip->req_uri->port != NULL)
			port = osip_atoi(sip->req_uri->port);
		else
			port = 5061;
	}

	if (port == 5060)
		port = 5061;

	if (MSG_IS_REQUEST(sip)) {
		if (MSG_IS_REGISTER(sip)
			|| MSG_IS_INVITE(sip) || MSG_IS_SUBSCRIBE(sip) || MSG_IS_NOTIFY(sip))
			eXtl_update_local_target(sip);
	}

	i = -1;
#ifndef MINISIZE
	if (tr==NULL)
	{
		_eXosip_srv_lookup(sip, &naptr_record);

		if (naptr_record!=NULL) {
			eXosip_dnsutils_dns_process(naptr_record, 1);
			if (naptr_record->naptr_state==OSIP_NAPTR_STATE_NAPTRDONE
				||naptr_record->naptr_state==OSIP_NAPTR_STATE_SRVINPROGRESS)
				eXosip_dnsutils_dns_process(naptr_record, 1);
		}

		if (naptr_record!=NULL && naptr_record->naptr_state==OSIP_NAPTR_STATE_SRVDONE)
		{
			/* 4: check if we have the one we want... */
			if (naptr_record->sipdtls_record.name[0] != '\0'
				&& naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index].srv[0] != '\0') {
					/* always choose the first here.
					if a network error occur, remove first entry and
					replace with next entries.
					*/
					osip_srv_entry_t *srv;
					int n = 0;
					for (srv = &naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index];
						n < 10 && naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index].srv[0];
						srv = &naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index]) {
							if (srv->ipaddress[0])
								i = eXosip_get_addrinfo(&addrinfo, srv->ipaddress, srv->port, IPPROTO_UDP);
							else
								i = eXosip_get_addrinfo(&addrinfo, srv->srv, srv->port, IPPROTO_UDP);
							if (i == 0) {
								host = srv->srv;
								port = srv->port;
								break;
							}

							i = eXosip_dnsutils_rotate_srv(&naptr_record->sipdtls_record);
							if (i<=0)
							{
								return -1;
							}
							if (i>=n)
							{
								return -1;
							}
							i = -1;
							/* copy next element */
							n++;
					}
			}
		}

		if (naptr_record!=NULL && naptr_record->keep_in_cache==0)
			osip_free(naptr_record);
		naptr_record=NULL;
	}
	else
	{
		naptr_record = tr->naptr_record;
	}

	if (naptr_record!=NULL)
	{
		/* 1: make sure there is no pending DNS */
		eXosip_dnsutils_dns_process(naptr_record, 0);
		if (naptr_record->naptr_state==OSIP_NAPTR_STATE_NAPTRDONE
			||naptr_record->naptr_state==OSIP_NAPTR_STATE_SRVINPROGRESS)
			eXosip_dnsutils_dns_process(naptr_record, 0);

		if (naptr_record->naptr_state==OSIP_NAPTR_STATE_UNKNOWN)
		{
			/* fallback to DNS A */
			if (naptr_record->keep_in_cache==0)
				osip_free(naptr_record);
			naptr_record=NULL;
			if (tr!=NULL)
				tr->naptr_record=NULL;
			/* must never happen? */
		}
		else if (naptr_record->naptr_state==OSIP_NAPTR_STATE_INPROGRESS)
		{
			/* 2: keep waiting (naptr answer not received) */
			return OSIP_SUCCESS + 1;
		}
		else if (naptr_record->naptr_state==OSIP_NAPTR_STATE_NAPTRDONE)
		{
			/* 3: keep waiting (naptr answer received/no srv answer received) */
			return OSIP_SUCCESS + 1;
		}
		else if (naptr_record->naptr_state==OSIP_NAPTR_STATE_SRVINPROGRESS)
		{
			/* 3: keep waiting (naptr answer received/no srv answer received) */
			return OSIP_SUCCESS + 1;
		}
		else if (naptr_record->naptr_state==OSIP_NAPTR_STATE_SRVDONE)
		{
			/* 4: check if we have the one we want... */
			if (naptr_record->sipdtls_record.name[0] != '\0'
				&& naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index].srv[0] != '\0') {
					/* always choose the first here.
					if a network error occur, remove first entry and
					replace with next entries.
					*/
					osip_srv_entry_t *srv;
					int n = 0;
					for (srv = &naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index];
						n < 10 && naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index].srv[0];
						srv = &naptr_record->sipdtls_record.srventry[naptr_record->sipdtls_record.index]) {
							if (srv->ipaddress[0])
								i = eXosip_get_addrinfo(&addrinfo, srv->ipaddress, srv->port, IPPROTO_UDP);
							else
								i = eXosip_get_addrinfo(&addrinfo, srv->srv, srv->port, IPPROTO_UDP);
							if (i == 0) {
								host = srv->srv;
								port = srv->port;
								break;
							}
							
							i = eXosip_dnsutils_rotate_srv(&naptr_record->sipdtls_record);
							if (i<=0)
							{
								return -1;
							}
							if (i>=n)
							{
								return -1;
							}
							i = -1;
							/* copy next element */
							n++;
					}
			}
		}
		else if (naptr_record->naptr_state==OSIP_NAPTR_STATE_NOTSUPPORTED
			||naptr_record->naptr_state==OSIP_NAPTR_STATE_RETRYLATER)
		{
			/* 5: fallback to DNS A */
			if (naptr_record->keep_in_cache==0)
				osip_free(naptr_record);
			naptr_record=NULL;
			if (tr!=NULL)
				tr->naptr_record=NULL;
		}
	}
#endif

	/* if SRV was used, destination may be already found */
	if (i != 0) {
		i = eXosip_get_addrinfo(&addrinfo, host, port, IPPROTO_UDP);
	}

	if (i != 0) {
		return -1;
	}

	memcpy(&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
	len = addrinfo->ai_addrlen;

	eXosip_freeaddrinfo(addrinfo);

	/* remove preloaded route if there is no tag in the To header
	 */
	{
		osip_route_t *route = NULL;
		osip_generic_param_t *tag = NULL;
		osip_message_get_route(sip, 0, &route);

		osip_to_get_tag(sip->to, &tag);
		if (tag == NULL && route != NULL && route->url != NULL) {
			osip_list_remove(&sip->routes, 0);
		}
		i = osip_message_to_str(sip, &message, &length);
		if (tag == NULL && route != NULL && route->url != NULL) {
			osip_list_add(&sip->routes, route, 0);
		}
	}

	if (i != 0 || length <= 0) {
		return -1;
	}

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
		strncpy(ipbuf, "(unknown)", sizeof(ipbuf));
		break;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "Message sent: \n%s (to dest=%s:%i)\n",
						  message, ipbuf, port));

	if (osip_strcasecmp(host, ipbuf)!=0 && MSG_IS_REQUEST(sip)) {
		if (MSG_IS_REGISTER(sip)) {
			struct eXosip_dns_cache entry;

			memset(&entry, 0, sizeof(struct eXosip_dns_cache));
			snprintf(entry.host, sizeof(entry.host), "%s", host);
			snprintf(entry.ip, sizeof(entry.ip), "%s", ipbuf);
			eXosip_set_option(EXOSIP_OPT_ADD_DNS_CACHE, (void *) &entry);
		}
	}

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (dtls_socket_tab[pos].ssl_conn != NULL
			&& dtls_socket_tab[pos].ssl_type == EXOSIP_AS_A_SERVER) {
			if (dtls_socket_tab[pos].remote_port == port &&
				(strcmp(dtls_socket_tab[pos].remote_ip, ipbuf) == 0)) {
				BIO *rbio;
				socket_tab_used = &dtls_socket_tab[pos];
				rbio = BIO_new_dgram(dtls_socket, BIO_NOCLOSE);
				BIO_dgram_set_peer(rbio, &addr);
				dtls_socket_tab[pos].ssl_conn->rbio = rbio;
				break;
			}
		}
	}
	if (socket_tab_used == NULL) {
		for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
			if (dtls_socket_tab[pos].ssl_conn != NULL
				&& dtls_socket_tab[pos].ssl_type == EXOSIP_AS_A_CLIENT) {
				if (dtls_socket_tab[pos].remote_port == port &&
					(strcmp(dtls_socket_tab[pos].remote_ip, ipbuf) == 0)) {
					BIO *rbio;
					socket_tab_used = &dtls_socket_tab[pos];
					rbio = BIO_new_dgram(dtls_socket, BIO_NOCLOSE);
					BIO_dgram_set_peer(rbio, &addr);
					dtls_socket_tab[pos].ssl_conn->rbio = rbio;
					break;
				}
			}
		}
	}

	if (socket_tab_used == NULL) {
		/* delete an old one! */
		pos = 0;
		if (dtls_socket_tab[pos].ssl_conn != NULL) {
			shutdown_free_client_dtls(pos);
			shutdown_free_server_dtls(pos);
		}

		memset(&dtls_socket_tab[pos], 0, sizeof(struct socket_tab));
	}

	if (dtls_socket_tab[pos].ssl_conn == NULL) {
		/* create a new one */
		SSL_CTX_set_read_ahead(client_ctx, 1);
		dtls_socket_tab[pos].ssl_conn = SSL_new(client_ctx);

		if (dtls_socket_tab[pos].ssl_conn == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "DTLS-UDP SSL_new error\n"));

			if (dtls_socket_tab[pos].ssl_conn != NULL) {
				shutdown_free_client_dtls(pos);
				shutdown_free_server_dtls(pos);
			}

			memset(&dtls_socket_tab[pos], 0, sizeof(struct socket_tab));

			osip_free(message);
			return -1;
		}

		if (connect(dtls_socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "DTLS-UDP connect error\n"));
			if (dtls_socket_tab[pos].ssl_conn != NULL) {
				shutdown_free_client_dtls(pos);
				shutdown_free_server_dtls(pos);
			}

			memset(&dtls_socket_tab[pos], 0, sizeof(struct socket_tab));

			osip_free(message);
			return -1;
		}

		SSL_set_options(dtls_socket_tab[pos].ssl_conn, SSL_OP_NO_QUERY_MTU);
		SSL_set_mtu(dtls_socket_tab[pos].ssl_conn, 2000);
		SSL_set_connect_state(dtls_socket_tab[pos].ssl_conn);
		sbio = BIO_new_dgram(dtls_socket, BIO_NOCLOSE);
		BIO_ctrl_set_connected(sbio, 1, (struct sockaddr *) &addr);
		SSL_set_bio(dtls_socket_tab[pos].ssl_conn, sbio, sbio);

		dtls_socket_tab[pos].ssl_type = 2;
		dtls_socket_tab[pos].ssl_state = 2;

		osip_strncpy(dtls_socket_tab[pos].remote_ip, ipbuf,
					 sizeof(dtls_socket_tab[pos].remote_ip) - 1);
		dtls_socket_tab[pos].remote_port = port;
	}

	i = SSL_write(dtls_socket_tab[pos].ssl_conn, message, length);

	if (i < 0) {
		i = SSL_get_error(dtls_socket_tab[pos].ssl_conn, i);
		print_ssl_error(i);
		if (i == SSL_ERROR_SSL || i == SSL_ERROR_SYSCALL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "DTLS-UDP SSL_write error\n"));
			if (dtls_socket_tab[pos].ssl_conn != NULL) {
				shutdown_free_client_dtls(pos);
				shutdown_free_server_dtls(pos);
			}

			memset(&dtls_socket_tab[pos], 0, sizeof(struct socket_tab));

		}
#ifndef MINISIZE
		if (naptr_record!=NULL)
		{
			/* rotate on failure! */
			if (eXosip_dnsutils_rotate_srv(&naptr_record->sipdtls_record)>0)
			{
				osip_free(message);
				return OSIP_SUCCESS;	/* retry for next retransmission! */
			}
		}
#endif
		/* SIP_NETWORK_ERROR; */
		osip_free(message);
		return -1;
	}

	if (eXosip.keep_alive > 0) {
		if (MSG_IS_REGISTER(sip)) {
			eXosip_reg_t *reg = NULL;

			if (_eXosip_reg_find(&reg, tr) == 0) {
				memcpy(&(reg->addr), &addr, len);
				reg->len = len;
			}
		}
	}

	osip_free(message);
	return OSIP_SUCCESS;
}

static int dtls_tl_keepalive(void)
{
	char buf[4] = "jaK";
	eXosip_reg_t *jr;

	if (eXosip.keep_alive <= 0) {
		return 0;
	}

	if (dtls_socket <= 0)
		return OSIP_UNDEFINED_ERROR;

	for (jr = eXosip.j_reg; jr != NULL; jr = jr->next) {
		if (jr->len > 0) {
			if (sendto(dtls_socket, (const void *) buf, 4, 0,
					   (struct sockaddr *) &(jr->addr), jr->len) > 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"eXosip: Keep Alive sent on DTLS-UDP!\n"));
			}
		}
	}
	return OSIP_SUCCESS;
}

static int dtls_tl_set_socket(int socket)
{
	dtls_socket = socket;

	return OSIP_SUCCESS;
}

static int dtls_tl_masquerade_contact(const char *public_address, int port)
{
	if (public_address == NULL || public_address[0] == '\0') {
		memset(dtls_firewall_ip, '\0', sizeof(dtls_firewall_ip));
		memset(dtls_firewall_port, '\0', sizeof(dtls_firewall_port));
		if (eXtl_dtls.proto_port > 0)
			snprintf(dtls_firewall_port, sizeof(dtls_firewall_port), "%i",
					 eXtl_dtls.proto_port);
		return OSIP_SUCCESS;
	}
	snprintf(dtls_firewall_ip, sizeof(dtls_firewall_ip), "%s", public_address);
	if (port > 0) {
		snprintf(dtls_firewall_port, sizeof(dtls_firewall_port), "%i", port);
	}
	return OSIP_SUCCESS;
}

static int
dtls_tl_get_masquerade_contact(char *ip, int ip_size, char *port, int port_size)
{
	memset(ip, 0, ip_size);
	memset(port, 0, port_size);

	if (dtls_firewall_ip != '\0')
		snprintf(ip, ip_size, "%s", dtls_firewall_ip);

	if (dtls_firewall_port != '\0')
		snprintf(port, port_size, "%s", dtls_firewall_port);
	return OSIP_SUCCESS;
}

struct eXtl_protocol eXtl_dtls = {
	1,
	5061,
	"DTLS-UDP",
	"0.0.0.0",
	IPPROTO_UDP,
	AF_INET,
	0,
	0,

	&dtls_tl_init,
	&dtls_tl_free,
	&dtls_tl_open,
	&dtls_tl_set_fdset,
	&dtls_tl_read_message,
	&dtls_tl_send_message,
	&dtls_tl_keepalive,
	&dtls_tl_set_socket,
	&dtls_tl_masquerade_contact,
	&dtls_tl_get_masquerade_contact
};

#endif

#endif
