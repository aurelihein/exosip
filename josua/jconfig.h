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


#ifndef _JCONFIG_H_
#define _JCONFIG_H_

typedef struct josua_realm_t josua_realm_t;

struct josua_realm_t
{

  char realm[100];
  char username[100];
  char password[100];

  josua_realm_t *next;
  josua_realm_t *parent;

};

typedef struct josua_config_t josua_config_t;

struct josua_config_t
{

  int id;
  char identity[100];
  char proxy[100];
  char registrar[100];

  josua_realm_t *realms;

  josua_config_t *next;
  josua_config_t *parent;

};

int josua_config_select (int i);
int josua_config_create (char *identity, char *proxy, char *registrar);

int josua_config_addrealm (char *realm, char *username, char *password);

#endif
