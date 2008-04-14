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

extern eXosip_t eXosip;

char *
_eXosip_transport_protocol (osip_message_t * msg)
{
  osip_via_t *via;

  via = (osip_via_t *) osip_list_get (&msg->vias, 0);
  if (via == NULL || via->protocol == NULL)
    return NULL;
  return via->protocol;
}

int
_eXosip_find_protocol (osip_message_t * msg)
{
  osip_via_t *via;

  via = (osip_via_t *) osip_list_get (&msg->vias, 0);
  if (via == NULL || via->protocol == NULL)
    return -1;
  else if (0 == osip_strcasecmp (via->protocol, "UDP"))
    return IPPROTO_UDP;
  else if (0 == osip_strcasecmp (via->protocol, "TCP"))
    return IPPROTO_TCP;
  return -1;;
}


int
eXosip_transport_set (osip_message_t * msg, const char *transport)
{
  osip_via_t *via;

  via = (osip_via_t *) osip_list_get (&msg->vias, 0);
  if (via == NULL || via->protocol == NULL)
    return -1;

  if (0 == osip_strcasecmp (via->protocol, transport))
    return OSIP_SUCCESS;

  osip_free (via->protocol);
  via->protocol = osip_strdup (transport);
  return OSIP_SUCCESS;
}

#if 0
#ifndef MINISIZE
int
_eXosip_recvfrom (int s, char *buf, int len, unsigned int flags,
                  struct sockaddr *from, socklen_t * fromlen)
{
  int message_size = 0;
  int length_done = 0;
  int real_size = 0;
  int i;
  int extra_data_discarded;

  if (!eXosip.http_port)
    {
      return recvfrom (s, buf, len, flags, from, fromlen);
    }

  /* we get the size of the HTTP data */
  i = recv (eXosip.net_interfaces[0].net_socket, (char *) &(message_size), 4, 0);

  real_size = message_size;
  if (message_size < 0)
    {
      return -1;                /* Connection error? */
    }
  if (message_size == 0)
    {
      buf[0] = '\0';
      return OSIP_SUCCESS;
    }
  if (message_size > len - 1)
    message_size = len - 1;

  length_done = 0;

  i = recv (eXosip.net_interfaces[0].net_socket, buf, message_size, 0);
  length_done = i;

  if (length_done == real_size)
    {
      return length_done;
    }

  if (length_done < message_size)
    {
      /* continue reading up to message_size */
      while (length_done < message_size)
        {
          i =
            recv (eXosip.net_interfaces[0].net_socket, buf + length_done,
                  message_size - length_done, 0);
          length_done = length_done + i;
        }
    }

  extra_data_discarded = length_done;
  while (extra_data_discarded < real_size)
    {
      char buf2[2048];

      /* We have to discard the end of data... up to the next message */
      i = recv (eXosip.net_interfaces[0].net_socket, buf2, 2048, 0);
      extra_data_discarded = extra_data_discarded + i;
    }
  return length_done;
}

int
_eXosip_sendto (int s, const void *buf, size_t len, int flags,
                const struct sockaddr *to, socklen_t tolen)
{
  int i;
  char buf2[10000];

  if (!eXosip.http_port)
    {
      i = sendto (s, buf, len, flags, to, tolen);
      return i;
    }

  memset (buf2, '\0', sizeof (buf2));
  memcpy (buf2, &len, sizeof (int));
  memcpy (buf2 + sizeof (int), buf, len);

  i = send (s, (char *) buf2, len + sizeof (int), 0);   /* use TCP connection to proxy */
  if (i > 0)
    i = i - sizeof (int);

  return i;
}
#endif

#endif
