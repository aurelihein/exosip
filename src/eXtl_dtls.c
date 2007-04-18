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

extern eXosip_t eXosip;

#if defined(_WIN32_WCE)
#define strerror(X) "-1"
#endif

static int dtls_socket;
static struct sockaddr_storage ai_addr;

static char dtls_firewall_ip[64];
static char dtls_firewall_port[10];

static int
dtls_tl_init(void)
{
  memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
  memset(dtls_firewall_ip, 0, sizeof(dtls_firewall_ip));
  memset(dtls_firewall_port, 0, sizeof(dtls_firewall_port));
  dtls_socket=0;
  return 0;
}

static int
dtls_tl_free(void)
{
  memset(dtls_firewall_ip, 0, sizeof(dtls_firewall_ip));
  memset(dtls_firewall_port, 0, sizeof(dtls_firewall_port));
  memset(&ai_addr, 0, sizeof(struct sockaddr_storage));
  if (dtls_socket>0)
    close(dtls_socket);

  return 0;
}

static int
dtls_tl_open(void)
{
  int res;
  struct addrinfo *addrinfo = NULL;
  struct addrinfo *curinfo;
  int sock = -1;

  if (eXtl_dtls.proto_port < 0)
    eXtl_dtls.proto_port = 5061;


  res = eXosip_get_addrinfo (&addrinfo,
			     eXtl_dtls.proto_ifs,
			     eXtl_dtls.proto_port,
			     eXtl_dtls.proto_num);
  if (res)
    return -1;

  for (curinfo = addrinfo; curinfo; curinfo = curinfo->ai_next)
    {
      socklen_t len;

      if (curinfo->ai_protocol && curinfo->ai_protocol != eXtl_dtls.proto_num)
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
                       eXtl_dtls.proto_ifs, curinfo->ai_family, strerror (errno)));
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

      if (eXtl_dtls.proto_num != IPPROTO_UDP)
        {
          res = listen (sock, SOMAXCONN);
          if (res < 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "eXosip: Cannot bind socket node:%s family:%d %s\n",
                           eXtl_dtls.proto_ifs, curinfo->ai_family, strerror (errno)));
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
                   "eXosip: Cannot bind on port: %i\n", eXtl_dtls.proto_port));
      return -1;
    }

  dtls_socket = sock;

  if (eXtl_dtls.proto_port == 0)
    {
      /* get port number from socket */
      if (eXtl_dtls.proto_family == AF_INET)
        eXtl_dtls.proto_port = ntohs (((struct sockaddr_in *) &ai_addr)->sin_port);
      else
        eXtl_dtls.proto_port = ntohs (((struct sockaddr_in6 *) &ai_addr)->sin6_port);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO1, NULL,
                   "eXosip: Binding on port %i!\n", eXtl_dtls.proto_port));
    }

  snprintf(dtls_firewall_port, sizeof(dtls_firewall_port), "%i", eXtl_dtls.proto_port);
  return 0;
}


static int
dtls_tl_set_fdset(fd_set *osip_fdset, int *fd_max)
{
  if (dtls_socket<=0)
    return -1;

  eXFD_SET (dtls_socket, osip_fdset);

  if (dtls_socket > *fd_max)
    *fd_max = dtls_socket;

  return 0;
}

static int
dtls_tl_read_message(fd_set *osip_fdset)
{
  char *buf;
  int i;

  if (dtls_socket<=0)
    return -1;
  
  if (FD_ISSET (dtls_socket, osip_fdset))
    {
      struct sockaddr_storage sa;
      
#ifdef __linux
      socklen_t slen;
#else
      int slen;
#endif
      if (eXtl_dtls.proto_family == AF_INET)
	slen = sizeof (struct sockaddr_in);
      else
	slen = sizeof (struct sockaddr_in6);
      
      buf = (char *) osip_malloc (SIP_MESSAGE_MAX_LENGTH * sizeof (char) + 1);
      if (buf==NULL)
	return -1;

      i = recvfrom (dtls_socket, buf,
		    SIP_MESSAGE_MAX_LENGTH, 0,
		    (struct sockaddr *) &sa, &slen);
      
      if (i > 5)
	{
	  char src6host[NI_MAXHOST];
	  int recvport = 0;
	  int err;
	  
	  osip_strncpy (buf + i, "\0", 1);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "Received message: \n%s\n", buf));

	  memset (src6host, 0, sizeof (src6host));
	  
	  if (eXtl_dtls.proto_family == AF_INET)
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
	      }
	  }
#else
	  err = getnameinfo ((struct sockaddr *) &sa, slen,
			     src6host, NI_MAXHOST,
			     NULL, 0, NI_NUMERICHOST);
	  
	  if (err != 0)
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
			   "Message received from: %s:%i\n",
			   src6host, recvport));
	    }
#endif
	  
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO1, NULL,
		       "Message received from: %s:%i\n",
		       src6host, recvport));
	  
	  
	  _eXosip_handle_incoming_message(buf, i, dtls_socket, src6host, recvport);
      
	}
#ifndef MINISIZE
      else if (i < 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "Could not read socket\n"));
	}
      else
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "Dummy SIP message received\n"));
	}
#endif
  
      osip_free (buf);
    }

  return 0;
}

