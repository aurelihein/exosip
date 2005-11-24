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

#include <sys/socket.h>
#include "commands.h"
#include "jfriends.h"
#include "jidentity.h"
#include "sdptools.h"

static int _check_url (char *url);

static int
_check_url (char *url)
{
  int i;
  osip_from_t *to;

  i = osip_from_init (&to);
  if (i != 0)
    return -1;
  i = osip_from_parse (to, url);
  osip_from_free (to);
  if (i != 0)
    return -1;
  return 0;
}

int
_josua_start_call (char *from, char *to, char *subject, char *route,
                   char *port, void *reference)
{
  osip_message_t *invite;
  int i;

  osip_clrspace (to);
  osip_clrspace (subject);
  osip_clrspace (from);
  osip_clrspace (route);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL, "To: |%s|\n", to));
  if (0 != _check_url (from))
    return -1;
  if (0 != _check_url (to))
    return -1;

  i = eXosip_call_build_initial_invite (&invite, to, from, route, subject);
  if (i != 0)
    {
      return -1;
    }

  osip_message_set_supported (invite, "100rel");

  /* add sdp body */
  {
    char tmp[4096];
    char localip[128];

    eXosip_guess_localip (AF_INET, localip, 128);
    snprintf (tmp, 4096,
              "v=0\r\n"
              "o=josua 0 0 IN IP4 %s\r\n"
              "s=conversation\r\n"
              "c=IN IP4 %s\r\n"
              "t=0 0\r\n"
              "m=audio %s RTP/AVP 0 8 101\r\n"
              "a=rtpmap:0 PCMU/8000\r\n"
              "a=rtpmap:8 PCMA/8000\r\n"
              "a=rtpmap:101 telephone-event/8000\r\n"
              "a=fmtp:101 0-11\r\n", localip, localip, port);
    osip_message_set_body (invite, tmp, strlen (tmp));
    osip_message_set_content_type (invite, "application/sdp");
  }

  if (cfg.service_route[0] != '\0')
    {
      char *header = osip_strdup ("route");

      osip_message_set_multiple_header (invite, header, cfg.service_route);
      osip_free (header);
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO2, NULL,
                   "Route from Service-Route discovery added: |%s|\n",
                   cfg.service_route));
    }

  eXosip_lock ();
  i = eXosip_call_send_initial_invite (invite);
  if (i > 0)
    {
      eXosip_call_set_reference (i, reference);
    }
  OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_INFO2, NULL,
               "CALL STARTED FOR CID: %i\n", i));
  eXosip_unlock ();
  return i;
}

int
_josua_mute_call (int did, char *wav_file)
{
  osip_message_t *invite;
  int i;

  char port[10];
  sdp_message_t *local_sdp = NULL;

  eXosip_lock ();
  local_sdp = eXosip_get_local_sdp (did);
  eXosip_unlock ();
  if (local_sdp == NULL)
    return -1;

  snprintf (port, 10, "%i", 10502);

  eXosip_lock ();
  i = eXosip_call_build_request (did, "INVITE", &invite);
  eXosip_unlock ();

  if (i != 0)
    {
      sdp_message_free (local_sdp);
      return -1;
    }

  osip_message_set_supported (invite, "100rel");

  /* add sdp body */
  {
    char *tmp = NULL;

    i = _sdp_hold_call (local_sdp);
    if (i != 0)
      {
        sdp_message_free (local_sdp);
        osip_message_free (invite);
        return -1;
      }

    i = sdp_message_to_str (local_sdp, &tmp);
    sdp_message_free (local_sdp);
    if (i != 0)
      {
        osip_message_free (invite);
        return -1;
      }
    osip_message_set_body (invite, tmp, strlen (tmp));
    osip_free (tmp);
    osip_message_set_content_type (invite, "application/sdp");
  }

  eXosip_lock ();
  i = eXosip_call_send_request (did, invite);
  if (i > 0)
    {
    }
  eXosip_unlock ();
  return i;
}

