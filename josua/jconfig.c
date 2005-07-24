/*
 * josua - Jack's open sip User Agent
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef WIN32
#define snprintf _snprintf
#else
#include "config.h"
#endif

#include <osipparser2/osip_port.h>
#include "jconfig.h"
#include <eXosip2/eXosip.h>

static josua_config_t *selected_config = NULL;
static josua_config_t *josua_configs = NULL;

int
josua_config_select (int i)
{
  josua_config_t *jc = NULL;

  for (jc = josua_configs; jc != NULL; jc = jc->next)
    {
      if (i == jc->id)
        break;
    }
  selected_config = jc;
  if (jc == NULL)
    return -1;
  return jc->id;
}

int
josua_config_create (char *identity, char *proxy, char *registrar)
{
  josua_config_t *jc;
  static int id = 0;

  id++;

  jc = (josua_config_t *) osip_malloc (sizeof (josua_config_t));
  jc->id = id;
  snprintf (jc->identity, 100, identity);
  snprintf (jc->proxy, 100, proxy);
  snprintf (jc->registrar, 100, registrar);
  jc->realms = NULL;
  jc->next = NULL;
  jc->parent = NULL;

  ADD_ELEMENT (josua_configs, jc);
  selected_config = jc;
  return jc->id;
}


int
josua_config_addrealm (char *realm, char *username, char *password)
{
  josua_realm_t *jr;

  jr = (josua_realm_t *) osip_malloc (sizeof (josua_realm_t));
  snprintf (jr->realm, 100, realm);
  snprintf (jr->username, 100, username);
  snprintf (jr->password, 100, password);
  jr->next = NULL;
  jr->parent = NULL;

  ADD_ELEMENT (selected_config->realms, jr);
  return selected_config->id;
}
