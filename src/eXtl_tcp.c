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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef WIN32
#include <Mstcpip.h>
#endif

#if defined(_WIN32_WCE) || defined(WIN32)
#define strerror(X) "-1"
#define ex_errno WSAGetLastError()
#define is_wouldblock_error(r) ((r)==WSAEINTR||(r)==WSAEWOULDBLOCK)
#define is_connreset_error(r) ((r)==WSAECONNRESET || (r)==WSAECONNABORTED || (r)==WSAETIMEDOUT || (r)==WSAENETRESET || (r)==WSAENOTCONN)
#else
#define ex_errno errno
#define closesocket close
#endif
#ifndef is_wouldblock_error
#define is_wouldblock_error(r) ((r)==EINTR||(r)==EWOULDBLOCK||(r)==EAGAIN)
#define is_connreset_error(r) ((r)==ECONNRESET || (r)==ECONNABORTED || (r)==ETIMEDOUT || (r)==ENETRESET || (r)==ENOTCONN)
#endif

extern eXosip_t eXosip;

#ifdef __APPLE_CC__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
#include <CoreFoundation/CFStream.h>
#include <CFNetwork/CFSocketStream.h>
#define MULTITASKING_ENABLED
#endif

static int tcp_socket;
static struct sockaddr_storage ai_addr;

static char tcp_firewall_ip[64];
static char tcp_firewall_port[10];

/* persistent connection */
struct _tcp_sockets {
	int socket;
	struct sockaddr ai_addr;
	size_t ai_addrlen;
	char remote_ip[65];
	int remote_port;
	char *buf;      /* recv buffer */
	size_t bufsize; /* allocated size of buf */
	size_t buflen;  /* current length of buf */
	char *sendbuf;  /* send buffer */
	size_t sendbufsize;
	size_t sendbuflen;
#ifdef MULTITASKING_ENABLED
	CFReadStreamRef readStream;
	CFWriteStreamRef writeStream;
#endif
};

#ifndef SOCKET_TIMEOUT
/* when stream has sequence error: */
/* having SOCKET_TIMEOUT > 0 helps the system to recover */
#define SOCKET_TIMEOUT 0
#endif

#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS 100
#endif

static int _tcp_tl_send_sockinfo (struct _tcp_sockets *sockinfo, const char *msg, int msglen);

static struct _tcp_sockets tcp_socket_tab[EXOSIP_MAX_SOCKETS];

static int tcp_tl_init(void)
{
	tcp_socket = 0;
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	memset(&tcp_socket_tab, 0, sizeof(struct _tcp_sockets) * EXOSIP_MAX_SOCKETS);
	memset(tcp_firewall_ip, 0, sizeof(tcp_firewall_ip));
	memset(tcp_firewall_port, 0, sizeof(tcp_firewall_port));
	return OSIP_SUCCESS;
}

static void _tcp_tl_close_sockinfo(struct _tcp_sockets *sockinfo)
{
	closesocket(sockinfo->socket);
	if (sockinfo->buf!=NULL)
		osip_free(sockinfo->buf);
	if (sockinfo->sendbuf!=NULL)
		osip_free(sockinfo->sendbuf);
#ifdef MULTITASKING_ENABLED
	if (sockinfo->readStream!=NULL)
	{
		CFReadStreamClose(sockinfo->readStream);
		CFRelease(sockinfo->readStream);						
	}
	if (sockinfo->writeStream!=NULL)
	{
		CFWriteStreamClose(sockinfo->writeStream);			
		CFRelease(sockinfo->writeStream);
	}
#endif
	memset(sockinfo, 0, sizeof(*sockinfo));
}

static int tcp_tl_free(void)
{
	int pos;
	memset(tcp_firewall_ip, 0, sizeof(tcp_firewall_ip));
	memset(tcp_firewall_port, 0, sizeof(tcp_firewall_port));
	memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
	if (tcp_socket > 0)
		closesocket(tcp_socket);

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket > 0) {
			_tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
		}
	}

	return OSIP_SUCCESS;
}

static int tcp_tl_open(void)
{
	int res;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *curinfo;
	int sock = -1;

	if (eXtl_tcp.proto_port < 0)
		eXtl_tcp.proto_port = 5060;


	res = eXosip_get_addrinfo(&addrinfo,
							  eXtl_tcp.proto_ifs,
							  eXtl_tcp.proto_port, eXtl_tcp.proto_num);
	if (res)
		return -1;

	for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next) {
		socklen_t len;

		if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_tcp.proto_num) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO3, NULL,
						"Skipping protocol %d\n", curinfo->ai_protocol));
			continue;
		}

		sock = (int) socket(curinfo->ai_family, curinfo->ai_socktype,
							curinfo->ai_protocol);
		if (sock < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"Cannot create socket %s!\n", strerror(ex_errno)));
			continue;
		}

		if (curinfo->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				closesocket(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Cannot set socket option %s!\n", strerror(ex_errno)));
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}

		res = bind(sock, curinfo->ai_addr, curinfo->ai_addrlen);
		if (res < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"Cannot bind socket node:%s family:%d %s\n",
						eXtl_tcp.proto_ifs, curinfo->ai_family,
						strerror(ex_errno)));
			closesocket(sock);
			sock = -1;
			continue;
		}
		len = sizeof(ai_addr);
		res = getsockname(sock, (struct sockaddr *) &ai_addr, &len);
		if (res != 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"Cannot get socket name (%s)\n", strerror(ex_errno)));
			memcpy(&ai_addr, curinfo->ai_addr, curinfo->ai_addrlen);
		}

		if (eXtl_tcp.proto_num == IPPROTO_TCP) {
			res = listen(sock, SOMAXCONN);
			if (res < 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Cannot bind socket node:%s family:%d %s\n",
							eXtl_tcp.proto_ifs, curinfo->ai_family,
							strerror(ex_errno)));
				closesocket(sock);
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
					"Cannot bind on port: %i\n", eXtl_tcp.proto_port));
		return -1;
	}

	tcp_socket = sock;

	if (eXtl_tcp.proto_port == 0) {
		/* get port number from socket */
		if (eXtl_tcp.proto_family == AF_INET)
			eXtl_tcp.proto_port =
				ntohs(((struct sockaddr_in *) &ai_addr)->sin_port);
		else
			eXtl_tcp.proto_port =
				ntohs(((struct sockaddr_in6 *) &ai_addr)->sin6_port);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"Binding on port %i!\n", eXtl_tcp.proto_port));
	}

	snprintf(tcp_firewall_port, sizeof(tcp_firewall_port), "%i",
			 eXtl_tcp.proto_port);
	return OSIP_SUCCESS;
}

