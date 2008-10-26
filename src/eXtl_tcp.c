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

static int tcp_socket;
static struct sockaddr_storage ai_addr;

static char tcp_firewall_ip[64];
static char tcp_firewall_port[10];

/* persistent connection */
struct socket_tab
{
  int socket;
  char remote_ip[65];
  int remote_port;
};

#ifndef EXOSIP_MAX_SOCKETS
#define EXOSIP_MAX_SOCKETS 100
#endif

static struct socket_tab tcp_socket_tab[EXOSIP_MAX_SOCKETS];

static int
tcp_tl_init (void)
{
  tcp_socket = 0;
  memset (&ai_addr, 0, sizeof (struct sockaddr_storage));
  memset (&tcp_socket_tab, 0, sizeof (struct socket_tab) * EXOSIP_MAX_SOCKETS);
  memset (tcp_firewall_ip, 0, sizeof (tcp_firewall_ip));
  memset (tcp_firewall_port, 0, sizeof (tcp_firewall_port));
  return OSIP_SUCCESS;
}

static int
tcp_tl_free (void)
{
  int pos;
  memset (tcp_firewall_ip, 0, sizeof (tcp_firewall_ip));
  memset (tcp_firewall_port, 0, sizeof (tcp_firewall_port));
  memset (&ai_addr, 0, sizeof (struct sockaddr_storage));
  if (tcp_socket > 0)
    close (tcp_socket);

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tcp_socket_tab[pos].socket > 0)
        {
          close (tcp_socket_tab[pos].socket);
        }
    }
  memset (&tcp_socket_tab, 0, sizeof (struct socket_tab) * EXOSIP_MAX_SOCKETS);
  return OSIP_SUCCESS;
}

static int
tcp_tl_open (void)
{
  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  if (eXtl_tcp.proto_port < 0)
    eXtl_tcp.proto_port = 5060;


  res = eXosip_get_addrinfo (&addrinfo,
                             eXtl_tcp.proto_ifs,
                             eXtl_tcp.proto_port, eXtl_tcp.proto_num);
  if (res)
    return -1;

  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      socklen_t len;

      if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_tcp.proto_num)
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
                       eXtl_tcp.proto_ifs, curinfo->ai_family, strerror (errno)));
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

      if (eXtl_tcp.proto_num == IPPROTO_TCP)
        {
          res = listen (sock, SOMAXCONN);
          if (res < 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot bind socket node:%s family:%d %s\n",
                           eXtl_tcp.proto_ifs, curinfo->ai_family,
                           strerror (errno)));
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
                   "eXosip: Cannot bind on port: %i\n", eXtl_tcp.proto_port));
      return -1;
    }

  tcp_socket = sock;

  if (eXtl_tcp.proto_port == 0)
    {
      /* get port number from socket */
      if (eXtl_tcp.proto_family == AF_INET)
        eXtl_tcp.proto_port = ntohs (((struct sockaddr_in *) &ai_addr)->sin_port);
      else
        eXtl_tcp.proto_port =
          ntohs (((struct sockaddr_in6 *) &ai_addr)->sin6_port);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "eXosip: Binding on port %i!\n", eXtl_tcp.proto_port));
    }

  snprintf (tcp_firewall_port, sizeof (tcp_firewall_port), "%i",
            eXtl_tcp.proto_port);
  return OSIP_SUCCESS;
}

static int
tcp_tl_set_fdset (fd_set * osip_fdset, int *fd_max)
{
  int pos;
  if (tcp_socket <= 0)
    return -1;

  eXFD_SET (tcp_socket, osip_fdset);

  if (tcp_socket > *fd_max)
    *fd_max = tcp_socket;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tcp_socket_tab[pos].socket > 0)
        {
          eXFD_SET (tcp_socket_tab[pos].socket, osip_fdset);
          if (tcp_socket_tab[pos].socket > *fd_max)
            *fd_max = tcp_socket_tab[pos].socket;
        }
    }

  return OSIP_SUCCESS;
}

static int
tcp_tl_read_message (fd_set * osip_fdset)
{
  int pos = 0;
  char *buf;

  if (FD_ISSET (tcp_socket, osip_fdset))
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
      if (eXtl_tcp.proto_family == AF_INET)
        slen = sizeof (struct sockaddr_in);
      else
        slen = sizeof (struct sockaddr_in6);

      for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
        {
          if (tcp_socket_tab[pos].socket == 0)
            break;
        }
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
                              "creating TCP socket at index: %i\n", pos));
      sock = accept (tcp_socket, (struct sockaddr *) &sa, &slen);
      if (sock < 0)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "Error accepting TCP socket\n"));
      } else
        {
          tcp_socket_tab[pos].socket = sock;
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                  "New TCP connection accepted\n"));

          memset (src6host, 0, sizeof (src6host));

          if (eXtl_tcp.proto_family == AF_INET)
            recvport = ntohs (((struct sockaddr_in *) &sa)->sin_port);
          else
            recvport = ntohs (((struct sockaddr_in6 *) &sa)->sin6_port);

