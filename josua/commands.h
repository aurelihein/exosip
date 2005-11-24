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

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <osip2/osip_mt.h>

#include <eXosip2/eXosip.h>
#include "jcalls.h"
#include "jmainconfig.h"

int _josua_start_call (char *from, char *to, char *subject, char *route,
                       char *port, void *reference);
int _josua_mute_call (int did, char *wav_file);
int _josua_unmute_call (int did);

int _josua_start_options (char *from, char *to, char *route);

int _josua_start_message (char *from, char *to, char *route, char *buf);

int _josua_start_subscribe (char *from, char *to, char *route);

int _josua_start_refer (char *refer_to, char *from, char *to, char *route);

int _josua_add_contact (char *sipurl, char *telurl, char *email, char *phone);

int _josua_register (int pos_id);
int _josua_unregister (int pos_id);

int _josua_set_service_route (osip_message_t * response);

#endif