static int tcp_tl_set_fdset(fd_set * osip_fdset, fd_set * osip_wrset, int *fd_max)
{
	int pos;
	if (tcp_socket <= 0)
		return -1;

	eXFD_SET(tcp_socket, osip_fdset);

	if (tcp_socket > *fd_max)
		*fd_max = tcp_socket;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket > 0) {
			eXFD_SET(tcp_socket_tab[pos].socket, osip_fdset);
			if (tcp_socket_tab[pos].socket > *fd_max)
				*fd_max = tcp_socket_tab[pos].socket;
			if (tcp_socket_tab[pos].sendbuflen > 0)
				eXFD_SET (tcp_socket_tab[pos].socket, osip_wrset);
		}
	}

	return OSIP_SUCCESS;
}

/* Like strstr, but works for haystack that may contain binary data and is
   not NUL-terminated. */
static char *buffer_find(const char *haystack, size_t haystack_len, const char *needle)
{
	const char *search = haystack, *end = haystack + haystack_len;
	char *p;
	size_t len = strlen(needle);

	while (search < end &&
		   (p = memchr(search, *needle, end - search)) != NULL) {
		if (p + len > end)
			break;
		if (memcmp(p, needle, len) == 0)
			return (p);
		search = p + 1;
	}

	return (NULL);
}

#define END_HEADERS_STR "\r\n\r\n"
#define CLEN_HEADER_STR "\r\ncontent-length:"
#define CLEN_HEADER_COMPACT_STR "\r\nl:"
#define CLEN_HEADER_STR2 "\r\ncontent-length "
#define CLEN_HEADER_COMPACT_STR2 "\r\nl "
#define const_strlen(x) (sizeof((x)) - 1)

/* consume any complete messages in sockinfo->buf and
   return the total number of bytes consumed */
static int handle_messages(struct _tcp_sockets *sockinfo)
{
	int consumed = 0;
	char *buf = sockinfo->buf;
	size_t buflen = sockinfo->buflen;
	char *end_headers;
	while (buflen > 0 && (end_headers = buffer_find(buf, buflen, END_HEADERS_STR)) != NULL) {
		int clen, msglen;
		char *clen_header;

		if (buf == end_headers)
		{
			/* skip tcp standard keep-alive */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									"socket %s:%i: standard keep alive received (CRLFCRLF)\n",
									sockinfo->remote_ip, sockinfo->remote_port, buf));
			consumed += 4;
			buflen -= 4;
			buf += 4;
			continue;
		}

		/* stuff a nul in so we can use osip_strcasestr */
		*end_headers = '\0';

		/* ok we have complete headers, find content-length: or l: */
		clen_header = osip_strcasestr(buf, CLEN_HEADER_STR);
		if (!clen_header)
			clen_header = osip_strcasestr(buf, CLEN_HEADER_STR2);
		if (!clen_header)
			clen_header = osip_strcasestr(buf, CLEN_HEADER_COMPACT_STR);
		if (!clen_header)
			clen_header = osip_strcasestr(buf, CLEN_HEADER_COMPACT_STR2);
		if (clen_header != NULL)
		{
			clen_header = strchr(clen_header, ':');
			clen_header++;
		}
		if (!clen_header)
		{
			/* Oops, no content-length header.	Presume 0 (below) so we
				consume the headers and make forward progress.  This permits
				server-side keepalive of "\r\n\r\n". */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									"socket %s:%i: message has no content-length: <%s>\n",
									sockinfo->remote_ip, sockinfo->remote_port, buf));
		}
		clen = clen_header ? atoi(clen_header) : 0;

		/* undo our overwrite and advance end_headers */
		*end_headers = END_HEADERS_STR[0];
		end_headers += const_strlen(END_HEADERS_STR);

		/* do we have the whole message? */
		msglen = end_headers - buf + clen;
		if (msglen > buflen) {
			/* nope */
			return consumed;
		}
		/* yep; handle the message */
		_eXosip_handle_incoming_message(buf, msglen, sockinfo->socket,
										sockinfo->remote_ip, sockinfo->remote_port);
		consumed += msglen;
		buflen -= msglen;
		buf += msglen;
	}

	return consumed;
}

