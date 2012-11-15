/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard@antisip.com
  
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

#include "eXosip2.h"

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