#if defined(__arc__)
          {
            struct sockaddr_in *fromsa = (struct sockaddr_in *) &sa;
            char *tmp;
            tmp = inet_ntoa (fromsa->sin_addr);
            if (tmp == NULL)
              {
                OSIP_TRACE (osip_trace
                            (__FILE__, __LINE__, OSIP_ERROR, NULL,
                             "Message received from: NULL:%i inet_ntoa failure\n",
                             recvport));
            } else
              {
                snprintf (src6host, sizeof (src6host), "%s", tmp);
                OSIP_TRACE (osip_trace
                            (__FILE__, __LINE__, OSIP_INFO1, NULL,
                             "Message received from: %s:%i\n", src6host,
                             recvport));
                osip_strncpy (tcp_socket_tab[pos].remote_ip, src6host,
                              sizeof (tcp_socket_tab[pos].remote_ip) - 1);
                tcp_socket_tab[pos].remote_port = recvport;
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
              snprintf (src6host, sizeof (src6host), "127.0.0.1");
          } else
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO1, NULL,
                           "Message received from: %s:%i\n", src6host, recvport));
              osip_strncpy (tcp_socket_tab[pos].remote_ip, src6host,
                            sizeof (tcp_socket_tab[pos].remote_ip) - 1);
              tcp_socket_tab[pos].remote_port = recvport;
            }
#endif
        }
    }



  buf = NULL;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tcp_socket_tab[pos].socket > 0
          && FD_ISSET (tcp_socket_tab[pos].socket, osip_fdset))
        {
          int i;

          if (buf == NULL)
            buf =
              (char *) osip_malloc (SIP_MESSAGE_MAX_LENGTH * sizeof (char) + 1);
          if (buf == NULL)
            return OSIP_NOMEM;

          i = recv (tcp_socket_tab[pos].socket, buf, SIP_MESSAGE_MAX_LENGTH, 0);
          if (i > 5)
            {
              osip_strncpy (buf + i, "\0", 1);
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO1, NULL,
                           "Received TCP message: \n%s\n", buf));
              _eXosip_handle_incoming_message (buf, i,
                                               tcp_socket_tab[pos].socket,
                                               tcp_socket_tab[pos].remote_ip,
                                               tcp_socket_tab[pos].remote_port);
          } else if (i < 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "Could not read socket - close it\n"));
              close (tcp_socket_tab[pos].socket);
              memset (&(tcp_socket_tab[pos]), 0, sizeof (tcp_socket_tab[pos]));
          } else if (i == 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO1, NULL,
                           "End of stream (read 0 byte from %s:%i)\n",
                           tcp_socket_tab[pos].remote_ip,
                           tcp_socket_tab[pos].remote_port));
              close (tcp_socket_tab[pos].socket);
              memset (&(tcp_socket_tab[pos]), 0, sizeof (tcp_socket_tab[pos]));
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

  if (buf != NULL)
    osip_free (buf);

  return OSIP_SUCCESS;
}

static int
_tcp_tl_find_socket (char *host, int port)
{
  int pos;

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tcp_socket_tab[pos].socket != 0)
        {
          if (0 == osip_strcasecmp (tcp_socket_tab[pos].remote_ip, host)
              && port == tcp_socket_tab[pos].remote_port)
            return tcp_socket_tab[pos].socket;
        }
    }
  return -1;
}

static int
_tcp_tl_connect_socket (char *host, int port)
{
  int pos;

  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  char src6host[NI_MAXHOST];
  memset (src6host, 0, sizeof (src6host));

  for (pos = 0; pos < EXOSIP_MAX_SOCKETS; pos++)
    {
      if (tcp_socket_tab[pos].socket == 0)
        {
          break;
        }
    }

  if (pos == EXOSIP_MAX_SOCKETS)
    return -1;

  res = eXosip_get_addrinfo (&addrinfo, host, port, IPPROTO_TCP);
  if (res)
    return -1;


  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      if (curinfo->ai_protocol && curinfo->ai_protocol != IPPROTO_TCP)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Skipping protocol %d\n", curinfo->ai_protocol));
          continue;
        }

      res = getnameinfo ((struct sockaddr *) curinfo->ai_addr, curinfo->ai_addrlen,
                         src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

      if (res == 0)
        {
          int i = _tcp_tl_find_socket (src6host, port);
          if (i >= 0)
            {
              eXosip_freeaddrinfo (addrinfo);
              return i;
            }
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "New binding with %s\n", src6host));
        }

      sock = (int) socket (curinfo->ai_family, curinfo->ai_socktype,
                           curinfo->ai_protocol);
      if (sock < 0)
        {
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot create socket!\n"));
#else
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot create socket %s!\n", strerror (errno)));
#endif
          continue;
        }

      if (curinfo->ai_family == AF_INET6)
        {
#ifdef IPV6_V6ONLY
          if (setsockopt_ipv6only (sock))
            {
              close (sock);
              sock = -1;
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "eXosip: Cannot set socket option!\n"));
#else
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_INFO2, NULL,
                           "eXosip: Cannot set socket option %s!\n",
                           strerror (errno)));
