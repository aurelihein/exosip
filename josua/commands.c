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

#include "commands.h"

int _josua_start_call(char *from, char *to, char *subject, char *route)
{
  osip_message_t *invite;
  int i;
  i = eXosip_build_initial_invite(&invite,
				  to,
				  from,
				  route,
				  subject);
  if (i!=0)
    {
      return -1;
    }
  eXosip_lock();
  i = eXosip_start_call(invite, NULL, NULL, "10500");
  eXosip_unlock();  
  return i;
}