static int _tcp_tl_recv(struct _tcp_sockets *sockinfo)
{
	int r;
	if (!sockinfo->buf) {
		sockinfo->buf = (char *) osip_malloc(SIP_MESSAGE_MAX_LENGTH);
		if (sockinfo->buf == NULL)
			return OSIP_NOMEM;
		sockinfo->bufsize = SIP_MESSAGE_MAX_LENGTH;
		sockinfo->buflen = 0;
	}

	/* buffer is 100% full -> realloc with more size */
	if (sockinfo->bufsize - sockinfo->buflen<=0)
	{
		sockinfo->buf = (char *)osip_realloc(sockinfo->buf, sockinfo->bufsize+1000);
		if (sockinfo->buf == NULL)
			return OSIP_NOMEM;
		sockinfo->bufsize = sockinfo->bufsize+1000;
	}

	/* buffer is 100% empty-> realloc with initial size */
	if (sockinfo->buflen == 0 && sockinfo->bufsize>SIP_MESSAGE_MAX_LENGTH)
	{
		osip_free(sockinfo->buf);
		sockinfo->buf = (char *) osip_malloc(SIP_MESSAGE_MAX_LENGTH);
		if (sockinfo->buf == NULL)
			return OSIP_NOMEM;
		sockinfo->bufsize = SIP_MESSAGE_MAX_LENGTH;
	}

	r = recv(sockinfo->socket, sockinfo->buf + sockinfo->buflen, sockinfo->bufsize - sockinfo->buflen, 0);
	if (r == 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "socket %s:%i: eof\n", sockinfo->remote_ip, sockinfo->remote_port));
		_tcp_tl_close_sockinfo(sockinfo);
		eXosip_mark_all_registrations_expired();
		return OSIP_UNDEFINED_ERROR;
	} else if (r < 0) {
		int status = ex_errno;
		if (is_wouldblock_error(status))
			return OSIP_SUCCESS;
		/* Do we need next line ? */
		/* else if (is_connreset_error(status)) */
		eXosip_mark_all_registrations_expired();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "socket %s:%i: error %d\n", sockinfo->remote_ip, sockinfo->remote_port, status));
		_tcp_tl_close_sockinfo(sockinfo);
		return OSIP_UNDEFINED_ERROR;
	} else {
		int consumed;
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "socket %s:%i: read %d bytes\n", sockinfo->remote_ip, sockinfo->remote_port, r));
		sockinfo->buflen += r;
		consumed = handle_messages(sockinfo);
		if (consumed == 0) {
			return OSIP_SUCCESS;
		} else {
			if (sockinfo->buflen > consumed) {
				memmove(sockinfo->buf, sockinfo->buf + consumed, sockinfo->buflen - consumed);
				sockinfo->buflen -= consumed;
			} else {
				sockinfo->buflen = 0;
			}
			return OSIP_SUCCESS;
		}
	}
}

static int tcp_tl_read_message(fd_set * osip_fdset, fd_set * osip_wrset)
{
	int pos = 0;

	if (FD_ISSET(tcp_socket, osip_fdset)) {
		/* accept incoming connection */
		char src6host[NI_MAXHOST];
		int recvport = 0;
		struct sockaddr_storage sa;
		int sock;
		int i;

#ifdef __linux
		socklen_t slen;
#else
		int slen;
#endif
		if (eXtl_tcp.proto_family == AF_INET)
			slen = sizeof(struct sockaddr_in);
		else
			slen = sizeof(struct sockaddr_in6);

		for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
			if (tcp_socket_tab[pos].socket == 0)
				break;
		}
		if (pos == EXOSIP_MAX_SOCKETS) {
			/* delete an old one! */
			pos = 0;
			if (tcp_socket_tab[pos].socket > 0) {
				_tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
			}
			memset(&tcp_socket_tab[pos], 0, sizeof(tcp_socket_tab[pos]));
		}
		
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO3, NULL,
							  "creating TCP socket at index: %i\n", pos));
		sock = accept(tcp_socket, (struct sockaddr *) &sa, &slen);
		if (sock < 0) {
#if defined(EBADF)
			int status = ex_errno;
#endif
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "Error accepting TCP socket\n"));
#if defined(EBADF)
			if (status==EBADF)
			{
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "Error accepting TCP socket: EBADF\n"));
				memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
				if (tcp_socket > 0)
					closesocket(tcp_socket);
				tcp_tl_open();
			}
#endif
		} else {
			tcp_socket_tab[pos].socket = sock;
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
								  "New TCP connection accepted\n"));

			memset(src6host, 0, sizeof(src6host));

			if (eXtl_tcp.proto_family == AF_INET)
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
					osip_strncpy(tcp_socket_tab[pos].remote_ip, src6host,
								 sizeof(tcp_socket_tab[pos].remote_ip) - 1);
					tcp_socket_tab[pos].remote_port = recvport;
				}
			}
#else
			i = getnameinfo((struct sockaddr *) &sa, slen,
							src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"Message received from: NULL:%i getnameinfo failure\n",
							recvport));
				snprintf(src6host, sizeof(src6host), "127.0.0.1");
			} else {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO1, NULL,
							"Message received from: %s:%i\n", src6host, recvport));
				osip_strncpy(tcp_socket_tab[pos].remote_ip, src6host,
							 sizeof(tcp_socket_tab[pos].remote_ip) - 1);
				tcp_socket_tab[pos].remote_port = recvport;
			}
#endif
		}
	}

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket > 0) {
			if (FD_ISSET(tcp_socket_tab[pos].socket, osip_wrset))
				_tcp_tl_send_sockinfo(&tcp_socket_tab[pos], NULL, 0);
			if (FD_ISSET(tcp_socket_tab[pos].socket, osip_fdset))
				_tcp_tl_recv(&tcp_socket_tab[pos]);
		}
	}

	return OSIP_SUCCESS;
}