#ifdef INET6_ADDRSTRLEN
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

  if (dtls_socket <= 0)
    return -1;

  if (host == NULL)
    {
      host = sip->req_uri->host;
      if (sip->req_uri->port != NULL)
        port = osip_atoi (sip->req_uri->port);
      else
        port = 5061;
    }
  if (port==5060)
    port=5061;

  i=-1;
#ifndef MINISIZE
  if (tr!=NULL && tr->record.name[0]!='\0' && tr->record.srventry[0].srv[0]!='\0')
    {
      /* always choose the first here.
	 if a network error occur, remove first entry and
	 replace with next entries.
      */
      osip_srv_entry_t *srv;
      int n=0;
      for (srv = &tr->record.srventry[0];
	   n<10 && tr->record.srventry[0].srv[0];
	   srv = &tr->record.srventry[0])
	{
	  i = eXosip_get_addrinfo (&addrinfo, srv->srv, srv->port, IPPROTO_UDP);
	  if (i == 0)
	    {
	      host = srv->srv;
	      port = srv->port;
	      break;
	    }
	  memmove(&tr->record.srventry[0], &tr->record.srventry[1], 9*sizeof(osip_srv_entry_t));
	  memset(&tr->record.srventry[9], 0, sizeof(osip_srv_entry_t));
	  i=-1;
	  /* copy next element */
	  n++;
	}
    }
#endif
  
  /* if SRV was used, distination may be already found */
  if (i != 0)
    {
      i = eXosip_get_addrinfo (&addrinfo, host, port, IPPROTO_UDP);
    }
  
  if (i != 0)
    {
      return -1;
    }
  
  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  len = addrinfo->ai_addrlen;
  
  eXosip_freeaddrinfo (addrinfo);
  
  /* remove preloaded route if there is no tag in the To header
   */
  {
    osip_route_t *route=NULL;
    osip_generic_param_t *tag=NULL;
    osip_message_get_route (sip, 0, &route);
    
    osip_to_get_tag (sip->to, &tag);
    if (tag==NULL && route != NULL && route->url != NULL)
      {
	osip_list_remove(&sip->routes, 0);
      }
    i = osip_message_to_str (sip, &message, &length);
    if (tag==NULL && route != NULL && route->url != NULL)
      {
	osip_list_add(&sip->routes, route, 0);
      }
  }
  
  if (i != 0 || length <= 0)
    {
      return -1;
    }
  
  switch ( ((struct sockaddr *)&addr)->sa_family )
    {
    case AF_INET:
      inet_ntop (((struct sockaddr *)&addr)->sa_family, &(((struct sockaddr_in *) &addr)->sin_addr),
		 ipbuf, sizeof (ipbuf));
      break;
    case AF_INET6:
      inet_ntop (((struct sockaddr *)&addr)->sa_family,
		 &(((struct sockaddr_in6 *) &addr)->sin6_addr), ipbuf,
		 sizeof (ipbuf));
      break;
    default:
      strncpy (ipbuf, "(unknown)", sizeof (ipbuf));
      break;
    }
  
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                          "Message sent: \n%s (to dest=%s:%i)\n",
                          message, ipbuf, port));
  if (0 >
      sendto (dtls_socket, (const void *) message, length, 0,
	      (struct sockaddr *) &addr, len))
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
	}
      else
	{
	  
#ifndef MINISIZE
	  /* delete first SRV entry that is not reachable */
	  if (tr->record.name[0]!='\0' && tr->record.srventry[0].srv[0]!='\0')
	    {
	      memmove(&tr->record.srventry[0], &tr->record.srventry[1], 9*sizeof(osip_srv_entry_t));
	      memset(&tr->record.srventry[9], 0, sizeof(osip_srv_entry_t));
	      osip_free (message);
	      return 0; /* retry for next retransmission! */
	    }
#endif
	  /* SIP_NETWORK_ERROR; */
	  osip_free (message);
          return -1;
	}
    }

  if (eXosip.keep_alive > 0)
    {
      if (MSG_IS_REGISTER (sip))
        {
          eXosip_reg_t *reg = NULL;
	  
          if (_eXosip_reg_find (&reg, tr) == 0)
            {
              memcpy (&(reg->addr), &addr, len);
              reg->len = len;
            }
        }
    }
  
  osip_free (message);
  return 0;
}

static int
dtls_tl_keepalive(void)
{
  return 0;
}

static int
dtls_tl_set_socket(int socket)
{
  dtls_socket = socket;
  
  return 0;
}

static int
dtls_tl_masquerade_contact(const char *public_address, int port)
{
  if (public_address == NULL || public_address[0] == '\0')
    {
      memset (dtls_firewall_ip, '\0', sizeof (dtls_firewall_ip));
      return 0;
    }
  snprintf (dtls_firewall_ip, sizeof (dtls_firewall_ip), "%s", public_address);
  if (port > 0)
    {
      snprintf (dtls_firewall_port, sizeof(dtls_firewall_port), "%i", port);
    }
  return 0;
}

static int
dtls_tl_get_masquerade_contact(char *ip, int ip_size, char *port, int port_size)
{
  memset(ip, 0, ip_size);
  memset(port, 0, port_size);

  if (dtls_firewall_ip!='\0')
    snprintf(ip, ip_size, "%s", dtls_firewall_ip);
  
  if (dtls_firewall_port!='\0')
    snprintf(port, port_size, "%s", dtls_firewall_port);
  return 0;
}

struct eXtl_protocol eXtl_dtls = 
  {
    1,
    5061,
    "DTLS",
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
