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

#if defined(_WIN32_WCE)
#define strerror(X) "-1"
#endif

static int tls_socket;
static struct sockaddr_storage ai_addr;

static char tls_firewall_ip[64];
static char tls_firewall_port[10];

/* persistent connection */
struct socket_tab
{
  int socket;
  char remote_ip[50];
  int remote_port;
};

#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS
#endif

static struct socket_tab tls_socket_tab[EXOSIP_MAX_SOCKETS];

static int
tls_tl_init(void)
{
  tls_socket=0;
  memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
  memset(&tls_socket_tab, 0, sizeof(struct socket_tab)*EXOSIP_MAX_SOCKETS);
  memset(tls_firewall_ip, 0, sizeof(tls_firewall_ip));
  memset(tls_firewall_port, 0, sizeof(tls_firewall_port));
  return 0;
}

static int
tls_tl_free(void)
{
  int pos;
  memset(tls_firewall_ip, 0, sizeof(tls_firewall_ip));
  memset(tls_firewall_port, 0, sizeof(tls_firewall_port));
  memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
  if (tls_socket>0)
    close(tls_socket);

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0)
	{
	  close(tls_socket_tab[pos].socket);
	}
    }
  memset(&tls_socket_tab, 0, sizeof(struct socket_tab)*EXOSIP_MAX_SOCKETS);
  return 0;
}

static int
tls_tl_open(void)
{
  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  if (eXtl_tls.proto_port < 0)
    eXtl_tls.proto_port = 5060;


  res = eXosip_get_addrinfo (&addrinfo,
			     eXtl_tls.proto_ifs,
			     eXtl_tls.proto_port,
			     eXtl_tls.proto_num);
  if (res)
    return -1;

  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      socklen_t len;

      if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_tls.proto_num)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO3, NULL,
                       "eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
          continue;
        }

      sock = (int) socket (curinfo->ai_family, curinfo->ai_socktype,
                           curinfo->ai_protocol);
      if (sock < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot create socket %s!\n", strerror (errno)));
          continue;
        }

      if (curinfo->ai_family == AF_INET6)
        {
#ifdef IPV6_V6ONLY
          if (setsockopt_ipv6only (sock))
            {
              close (sock);
              sock = -1;
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot set socket option %s!\n",
                           strerror (errno)));
              continue;
            }
#endif /* IPV6_V6ONLY */
        }

      res = bind (sock, curinfo->ai_addr, curinfo->ai_addrlen);
      if (res < 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d %s\n",
                       eXtl_tls.proto_ifs, curinfo->ai_family, strerror (errno)));
          close (sock);
          sock = -1;
          continue;
        }
      len = sizeof (ai_addr);
      res = getsockname (sock, (struct sockaddr *) &ai_addr, &len);
      if (res != 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: Cannot get socket name (%s)\n", strerror (errno)));
          memcpy (&ai_addr, curinfo->ai_addr, curinfo->ai_addrlen);
        }

      if (eXtl_tls.proto_num == IPPROTO_TCP)
        {
          res = listen (sock, SOMAXCONN);
          if (res < 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot bind socket node:%s family:%d %s\n",
                           eXtl_tls.proto_ifs, curinfo->ai_family, strerror (errno)));
              close (sock);
              sock = -1;
              continue;
            }
        }

      break;
    }

  eXosip_freeaddrinfo (addrinfo);

  if (sock < 0)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: Cannot bind on port: %i\n", eXtl_tls.proto_port));
      return -1;
    }

  tls_socket = sock;

  if (eXtl_tls.proto_port == 0)
    {
      /* get port number from socket */
      if (eXtl_tls.proto_family == AF_INET)
        eXtl_tls.proto_port = ntohs (((struct sockaddr_in *) &ai_addr)->sin_port);
      else
        eXtl_tls.proto_port = ntohs (((struct sockaddr_in6 *) &ai_addr)->sin6_port);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "eXosip: Binding on port %i!\n", eXtl_tls.proto_port));
    }

  return 0;
}

static int
tls_tl_set_fdset(fd_set *osip_fdset, int *fd_max)
{
  int pos;
  if (tls_socket<=0)
    return -1;

  eXFD_SET (tls_socket, osip_fdset);

  if (tls_socket > *fd_max)
    *fd_max = tls_socket;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0)
	{
	  eXFD_SET (tls_socket_tab[pos].socket, osip_fdset);
	  if (tls_socket_tab[pos].socket > *fd_max)
	    *fd_max = tls_socket_tab[pos].socket;
	}
    }

  return 0;
}

static int
tls_tl_read_message(fd_set *osip_fdset)
{
  int pos = 0;
  char *buf;

  if (FD_ISSET (tls_socket, osip_fdset))
    {
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
      if (eXtl_tls.proto_family == AF_INET)
	slen = sizeof (struct sockaddr_in);
      else
	slen = sizeof (struct sockaddr_in6);

      for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
	{
	  if (tls_socket_tab[pos].socket == 0)
	    break;
	}
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			      "creating TLS socket at index: %i\n", pos));
      sock = accept (tls_socket,
		     (struct sockaddr *) &sa, &slen);
      if (sock < 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "Error accepting TLS socket\n"));
	}
      else
	{
	  tls_socket_tab[pos].socket = sock;
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "New TLS connection accepted\n"));
	  
	  memset (src6host, 0, sizeof (src6host));
	  
	  if (eXtl_tls.proto_family == AF_INET)
	    recvport = ntohs (((struct sockaddr_in *) &sa)->sin_port);
	  else
	    recvport = ntohs (((struct sockaddr_in6 *) &sa)->sin6_port);
	  