static struct _tcp_sockets *_tcp_tl_find_sockinfo(int sock)
{
	int pos;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket == sock) {
			return &tcp_socket_tab[pos];
		}
	}
	return NULL;
}

static int _tcp_tl_find_socket(char *host, int port)
{
	int pos;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket != 0) {
			if (0 == osip_strcasecmp(tcp_socket_tab[pos].remote_ip, host)
				&& port == tcp_socket_tab[pos].remote_port)
				return pos;
		}
	}
	return -1;
}

static int _tcp_tl_is_connected(int sock)
{
	int res;
	struct timeval tv;
	fd_set wrset;
	int valopt;
	socklen_t sock_len;
	tv.tv_sec = SOCKET_TIMEOUT / 1000;
	tv.tv_usec = (SOCKET_TIMEOUT % 1000) * 1000;

	FD_ZERO(&wrset);
	FD_SET(sock, &wrset);

	res = select(sock + 1, NULL, &wrset, NULL, &tv);
	if (res > 0) {
		sock_len = sizeof(int);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &sock_len)
			== 0) {
			if (valopt) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot connect socket node / %s[%d]\n",
							strerror(ex_errno), ex_errno));
				return -1;
			} else {
				return 0;
			}
		} else {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Cannot connect socket node / error in getsockopt %s[%d]\n",
						strerror(ex_errno), ex_errno));
			return -1;
		}
	} else if (res < 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Cannot connect socket node / error in select %s[%d]\n",
					strerror(ex_errno), ex_errno));
		return -1;
	} else {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Cannot connect socket node / select timeout (%d ms)\n",
					SOCKET_TIMEOUT));
		return 1;
	}
}

static int _tcp_tl_check_connected()
{
	int pos;
	int res;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket > 0
			&& tcp_socket_tab[pos].ai_addrlen > 0) {
				res = connect(tcp_socket_tab[pos].socket, &tcp_socket_tab[pos].ai_addr, tcp_socket_tab[pos].ai_addrlen);
				if (res < 0) {
					int status = ex_errno;
#if defined(_WIN32_WCE) || defined(WIN32)
					if (status == WSAEISCONN) {
						tcp_socket_tab[pos].ai_addrlen=0; /* already connected */
						continue;
					}
#else
					if (status == EISCONN) {
						tcp_socket_tab[pos].ai_addrlen=0; /* already connected */
						continue;
					}
#endif
#if defined(_WIN32_WCE) || defined(WIN32)
					if (status != WSAEWOULDBLOCK && status != WSAEALREADY && status != WSAEINVAL) {
#else
					if (status != EINPROGRESS && status != EALREADY) {
#endif
						OSIP_TRACE(osip_trace
							(__FILE__, __LINE__, OSIP_INFO2, NULL,
							"_tcp_tl_check_connected: Cannot connect socket node:%s:%i, socket %d [pos=%d], family:%d, %s[%d]\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].remote_port,
							tcp_socket_tab[pos].socket,
							pos,
							tcp_socket_tab[pos].ai_addr.sa_family,
							strerror(status),
							status));
						_tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
						continue;
					} else {
						res = _tcp_tl_is_connected(tcp_socket_tab[pos].socket);
						if (res > 0) {
							OSIP_TRACE(osip_trace
								(__FILE__, __LINE__, OSIP_INFO2, NULL,
								"_tcp_tl_check_connected: socket node:%s:%i, socket %d [pos=%d], family:%d, in progress\n",
								tcp_socket_tab[pos].remote_ip,
								tcp_socket_tab[pos].remote_port,
								tcp_socket_tab[pos].socket,
								pos,
								tcp_socket_tab[pos].ai_addr.sa_family));
							continue;
						} else if (res == 0) {
							OSIP_TRACE(osip_trace
								(__FILE__, __LINE__, OSIP_INFO1, NULL,
								"_tcp_tl_check_connected: socket node:%s:%i , socket %d [pos=%d], family:%d, connected\n",
								tcp_socket_tab[pos].remote_ip,
								tcp_socket_tab[pos].remote_port,
								tcp_socket_tab[pos].socket,
								pos,
								tcp_socket_tab[pos].ai_addr.sa_family));
							/* stop calling "connect()" */
							tcp_socket_tab[pos].ai_addrlen=0;
							continue;
						} else {
							OSIP_TRACE(osip_trace
								(__FILE__, __LINE__, OSIP_INFO2, NULL,
								"_tcp_tl_check_connected: socket node:%s:%i, socket %d [pos=%d], family:%d, error\n",
								tcp_socket_tab[pos].remote_ip,
								tcp_socket_tab[pos].remote_port,
								tcp_socket_tab[pos].socket,
								pos,
								tcp_socket_tab[pos].ai_addr.sa_family));
							_tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
							continue;
						}
					}
				}
				else
				{
					OSIP_TRACE(osip_trace
						(__FILE__, __LINE__, OSIP_INFO1, NULL,
						"_tcp_tl_check_connected: socket node:%s:%i , socket %d [pos=%d], family:%d, connected (with connect)\n",
						tcp_socket_tab[pos].remote_ip,
						tcp_socket_tab[pos].remote_port,
						tcp_socket_tab[pos].socket,
						pos,
						tcp_socket_tab[pos].ai_addr.sa_family));
					/* stop calling "connect()" */
					tcp_socket_tab[pos].ai_addrlen=0;
				}
		}
	}
	return 0;
}