int
_josua_unmute_call (int did)
{
  osip_message_t *invite;
  int i;

  char port[10];
  sdp_message_t *local_sdp = NULL;

  eXosip_lock ();
  local_sdp = eXosip_get_local_sdp (did);
  eXosip_unlock ();
  if (local_sdp == NULL)
    return -1;

  snprintf (port, 10, "%i", 10504);

  eXosip_lock ();
  i = eXosip_call_build_request (did, "INVITE", &invite);
  eXosip_unlock ();

  if (i != 0)
    {
      sdp_message_free (local_sdp);
      return -1;
    }

  osip_message_set_supported (invite, "100rel");

  /* add sdp body */
  {
    char *tmp = NULL;

    i = _sdp_off_hold_call (local_sdp);
    if (i != 0)
      {
        sdp_message_free (local_sdp);
        osip_message_free (invite);
        return -1;
      }

    i = sdp_message_to_str (local_sdp, &tmp);
    sdp_message_free (local_sdp);
    if (i != 0)
      {
        osip_message_free (invite);
        return -1;
      }
    osip_message_set_body (invite, tmp, strlen (tmp));
    osip_free (tmp);
    osip_message_set_content_type (invite, "application/sdp");
  }

  eXosip_lock ();
  i = eXosip_call_send_request (did, invite);
  if (i > 0)
    {
    }
  eXosip_unlock ();
  return i;
}

int
_josua_start_options (char *from, char *to, char *route)
{
  osip_message_t *options;
  int i;

  osip_clrspace (to);
  osip_clrspace (from);
  osip_clrspace (route);

  i = eXosip_options_build_request (&options, to, from, route);
  if (i != 0)
    {
      return -1;
    }
  eXosip_lock ();
  i = eXosip_options_send_request (options);
  eXosip_unlock ();
  return i;
}

int
_josua_start_message (char *from, char *to, char *route, char *buf)
{
  osip_message_t *message;
  int i;

  osip_clrspace (to);
  osip_clrspace (from);
  osip_clrspace (route);

  i = eXosip_message_build_request (&message, "MESSAGE", to, from, route);
  if (i != 0)
    {
      return -1;
    }
  osip_message_set_expires (message, "120");
  osip_message_set_body (message, buf, strlen (buf));
  osip_message_set_content_type (message, "text/plain");

  eXosip_lock ();
  i = eXosip_message_send_request (message);
  eXosip_unlock ();
  return i;
}

int
_josua_start_subscribe (char *from, char *to, char *route)
{
  osip_message_t *sub;
  int i;

  osip_clrspace (to);
  osip_clrspace (from);
  osip_clrspace (route);

  if (0 != _check_url (from))
    return -1;
  if (0 != _check_url (to))
    return -1;

  i = eXosip_subscribe_build_initial_request (&sub, to, from, route,
                                              "presence", 900);
  if (i != 0)
    {
      return -1;
    }
#ifdef SUPPORT_MSN
  osip_message_set_accept (sub, "application/xpidf+xml");
#else
  osip_message_set_accept (sub, "application/pidf+xml");
#endif

  eXosip_lock ();
  i = eXosip_subscribe_send_initial_request (sub);
  eXosip_unlock ();
  return i;
}

int
_josua_start_refer (char *refer_to, char *from, char *to, char *route)
{
  osip_message_t *refer;
  int i;

  osip_clrspace (to);
  osip_clrspace (from);
  osip_clrspace (route);
  osip_clrspace (refer_to);

  if (0 != _check_url (from))
    return -1;
  if (0 != _check_url (to))
    return -1;

  i = eXosip_refer_build_request (&refer, refer_to, from, to, route);
  if (i != 0)
    {
      return -1;
    }

  eXosip_lock ();
  i = eXosip_refer_send_request (refer);
  eXosip_unlock ();
  return i;
}

int
_josua_add_contact (char *sipurl, char *telurl, char *email, char *phone)
{
  int i;
  char *_telurl;
  char *_email;
  char *_phone;

  osip_to_t *to;

  if (telurl == NULL || telurl[0] == '\0')
    _telurl = 0;
  else
    _telurl = telurl;

  if (email == NULL || email[0] == '\0')
    _email = 0;
  else
    _email = email;

  if (phone == NULL || phone[0] == '\0')
    _phone = 0;
  else
    _phone = phone;

  if (sipurl == NULL || sipurl[0] == '\0')
    return -1;

  i = osip_to_init (&to);
  if (i != 0)
    return -1;
  i = osip_to_parse (to, sipurl);
  if (i != 0)
    {
      osip_to_free (to);
      return -1;
    }
  if (to->displayname == NULL && to->url->username == NULL)
    jfriend_add ("xxxx", sipurl, _telurl, _email, _phone);
  else if (to->displayname != NULL)
    jfriend_add (to->displayname, sipurl, _telurl, _email, _phone);
  else if (to->url != NULL && to->url->username != NULL)
    jfriend_add (to->url->username, sipurl, _telurl, _email, _phone);

  osip_to_free (to);
  jfriend_unload ();
  jfriend_load ();
  return 0;
}

static int last_pos_id = -2;
static int last_reg_id = -2;