#if defined(__arc__)
	  {
	    struct sockaddr_in *fromsa = (struct sockaddr_in *) &sa;
	    char *tmp;
	    tmp = inet_ntoa(fromsa->sin_addr);
	    if (tmp==NULL)
	      {
		OSIP_TRACE (osip_trace
			    (__FILE__, __LINE__, OSIP_ERROR, NULL,
			     "Message received from: NULL:%i inet_ntoa failure\n",
			     recvport));
	      }
	    else
	      {
		snprintf(src6host, sizeof(src6host), "%s", tmp);
		OSIP_TRACE (osip_trace
			    (__FILE__, __LINE__, OSIP_INFO1, NULL,
			     "Message received from: %s:%i\n", src6host, recvport));
		osip_strncpy (tls_socket_tab[pos].remote_ip, src6host,
			      sizeof (tls_socket_tab[pos].remote_ip));
		tls_socket_tab[pos].remote_port = recvport;
	      }
	  }
#else
	  i = getnameinfo ((struct sockaddr *) &sa, slen,
			   src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	  
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "Message received from: NULL:%i getnameinfo failure\n",
			   recvport));
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			   "Message received from: %s:%i\n", src6host, recvport));
	      osip_strncpy (tls_socket_tab[pos].remote_ip, src6host,
			    sizeof (tls_socket_tab[pos].remote_ip));
	      tls_socket_tab[pos].remote_port = recvport;
	    }
#endif
	} 
    }



  buf=NULL;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tls_socket_tab[pos].socket > 0
	  && FD_ISSET (tls_socket_tab[pos].socket, osip_fdset))
	{
	  int i;

	  if (buf==NULL)
	    buf = (char *) osip_malloc (SIP_MESSAGE_MAX_LENGTH * sizeof (char) + 1);
	  if (buf==NULL)
	    return -1;

	  i = recv (tls_socket_tab[pos].socket, buf, SIP_MESSAGE_MAX_LENGTH, 0);
	  if (i > 5)
	    {
	      osip_strncpy (buf + i, "\0", 1);
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			   "Received TLS message: \n%s\n", buf));
	      _eXosip_handle_incoming_message (buf, i,
					       tls_socket_tab[pos].socket,
					       tls_socket_tab[pos].remote_ip,
					       tls_socket_tab[pos].remote_port);
	    }
	  else if (i < 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "Could not read socket - close it\n"));
	      close (tls_socket_tab[pos].socket);
	      memset (&(tls_socket_tab[pos]), 0, sizeof (tls_socket_tab[pos]));
	    }
	  else if (i == 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			   "End of stream (read 0 byte from %s:%i)\n",
			   tls_socket_tab[pos].remote_ip,
			   tls_socket_tab[pos].remote_port));
	      close (tls_socket_tab[pos].socket);
	      memset (&(tls_socket_tab[pos]), 0, sizeof (tls_socket_tab[pos]));
	    }
#ifndef MINISIZE
	  else
	    {
	      /* we expect at least one byte, otherwise there's no doubt that it is not a sip message ! */
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			   "Dummy SIP message received (size=%i)\n", i));
	    }
#endif
	}
    }

  if (buf!=NULL)
    osip_free (buf);

  return 0;
}

static int
tls_tl_send_message(osip_transaction_t * tr, osip_message_t * sip, char *host,
                    int port, int out_socket)
{
  return 0;
}

static int
tls_tl_keepalive(void)
{
  return 0;
}

static int
tls_tl_set_socket(int socket)
{
  tls_socket = socket;
  
  return 0;
}

static int
tls_tl_masquerade_contact(const char *public_address, int port)
{
  if (public_address == NULL || public_address[0] == '\0')
    {
      memset (tls_firewall_ip, '\0', sizeof (tls_firewall_ip));
      return 0;
    }
  snprintf (tls_firewall_ip, sizeof (tls_firewall_ip), "%s", public_address);
  if (port > 0)
    {
      snprintf (tls_firewall_port, sizeof(tls_firewall_port), "%i", port);
    }
  return 0;
}

struct eXtl_protocol eXtl_tls = 
  {
    1,
    5060,
    "TLS",
    "0.0.0.0",
    IPPROTO_TCP,
    AF_INET,
    0,
    0,
    
    &tls_tl_init,
    &tls_tl_free,
    &tls_tl_open,
    &tls_tl_set_fdset,
    &tls_tl_read_message,
    &tls_tl_send_message,
    &tls_tl_keepalive,
    &tls_tl_set_socket,
    &tls_tl_masquerade_contact,
    NULL
};