static int _tcp_tl_connect_socket(char *host, int port)
{
	int pos;
	int res;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *curinfo;
	int sock = -1;
	struct sockaddr selected_ai_addr;
	size_t selected_ai_addrlen;

	char src6host[NI_MAXHOST];
	memset(src6host, 0, sizeof(src6host));

	selected_ai_addrlen=0;
	memset(&selected_ai_addr, 0, sizeof(struct sockaddr));

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket == 0) {
			break;
		}
	}

	if (pos == EXOSIP_MAX_SOCKETS) {
	  OSIP_TRACE(osip_trace
		     (__FILE__, __LINE__, OSIP_ERROR, NULL,
		      "tcp_socket_tab is full - cannot create new socket!\n"));
#ifdef DELETE_OLD_SOCKETS
	  /* delete an old one! */
	  pos = 0;
	  if (tcp_socket_tab[pos].socket > 0) {
		  _tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
	  }
	  memset(&tcp_socket_tab[pos], 0, sizeof(tcp_socket_tab[pos]));
#else
	  return -1;
#endif
	}

	res = eXosip_get_addrinfo(&addrinfo, host, port, IPPROTO_TCP);
	if (res)
		return -1;


	for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next) {
		if (curinfo->ai_protocol && curinfo->ai_protocol != IPPROTO_TCP) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Skipping protocol %d\n", curinfo->ai_protocol));
			continue;
		}

		res =
			getnameinfo((struct sockaddr *) curinfo->ai_addr, curinfo->ai_addrlen,
						src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (res == 0) {
			int i = _tcp_tl_find_socket(src6host, port);
			if (i >= 0) {
				eXosip_freeaddrinfo(addrinfo);
				return i;
			}
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"New binding with %s:%i\n", src6host, port));
		}

		sock = (int) socket(curinfo->ai_family, curinfo->ai_socktype,
							curinfo->ai_protocol);
		if (sock < 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Cannot create socket %s!\n", strerror(ex_errno)));
			continue;
		}

		if (curinfo->ai_family == AF_INET6) {
#ifdef IPV6_V6ONLY
			if (setsockopt_ipv6only(sock)) {
				closesocket(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot set socket option %s!\n", strerror(ex_errno)));
				continue;
			}
#endif							/* IPV6_V6ONLY */
		}
		
		/* set NON-BLOCKING MODE */
#if defined(_WIN32_WCE) || defined(WIN32)
		{
			unsigned long nonBlock = 1;
			int val;

			ioctlsocket(sock, FIONBIO, &nonBlock);

			val = 1;
			if (setsockopt
				(sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &val,
				 sizeof(val)) == -1) {
				closesocket(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot get socket flag!\n"));
				continue;
			}
		}
#if !defined(_WIN32_WCE)
		{
			DWORD err = 0L;
			DWORD dwBytes = 0L;
			struct tcp_keepalive kalive = { 0 };
			struct tcp_keepalive kaliveOut = { 0 };
			kalive.onoff = 1;
			kalive.keepalivetime = 30000;	/* Keep Alive in 5.5 sec. */
			kalive.keepaliveinterval = 3000;	/* Resend if No-Reply */
			err = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive,
						   sizeof(kalive), &kaliveOut, sizeof(kaliveOut), &dwBytes,
						   NULL, NULL);
			if (err != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Cannot set keepalive interval!\n"));
			}
		}
#endif
#else
		{
			int val;

			val = fcntl(sock, F_GETFL);
			if (val < 0) {
				closesocket(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot get socket flag!\n"));
				continue;
			}
			val |= O_NONBLOCK;
			if (fcntl(sock, F_SETFL, val) < 0) {
				closesocket(sock);
				sock = -1;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot set socket flag!\n"));
				continue;
			}
#if 0
			val = 1;
			if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) ==
				-1)
				val = 30;		/* 30 sec before starting probes */
			setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val));
			val = 2;			/* 2 probes max */
			setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));
			val = 10;			/* 10 seconds between each probe */
			setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val));
#endif
#if SO_NOSIGPIPE
			val = 1;
			setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&val, sizeof(int));
#endif
		}
#endif

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"socket node:%s , socket %d, family:%d set to non blocking mode\n",
					host, sock, curinfo->ai_family));
		res = connect(sock, curinfo->ai_addr, curinfo->ai_addrlen);
		if (res < 0) {
#if defined(_WIN32_WCE) || defined(WIN32)
			if (ex_errno != WSAEWOULDBLOCK) {
#else
			if (ex_errno != EINPROGRESS) {
#endif
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"Cannot connect socket node:%s family:%d %s[%d]\n",
							host, curinfo->ai_family, strerror(ex_errno),
							ex_errno));
				closesocket(sock);
				sock = -1;
				continue;
			} else {
				res = _tcp_tl_is_connected(sock);
				if (res > 0) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"socket node:%s, socket %d [pos=%d], family:%d, in progress\n",
								host, sock, pos, curinfo->ai_family));
					selected_ai_addrlen = curinfo->ai_addrlen;
					memcpy(&selected_ai_addr, curinfo->ai_addr, sizeof(struct sockaddr));
					break;
				} else if (res == 0) {
#ifdef MULTITASKING_ENABLED
					tcp_socket_tab[pos].readStream = NULL;
					tcp_socket_tab[pos].writeStream = NULL;
					CFStreamCreatePairWithSocket(kCFAllocatorDefault, sock,
												 &tcp_socket_tab[pos].readStream, &tcp_socket_tab[pos].writeStream);
					if (tcp_socket_tab[pos].readStream!=NULL)
						CFReadStreamSetProperty(tcp_socket_tab[pos].readStream, kCFStreamNetworkServiceType, kCFStreamNetworkServiceTypeVoIP);
					if (tcp_socket_tab[pos].writeStream!=NULL)
						CFWriteStreamSetProperty(tcp_socket_tab[pos].writeStream, kCFStreamNetworkServiceType, kCFStreamNetworkServiceTypeVoIP);
					if (CFReadStreamOpen (tcp_socket_tab[pos].readStream))
					{ 
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_INFO1, NULL,
									"CFReadStreamOpen Succeeded!\n"));
					}
					
					CFWriteStreamOpen (tcp_socket_tab[pos].writeStream) ;
