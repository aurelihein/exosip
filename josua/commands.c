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
#include "eXosip/eXosip_cfg.h"

static int _check_url(char *url);

static int _check_url(char *url)
{
  int i;
  osip_from_t *to;
  i = osip_from_init(&to);
  if (i!=0) return -1;
  i = osip_from_parse(to, url);
  if (i!=0) return -1;
  return 0;
}

int _josua_start_call(char *from, char *to, char *subject, char *route, char *port, void *reference)
{
  osip_message_t *invite;
  int i;

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "To: |%s|\n", to));
  if (0!=_check_url(from))
    return -1;
  if (0!=_check_url(to))
    return -1;

#if 0
  if (0!=check_sipurl(route))
    return -1;
#endif

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
  i = eXosip_initiate_call(invite, reference, NULL, port);
  eXosip_unlock();  
  return i;
}

#if 0
int _josua_start_options(char *from, char *to, char *route)
{
  osip_message_t *options;
  int i;
  i = eXosip_build_initial_options(&options,
				  to,
				  from,
				  route);
  if (i!=0)
    {
      return -1;
    }
  eXosip_lock();
  i = eXosip_options_call(options, NULL, NULL, "10500");
  eXosip_unlock();
  return i;
}
#endif

int _josua_start_message(char *from, char *to, char *route, char *buf)
{
  int i;
  eXosip_lock();
  i = eXosip_message(to, from, route, buf);
  eXosip_unlock();
  if (i!=0)
    {
      return -1;
    }
  return i;
}

int _josua_start_subscribe(char *from, char *to, char *route)
{
  int i;
  eXosip_lock();
  i = eXosip_subscribe(to, from, route);
  eXosip_unlock();  
  return i;
}

int _josua_add_contact(char *sipurl, char *telurl, char *email, char *phone)
{
  int i;
  char *_telurl;
  char *_email;
  char *_phone;

  osip_to_t *to;

  if (telurl==NULL||telurl[0]=='\0')
    _telurl = 0;
  else
    _telurl = telurl;

  if (email==NULL||email[0]=='\0')
    _email = 0;
  else
    _email = email;

  if (phone==NULL||phone[0]=='\0')
    _phone = 0;
  else
    _phone = phone;

  if (sipurl==NULL||sipurl[0]=='\0')
    return -1;

  i = osip_to_init(&to);
  if (i!=0)
    return -1;
  i = osip_to_parse(to, sipurl);
  if (i!=0)
    return -1;
  if (to->displayname==NULL && to->url->username==NULL)
    jfriend_add("xxxx", sipurl, _telurl, _email, _phone);
  else if (to->displayname!=NULL)
    jfriend_add(to->displayname, sipurl, _telurl, _email, _phone);
  else if (to->url!=NULL && to->url->username!=NULL)
    jfriend_add(to->url->username, sipurl, _telurl, _email, _phone);

  osip_to_free(to);
  jfriend_unload();
  jfriend_load();
  return 0;
}

static int last_pos_id = -2;
static int last_reg_id = -2;

int
_josua_register(int pos_id)
{
  int reg_id = -1;
  int i;
  char *identity;
  char *registrar;
  /* start registration */
  identity = jidentity_get_identity(pos_id);
  registrar = jidentity_get_registrar(pos_id);
  if (identity==NULL || registrar==NULL)
    return -1;
  
  eXosip_lock();
  if (pos_id!=last_pos_id)
    {
      reg_id = eXosip_register_init(identity, registrar, NULL);
      if (reg_id<0)
	{
	  eXosip_unlock();
	  return -1;
	}
      last_pos_id = pos_id;
      last_reg_id = reg_id;
    }
  else
    {
      if (last_reg_id<0)
	{
	  eXosip_unlock();
	  return -1;
	}
      reg_id = last_reg_id;
    }

  i = eXosip_register(reg_id, 3600);
  eXosip_unlock();
  return i;
}


int
_josua_unregister(int pos_id)
{
  int reg_id;
  int i;
  char *identity;
  char *registrar;
  /* start registration */
  identity = jidentity_get_identity(pos_id);
  registrar = jidentity_get_registrar(pos_id);
  if (identity==NULL || registrar==NULL)
    return -1;
  
  eXosip_lock();
  if (pos_id!=last_pos_id)
    {
      reg_id = eXosip_register_init(identity, registrar, NULL);
      if (reg_id<0)
	{
	  eXosip_unlock();
	  return -1;
	}
      last_pos_id = pos_id;
      last_reg_id = reg_id;
    }
  else
    {
      if (last_reg_id<0)
	{
	  eXosip_unlock();
	  return -1;
	}
      reg_id = last_reg_id;
      last_reg_id = -1;
      last_pos_id = -1;
    }
  i = eXosip_register(reg_id, 0);
  eXosip_unlock();
  return i;
}