int
_josua_register (int pos_id)
{
  osip_message_t *reg = NULL;
  int reg_id = -1;
  int i;
  char *identity;
  char *registrar;
  char *route;

  /* start registration */
  identity = jidentity_get_identity (pos_id);
  registrar = jidentity_get_registrar (pos_id);
  route = jidentity_get_route (pos_id);
  if (identity == NULL || registrar == NULL)
    return -1;

  eXosip_lock ();
  if (pos_id != last_pos_id)
    {
      reg_id =
        eXosip_register_build_initial_register (identity, registrar, NULL,
                                                1800, &reg);
      if (reg_id < 0)
        {
          eXosip_unlock ();
          return -1;
        }

      if (cfg.proto == 1)
        eXosip_transport_set (reg, "TCP");
      else if (cfg.proto == 2)
        eXosip_transport_set (reg, "TLS");

      osip_message_set_supported (reg, "path");
      if (route != NULL && route[0] != '\0')
        {
          char *header = osip_strdup ("route");

          osip_message_set_multiple_header (reg, header, route);
          osip_free (header);
        }
      last_pos_id = pos_id;
      last_reg_id = reg_id;
  } else
    {
      if (last_reg_id < 0)
        {
          eXosip_unlock ();
          return -1;
        }
      reg_id = last_reg_id;
      i = eXosip_register_build_register (reg_id, 1800, &reg);
      if (i < 0)
        {
          eXosip_unlock ();
          return -1;
        }

      if (cfg.proto == 1)
        eXosip_transport_set (reg, "TCP");
      else if (cfg.proto == 2)
        eXosip_transport_set (reg, "TLS");
    }

  eXosip_register_send_register (reg_id, reg);
  eXosip_unlock ();
  return i;
}


int
_josua_unregister (int pos_id)
{
  osip_message_t *reg = NULL;
  int reg_id;
  int i;
  char *identity;
  char *registrar;

  /* start registration */
  identity = jidentity_get_identity (pos_id);
  registrar = jidentity_get_registrar (pos_id);
  if (identity == NULL || registrar == NULL)
    return -1;

  eXosip_lock ();
  if (pos_id != last_pos_id)
    {
      reg_id =
        eXosip_register_build_initial_register (identity, registrar, NULL, 0,
                                                &reg);
      if (reg_id < 0)
        {
          eXosip_unlock ();
          return -1;
        }

      if (cfg.proto == 1)
        eXosip_transport_set (reg, "TCP");
      else if (cfg.proto == 2)
        eXosip_transport_set (reg, "TLS");

      last_pos_id = pos_id;
      last_reg_id = reg_id;
  } else
    {
      if (last_reg_id < 0)
        {
          eXosip_unlock ();
          return -1;
        }
      reg_id = last_reg_id;
      i = eXosip_register_build_register (reg_id, 0, &reg);
      if (i < 0)
        {
          eXosip_unlock ();
          return -1;
        }

      if (cfg.proto == 1)
        eXosip_transport_set (reg, "TCP");
      else if (cfg.proto == 2)
        eXosip_transport_set (reg, "TLS");

      last_reg_id = -1;
      last_pos_id = -1;
    }
  eXosip_register_send_register (reg_id, reg);
  eXosip_unlock ();
  return i;
}

int
_josua_set_service_route (osip_message_t * response)
{
  /* rfc3608.txt:
     Extension Header Field for Service Route Discovery */
  /* build the complete service-route header */
  osip_header_t *hservroute;
  int pos;
  char servroute[2048];         /* should be enough! */
  int max_length = 2047;

  servroute[0] = '\0';
  pos = 0;
  pos =
    osip_message_header_get_byname (response, "service-route", pos, &hservroute);
  while (pos >= 0)
    {
      if (hservroute->hvalue == NULL)
        {
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
                                  "Empty pass!!\n"));
      } else
        {
          int header_length = strlen (hservroute->hvalue);

          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
                                  "service-route header found: %s\n",
                                  hservroute->hvalue));
          if (2047 - strlen (servroute) < header_length)
            {                   /* servroute header too long, discard other entries */
          } else
            {
              if (max_length == 2047)   /* don't add a coma */
                osip_strncpy (servroute, hservroute->hvalue, header_length);
              else
                {
                  osip_strncpy (servroute + strlen (servroute), ",", 1);
                  osip_strncpy (servroute + strlen (servroute),
                                hservroute->hvalue, header_length);
                }
              max_length = 2047 - strlen (servroute);
            }
        }
      pos++;
      pos =
        osip_message_header_get_byname (response, "service-route", pos,
                                        &hservroute);
    }


  if (servroute[0] != '\0')
    snprintf (cfg.service_route, 2048, "%s", servroute);
  return 0;
}