#endif		
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"socket node:%s , socket %d [pos=%d], family:%d, connected\n",
								host, sock, pos, curinfo->ai_family));
					break;
				} else {
					closesocket(sock);
					sock = -1;
					continue;
				}
			}
		}
	
		break;
	}

	eXosip_freeaddrinfo(addrinfo);

	if (sock > 0) {
		tcp_socket_tab[pos].socket = sock;

		tcp_socket_tab[pos].ai_addrlen = selected_ai_addrlen;
		memset(&tcp_socket_tab[pos].ai_addr, 0, sizeof(struct sockaddr));
		if (selected_ai_addrlen>0)
			memcpy(&tcp_socket_tab[pos].ai_addr, &selected_ai_addr, selected_ai_addrlen);

		if (src6host[0] == '\0')
			osip_strncpy(tcp_socket_tab[pos].remote_ip, host,
						 sizeof(tcp_socket_tab[pos].remote_ip) - 1);
		else
			osip_strncpy(tcp_socket_tab[pos].remote_ip, src6host,
						 sizeof(tcp_socket_tab[pos].remote_ip) - 1);

		tcp_socket_tab[pos].remote_port = port;


		return pos;
	}

	return -1;
}

static int
_tcp_tl_send_sockinfo (struct _tcp_sockets *sockinfo, const char *msg, int msglen)
{
	int i;
	while (1) {
		i = send(sockinfo->socket, (const void *) msg, msglen, 0);
		if (i < 0) {
			int status = ex_errno;
			if (is_wouldblock_error(status)) {
				struct timeval tv;
				fd_set wrset;
				tv.tv_sec = SOCKET_TIMEOUT / 1000;
				tv.tv_usec = (SOCKET_TIMEOUT % 1000) * 1000;
				if (tv.tv_usec==0)
					tv.tv_usec += 10000;

				FD_ZERO(&wrset);
				FD_SET(sockinfo->socket, &wrset);

				i = select(sockinfo->socket + 1, NULL, &wrset, NULL, &tv);
				if (i > 0) {
					continue;
				} else if (i < 0) {
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
										  "TCP select error: %s:%i\n",
										  strerror(ex_errno), ex_errno));
					return -1;
				} else {
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
										  "TCP timeout: %d ms\n", SOCKET_TIMEOUT));
					continue;
				}
			} else {
				/* SIP_NETWORK_ERROR; */
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "TCP error: %s\n", strerror(status)));
				return -1;
			}
		} else if (i == 0) {
			break; /* what's the meaning here? */
		} else if (i < msglen) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									"TCP partial write: wrote %i instead of %i\n", i, msglen));
			msglen -= i;
			msg += i;
			continue;
		}
		break;
	}
	return OSIP_SUCCESS;
}

static int
_tcp_tl_send (int sock, const char *msg, int msglen)
{
	struct _tcp_sockets *sockinfo = _tcp_tl_find_sockinfo (sock);
	if (sockinfo == NULL) {
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
								"could not find sockinfo for socket %d! dropping message\n", sock));
		return -1;
	}
	return _tcp_tl_send_sockinfo (sockinfo, msg, msglen);
}