#endif
              continue;
            }
#endif /* IPV6_V6ONLY */
        }

      res = connect (sock, curinfo->ai_addr, curinfo->ai_addrlen);
      if (res < 0)
        {
#if !defined(OSIP_MT) || defined(_WIN32_WCE)
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d\n",
                       host, curinfo->ai_family));
#else
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_INFO2, NULL,
                       "eXosip: Cannot bind socket node:%s family:%d %s\n",
                       host, curinfo->ai_family, strerror (errno)));
#endif
          close (sock);
          sock = -1;
          continue;
        }

      break;
    }

  eXosip_freeaddrinfo (addrinfo);

  if (sock > 0)
    {
      tcp_socket_tab[pos].socket = sock;

      if (src6host[0] == '\0')
        osip_strncpy (tcp_socket_tab[pos].remote_ip, host,
                      sizeof (tcp_socket_tab[pos].remote_ip) - 1);
      else
        osip_strncpy (tcp_socket_tab[pos].remote_ip, src6host,
                      sizeof (tcp_socket_tab[pos].remote_ip) - 1);

      tcp_socket_tab[pos].remote_port = port;
      return sock;
    }

  return -1;
}

static int
tcp_tl_send_message (osip_transaction_t * tr, osip_message_t * sip, char *host,
                     int port, int out_socket)
{
  size_t length = 0;
  char *message;
  int i;

  if (host == NULL)
    {
      host = sip->req_uri->host;
      if (sip->req_uri->port != NULL)
        port = osip_atoi (sip->req_uri->port);
      else
        port = 5060;
    }

  /* remove preloaded route if there is no tag in the To header
   */
  {
    osip_route_t *route = NULL;
    osip_generic_param_t *tag = NULL;
    osip_message_get_route (sip, 0, &route);

    osip_to_get_tag (sip->to, &tag);
    if (tag == NULL && route != NULL && route->url != NULL)
      {
        osip_list_remove (&sip->routes, 0);
      }
    i = osip_message_to_str (sip, &message, &length);
    if (tag == NULL && route != NULL && route->url != NULL)
      {
        osip_list_add (&sip->routes, route, 0);
      }
  }

  if (i != 0 || length <= 0)
    {
      return -1;
    }

  /* Step 1: find existing socket to send message */
  if (out_socket <= 0)
    {
      out_socket = _tcp_tl_find_socket (host, port);

      /* Step 2: create new socket with host:port */
      if (out_socket <= 0)
        {
          out_socket = _tcp_tl_connect_socket (host, port);
        }

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                              "Message sent: \n%s (to dest=%s:%i)\n",
                              message, host, port));
  } else
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                              "Message sent: \n%s (reusing REQUEST connection)\n",
                              message));
    }

  if (out_socket <= 0)
    {
      return -1;
    }


  if (0 > send (out_socket, (const void *) message, length, 0))
    {
#ifdef WIN32
      if (WSAECONNREFUSED == WSAGetLastError ())
#else
      if (ECONNREFUSED == errno)
#endif
        {
          /* This can be considered as an error, but for the moment,
             I prefer that the application continue to try sending
             message again and again... so we are not in a error case.
             Nevertheless, this error should be announced!
             ALSO, UAS may not have any other options than retry always
             on the same port.
           */
          osip_free (message);
          return 1;
      } else
        {
          /* SIP_NETWORK_ERROR; */
#if !defined(_WIN32_WCE)
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                                  "TCP error: \n%s\n", strerror (errno)));
#endif
          osip_free (message);
          return -1;
        }
    }

  osip_free (message);
  return OSIP_SUCCESS;
}

static int
tcp_tl_keepalive (void)
{
  return OSIP_SUCCESS;
}

static int
tcp_tl_set_socket (int socket)
{
  tcp_socket = socket;

  return OSIP_SUCCESS;
}

static int
tcp_tl_masquerade_contact (const char *public_address, int port)
{
  if (public_address == NULL || public_address[0] == '\0')
    {
      memset (tcp_firewall_ip, '\0', sizeof (tcp_firewall_ip));
      return OSIP_SUCCESS;
    }
  snprintf (tcp_firewall_ip, sizeof (tcp_firewall_ip), "%s", public_address);
  if (port > 0)
    {
      snprintf (tcp_firewall_port, sizeof (tcp_firewall_port), "%i", port);
    }
  return OSIP_SUCCESS;
}

static int
tcp_tl_get_masquerade_contact (char *ip, int ip_size, char *port, int port_size)
{
  memset (ip, 0, ip_size);
  memset (port, 0, port_size);

  if (tcp_firewall_ip != '\0')
    snprintf (ip, ip_size, "%s", tcp_firewall_ip);

  if (tcp_firewall_port != '\0')
    snprintf (port, port_size, "%s", tcp_firewall_port);
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