static int
tcp_tl_send_message(osip_transaction_t * tr, osip_message_t * sip, char *host,
					int port, int out_socket)
{
	size_t length = 0;
	char *message = NULL;
	int i;
	int pos=-1;
	osip_naptr_t *naptr_record=NULL;

	if (host == NULL) {
		host = sip->req_uri->host;
		if (sip->req_uri->port != NULL)
			port = osip_atoi(sip->req_uri->port);
		else
			port = 5060;
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
			if (naptr_record->siptcp_record.name[0] != '\0'
				&& naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index].srv[0] != '\0') {
					/* always choose the first here.
					if a network error occur, remove first entry and
					replace with next entries.
					*/
					osip_srv_entry_t *srv;
					srv = &naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index];
					if (srv->ipaddress[0]) {
						host = srv->ipaddress;
						port = srv->port;
					}
					else {
						host = srv->srv;
						port = srv->port;
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
			if (naptr_record->siptcp_record.name[0] != '\0'
				&& naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index].srv[0] != '\0') {
					/* always choose the first here.
					if a network error occur, remove first entry and
					replace with next entries.
					*/
					osip_srv_entry_t *srv;
					srv = &naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index];
					if (srv->ipaddress[0]) {
						host = srv->ipaddress;
						port = srv->port;
					}
					else {
						host = srv->srv;
						port = srv->port;
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
		osip_free(message);
		return -1;
	}

	/* verify all current connections */
	_tcp_tl_check_connected();

	if (out_socket > 0) {
		for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
			if (tcp_socket_tab[pos].socket != 0) {
				if (tcp_socket_tab[pos].socket == out_socket) {
					out_socket = tcp_socket_tab[pos].socket;
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
										  "reusing REQUEST connection (to dest=%s:%i)\n",
										  tcp_socket_tab[pos].remote_ip,
										  tcp_socket_tab[pos].remote_port));
					break;
				}
			}
		}
		if (pos == EXOSIP_MAX_SOCKETS)
			out_socket = 0;
	}
	
	/* Step 1: find existing socket to send message */
	if (out_socket <= 0) {
		pos = _tcp_tl_find_socket(host, port);
		if (pos>=0)
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
									"reusing connection (to dest=%s:%i)\n",
									tcp_socket_tab[pos].remote_ip,
									tcp_socket_tab[pos].remote_port));
		}
		
		/* Step 2: create new socket with host:port */
		if (pos < 0) {
			pos = _tcp_tl_connect_socket(host, port);
		}
		if (pos>=0)
			out_socket = tcp_socket_tab[pos].socket;
	}
	

	if (out_socket <= 0) {
		osip_free(message);
		return -1;
	}

	i = _tcp_tl_is_connected(out_socket);
	if (i > 0) {
		time_t now;
		now = time(NULL);
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"socket node:%s, socket %d [pos=%d], in progress\n",
					host, out_socket, pos));
		osip_free(message);
		if (tr != NULL && now - tr->birth_time > 10 && now - tr->birth_time < 13)
		{
			/* avoid doing this twice... */
			if (naptr_record!=NULL && MSG_IS_REGISTER(sip))
			{
				if (eXosip_dnsutils_rotate_srv(&naptr_record->siptcp_record)>0)
				{
					OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						"Doing TCP failover: %s:%i->%s:%i\n",
						host, port,
						naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index].srv,
						naptr_record->siptcp_record.srventry[naptr_record->siptcp_record.index].port));
					return OSIP_SUCCESS + 1;	/* retry for next retransmission! */
				}
			}

			return -1;
		}
		return 1;
	} else if (i == 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"socket node:%s , socket %d [pos=%d], connected\n",
					host, out_socket, pos));
	} else {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_ERROR, NULL,
					"socket node:%s, socket %d [pos=%d], socket error\n",
					host, out_socket, pos));
		osip_free(message);
		return -1;
	}

	
#ifdef MULTITASKING_ENABLED
	
	if (pos>=0 && tcp_socket_tab[pos].readStream==NULL)
	{
		tcp_socket_tab[pos].readStream = NULL;
		tcp_socket_tab[pos].writeStream = NULL;
		CFStreamCreatePairWithSocket(kCFAllocatorDefault, out_socket,
									 &tcp_socket_tab[pos].readStream, &tcp_socket_tab[pos].writeStream);
		if (tcp_socket_tab[pos].readStream!=NULL)
			CFReadStreamSetProperty(tcp_socket_tab[pos].readStream, kCFStreamNetworkServiceType, kCFStreamNetworkServiceTypeVoIP);
		if (tcp_socket_tab[pos].writeStream!=NULL)
			CFWriteStreamSetProperty(tcp_socket_tab[pos].writeStream, kCFStreamNetworkServiceType, kCFStreamNetworkServiceTypeVoIP);
		if (CFReadStreamOpen (tcp_socket_tab[pos].readStream))
		{ 
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO1, NULL,
						"CFReadStreamOpen Succeeded!\n"));
		}
		
		CFWriteStreamOpen (tcp_socket_tab[pos].writeStream) ;
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO1, NULL,
					"socket node:%s:%i , socket %d [pos=%d], family:?, connected\n",
					tcp_socket_tab[pos].remote_ip,
					tcp_socket_tab[pos].remote_port,
					tcp_socket_tab[pos].socket, pos));
	}
#endif
	
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "Message sent: (to dest=%s:%i) \n%s\n",
						  host, port, message));
	i = _tcp_tl_send(out_socket, (const void *)message, length);
	osip_free(message);
	return i;
}

#ifdef ENABLE_KEEP_ALIVE_OPTIONS_METHOD
static int _tcp_tl_get_socket_info(int socket, char *host, int hostsize, int *port)
{
	struct sockaddr addr;
	int nameLen = sizeof(addr);
	int ret;
	if(socket <= 0 || host== NULL || hostsize <= 0 || port == NULL)
		return OSIP_BADPARAMETER;
	ret = getsockname(socket, &addr, &nameLen);
	if (ret != 0)
	{
		/* ret = ex_errno; */
		return OSIP_UNDEFINED_ERROR;
	} 
	else 
	{
		ret = getnameinfo((struct sockaddr *) &addr, nameLen,
						host, hostsize, NULL, 0, NI_NUMERICHOST);
		if (ret!=0)
			return OSIP_UNDEFINED_ERROR;

		if (addr.sa_family == AF_INET)
			(*port) = ntohs(((struct sockaddr_in *) &addr)->sin_port);
		else
			(*port) = ntohs(((struct sockaddr_in6 *) &addr)->sin6_port);
	}
	return OSIP_SUCCESS;
}
#endif

static int tcp_tl_keepalive(void)
{
	char buf[5] = "\r\n\r\n";
	int pos;
	int i;

	if (tcp_socket <= 0)
		return OSIP_UNDEFINED_ERROR;

	for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++) {
		if (tcp_socket_tab[pos].socket > 0) {
			i = _tcp_tl_is_connected(tcp_socket_tab[pos].socket);
			if (i > 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"tcp_tl_keepalive socket node:%s:%i, socket %d [pos=%d], in progress\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].remote_port,
							tcp_socket_tab[pos].socket, pos));
				continue;
			} else if (i == 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_INFO2, NULL,
							"tcp_tl_keepalive socket node:%s:%i , socket %d [pos=%d], connected\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].remote_port,
							tcp_socket_tab[pos].socket, pos));
			} else {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							"tcp_tl_keepalive socket node:%s:%i, socket %d [pos=%d], socket error\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].remote_port,
							tcp_socket_tab[pos].socket, pos));
				_tcp_tl_close_sockinfo(&tcp_socket_tab[pos]);
				continue;
			}
			if (eXosip.keep_alive > 0) {
#ifdef ENABLE_KEEP_ALIVE_OPTIONS_METHOD
				if (eXosip.keep_alive_options != 0)
				{
					osip_message_t *options;
					char from[NI_MAXHOST];
					char to[NI_MAXHOST];
					char locip[NI_MAXHOST];
					int locport;
					char *message;
					size_t length;

					options = NULL;
					memset(to, '\0', sizeof(to));
					memset(from, '\0', sizeof(from));
					memset(locip, '\0', sizeof(locip));
					locport = 0;

					snprintf(to, sizeof(to), "<sip:%s:%d>", tcp_socket_tab[pos].remote_ip, tcp_socket_tab[pos].remote_port);
					_tcp_tl_get_socket_info(tcp_socket_tab[pos].socket, locip, sizeof(locip), &locport);
					if (locip[0] == '\0')
					{
						OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							"tcp_tl_keepalive socket node:%s , socket %d [pos=%d], failed to create sip options message\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].socket, 
							pos));
						continue;
					}

					snprintf(from, sizeof(from), "<sip:%s:%d>", locip, locport);

					eXosip_lock();
					/* Generate an options message */
					if(eXosip_options_build_request(&options,to,from,NULL) == OSIP_SUCCESS)
					{
						message = NULL;
						length = 0;
						/* Convert message to str for direct sending over correct socket */
						if(osip_message_to_str( options, &message, &length ) == OSIP_SUCCESS)
						{
							OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								"tcp_tl_keepalive socket node:%s , socket %d [pos=%d], sending sip options\n\r%s",
								tcp_socket_tab[pos].remote_ip,
								tcp_socket_tab[pos].socket, 
								pos,
								message));
							i = send(tcp_socket_tab[pos].socket, (const void *) message, length, 0);
							osip_free(message);
							if(i > 0) {
								OSIP_TRACE(osip_trace
									(__FILE__, __LINE__, OSIP_INFO1, NULL,
									"eXosip: Keep Alive sent on TCP!\n"));
							}
						}
						else
						{
							OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
								"tcp_tl_keepalive socket node:%s , socket %d [pos=%d], failed to convert sip options message\n",
								tcp_socket_tab[pos].remote_ip,
								tcp_socket_tab[pos].socket, 
								pos));
						}
					}
					else
					{
						OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							"tcp_tl_keepalive socket node:%s , socket %d [pos=%d], failed to create sip options message\n",
							tcp_socket_tab[pos].remote_ip,
							tcp_socket_tab[pos].socket, 
							pos));
					}
					eXosip_unlock();
					continue;
				}
#endif
				i = send(tcp_socket_tab[pos].socket, (const void *) buf, 4, 0);
			}
		}
	}
	return OSIP_SUCCESS;
}

static int tcp_tl_set_socket(int socket)
{
	tcp_socket = socket;

	return OSIP_SUCCESS;
}

static int tcp_tl_masquerade_contact(const char *public_address, int port)
{
	if (public_address == NULL || public_address[0] == '\0') {
		memset(tcp_firewall_ip, '\0', sizeof(tcp_firewall_ip));
		memset(tcp_firewall_port, '\0', sizeof(tcp_firewall_port));
		if (eXtl_tcp.proto_port > 0)
			snprintf(tcp_firewall_port, sizeof(tcp_firewall_port), "%i",
					 eXtl_tcp.proto_port);
		return OSIP_SUCCESS;
	}
	snprintf(tcp_firewall_ip, sizeof(tcp_firewall_ip), "%s", public_address);
	if (port > 0) {
		snprintf(tcp_firewall_port, sizeof(tcp_firewall_port), "%i", port);
	}
	return OSIP_SUCCESS;
}

static int
tcp_tl_get_masquerade_contact(char *ip, int ip_size, char *port, int port_size)
{
	memset(ip, 0, ip_size);
	memset(port, 0, port_size);

	if (tcp_firewall_ip[0] != '\0')
		snprintf(ip, ip_size, "%s", tcp_firewall_ip);

	if (tcp_firewall_port[0] != '\0')
		snprintf(port, port_size, "%s", tcp_firewall_port);
	return OSIP_SUCCESS;
}

struct eXtl_protocol eXtl_tcp = {
	1,
	5060,
	"TCP",
	"0.0.0.0",
	IPPROTO_TCP,
	AF_INET,
	0,
	0,

	&tcp_tl_init,
	&tcp_tl_free,
	&tcp_tl_open,
	&tcp_tl_set_fdset,
	&tcp_tl_read_message,
	&tcp_tl_send_message,
	&tcp_tl_keepalive,
	&tcp_tl_set_socket,
	&tcp_tl_masquerade_contact,
	&tcp_tl_get_masquerade_contact
};
