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

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __APPLE_CC__
#include <unistd.h>
#endif
#else
#include <windows.h>
#include <iphlpapi.h>
#endif

extern eXosip_t eXosip;

extern int ipv6_enable;

/* Private functions */
static int dialog_fill_route_set (osip_dialog_t * dialog,
                                  osip_message_t * request);

/* should use cryptographically random identifier is RECOMMENDED.... */
/* by now this should lead to identical call-id when application are
   started at the same time...   */
char *
osip_call_id_new_random ()
{
  char *tmp = (char *) osip_malloc (33);
  unsigned int number = osip_build_random_number ();
  if (tmp == NULL)
    return NULL;

  sprintf (tmp, "%u", number);
  return tmp;
}

int
eXosip_generate_random (char *buf, int buf_size)
{
  unsigned int number = osip_build_random_number ();

  snprintf (buf, buf_size, "%u", number);
  return OSIP_SUCCESS;
}

char *
osip_from_tag_new_random (void)
{
  return osip_call_id_new_random ();
}

char *
osip_to_tag_new_random (void)
{
  return osip_call_id_new_random ();
}

unsigned int
via_branch_new_random (void)
{
  return osip_build_random_number ();
}

int
_eXosip_dialog_add_contact (osip_message_t * request, osip_message_t * answer)
{
  osip_via_t *via;
  osip_from_t *a_from;
  char *contact = NULL;
  char locip[65];
  char firewall_ip[65];
  char firewall_port[10];
  int len;

  if (eXosip.eXtl == NULL)
    return OSIP_NO_NETWORK;
  if (request == NULL)
    return OSIP_BADPARAMETER;

  firewall_ip[0] = '\0';
  firewall_port[0] = '\0';
  if (eXosip.eXtl->tl_get_masquerade_contact != NULL)
    {
      eXosip.eXtl->tl_get_masquerade_contact (firewall_ip, sizeof (firewall_ip),
                                              firewall_port,
                                              sizeof (firewall_port));
    }

  /* search for topmost Via which indicate the transport protocol */
  via = (osip_via_t *) osip_list_get (&request->vias, 0);
  if (via == NULL || via->protocol == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: missing via header\n"));
      return OSIP_SYNTAXERROR;
    }

  if (answer == NULL)
    a_from = request->from;
  else
    a_from = answer->to;

  if (a_from == NULL || a_from->url == NULL)
    return OSIP_SYNTAXERROR;

  /*guess the local ip since req uri is known */
  memset (locip, '\0', sizeof (locip));

  if (a_from->url->username != NULL)
    len =
      2 + 4 + strlen (a_from->url->username) + 1 + 100 + 6 + 10 +
      strlen (eXosip.transport);
  else
    len = 2 + 4 + 100 + 6 + 10 + strlen (eXosip.transport);

  contact = (char *) osip_malloc (len + 1);
  if (contact == NULL)
    return OSIP_NOMEM;
  if (firewall_ip[0] != '\0')
    {
      char *c_address = request->req_uri->host;

      struct addrinfo *addrinfo;
      struct __eXosip_sockaddr addr;
      int i;

      i =
        eXosip_get_addrinfo (&addrinfo, request->req_uri->host, 5060, IPPROTO_TCP);
      if (i == 0)
        {
          memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
          eXosip_freeaddrinfo (addrinfo);
          c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
          OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
                                  "eXosip: here is the resolved destination host=%s\n",
                                  c_address));
        }

      if (eXosip_is_public_address (c_address))
        {
          memcpy (locip, firewall_ip, sizeof (locip));
        }
    }

  if (locip[0] == '\0')
    {
      eXosip_guess_ip_for_via (eXosip.eXtl->proto_family, locip, 49);
      if (locip[0] == '\0')
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "eXosip: no default interface defined\n"));
          return OSIP_NO_NETWORK;
        }
    }

  if (eXosip.eXtl->proto_family == AF_INET6)
    {
      if (a_from->url->username != NULL)
        snprintf (contact, len, "<sip:%s@[%s]:%s>", a_from->url->username,
                  locip, firewall_port);
      else
        snprintf (contact, len - strlen (eXosip.transport) - 10, "<sip:[%s]:%s>",
                  locip, firewall_port);
  } else
    {
      if (a_from->url->username != NULL)
        snprintf (contact, len, "<sip:%s@%s:%s>", a_from->url->username,
                  locip, firewall_port);
      else
        snprintf (contact, len - strlen (eXosip.transport) - 10, "<sip:%s:%s>",
                  locip, firewall_port);
    }
  if (osip_strcasecmp (eXosip.transport, "UDP") != 0)
    {
      contact[strlen (contact) - 1] = '\0';
      strcat (contact, ";transport=");
      strcat (contact, eXosip.transport);
      strcat (contact, ">");
    }
  osip_message_set_contact (request, contact);
  osip_free (contact);

  return OSIP_SUCCESS;
}

int
_eXosip_request_add_via (osip_message_t * request, const char *transport,
                         const char *locip)
{
  char tmp[200];
  const char *ip = NULL;
  char firewall_ip[65];
  char firewall_port[10];

  if (request == NULL)
    return OSIP_BADPARAMETER;

  if (request->call_id == NULL)
    return OSIP_SYNTAXERROR;

  if (locip == NULL && request->call_id->host == NULL)
    return OSIP_SYNTAXERROR;

  if (locip != NULL)
    ip = locip;
  else if (request->call_id->host != NULL)
    ip = request->call_id->host;

  firewall_ip[0] = '\0';
  firewall_port[0] = '\0';
  if (eXosip.eXtl != NULL && eXosip.eXtl->tl_get_masquerade_contact != NULL)
    {
      eXosip.eXtl->tl_get_masquerade_contact (firewall_ip, sizeof (firewall_ip),
                                              firewall_port,
                                              sizeof (firewall_port));
    }
#ifdef MASQUERADE_VIA
  /* this helps to work with a server that don't handle the
     "received" parameter correctly. Some still exists. */
  if (firewall_ip[0] != '\0')
    {
      ip = firewall_ip;
    }
#endif

  if (firewall_port[0] == '\0')
    {
      snprintf (firewall_port, sizeof (firewall_port), "5060");
    }

  if (eXosip.eXtl->proto_family == AF_INET6)
    snprintf (tmp, 200, "SIP/2.0/%s [%s]:%s;branch=z9hG4bK%u",
              eXosip.transport, ip, firewall_port, via_branch_new_random ());
  else
    {
      if (eXosip.use_rport != 0)
        snprintf (tmp, 200, "SIP/2.0/%s %s:%s;rport;branch=z9hG4bK%u",
                  eXosip.transport, ip, firewall_port, via_branch_new_random ());
      else
        snprintf (tmp, 200, "SIP/2.0/%s %s:%s;branch=z9hG4bK%u",
                  eXosip.transport, ip, firewall_port, via_branch_new_random ());
    }

  osip_message_set_via (request, tmp);

  return OSIP_SUCCESS;
}

/* prepare a minimal request (outside of a dialog) with required headers */
/* 
   method is the type of request. ("INVITE", "REGISTER"...)
   to is the remote target URI
   transport is either "TCP" or "UDP" (by now, only UDP is implemented!)
*/
int
generating_request_out_of_dialog (osip_message_t ** dest, const char *method,
                                  const char *to, const char *transport,
                                  const char *from, const char *proxy)
{
  /* Section 8.1:
     A valid request contains at a minimum "To, From, Call-iD, Cseq,
     Max-Forwards and Via
   */
  int i;
  osip_message_t *request;
  char locip[65];
  int doing_register;
  char *register_callid_number = NULL;

  *dest = NULL;

  if (eXosip.eXtl == NULL)
    return OSIP_NO_NETWORK;

  /*guess the local ip since req uri is known */
  memset (locip, '\0', sizeof (locip));
  eXosip_guess_ip_for_via (eXosip.eXtl->proto_family, locip, 49);
  if (locip[0] == '\0')
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: no default interface defined\n"));
      return OSIP_NO_NETWORK;
    }

  i = osip_message_init (&request);
  if (i != 0)
    return i;

  /* prepare the request-line */
  osip_message_set_method (request, osip_strdup (method));
  osip_message_set_version (request, osip_strdup ("SIP/2.0"));
  osip_message_set_status_code (request, 0);
  osip_message_set_reason_phrase (request, NULL);

  doing_register = 0 == strcmp ("REGISTER", method);

  if (doing_register)
    {
      osip_uri_init (&(request->req_uri));
      i = osip_uri_parse (request->req_uri, proxy);
      if (i != 0)
        {
          osip_message_free (request);
          return i;
        }
      i = osip_message_set_to (request, from);
      if (i != 0 || request->to == NULL)
        {
          if (i >= 0)
            i = OSIP_SYNTAXERROR;
          osip_message_free (request);
          return i;
        }
  } else
    {
      /* in any cases except REGISTER: */
      i = osip_message_set_to (request, to);
      if (i != 0 || request->to == NULL)
        {
          if (i >= 0)
            i = OSIP_SYNTAXERROR;
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "ERROR: callee address does not seems to be a sipurl: %s\n",
                       to));
          osip_message_free (request);
          return i;
        }

      /* REMOVE ALL URL PARAMETERS from to->url headers and add them as headers */
      if (request->to != NULL && request->to->url != NULL)
        {
          osip_uri_t *url = request->to->url;
          while (osip_list_size (&url->url_headers) > 0)
            {
              osip_uri_header_t *u_header;
              u_header = (osip_uri_param_t *) osip_list_get (&url->url_headers, 0);
              if (u_header == NULL)
                break;

              if (osip_strcasecmp (u_header->gname, "from") == 0)
                {
              } else if (osip_strcasecmp (u_header->gname, "to") == 0)
                {
              } else if (osip_strcasecmp (u_header->gname, "call-id") == 0)
                {
              } else if (osip_strcasecmp (u_header->gname, "cseq") == 0)
                {
              } else if (osip_strcasecmp (u_header->gname, "via") == 0)
                {
              } else if (osip_strcasecmp (u_header->gname, "contact") == 0)
                {
              } else
                osip_message_set_header (request, u_header->gname,
                                         u_header->gvalue);
              osip_list_remove (&url->url_headers, 0);
              osip_uri_param_free (u_header);
            }
        }

      if (proxy != NULL && proxy[0] != 0)
        {                       /* equal to a pre-existing route set */
          /* if the pre-existing route set contains a "lr" (compliance
             with bis-08) then the req_uri should contains the remote target
             URI */
          osip_uri_param_t *lr_param;
          osip_route_t *o_proxy;

          osip_route_init (&o_proxy);
          i = osip_route_parse (o_proxy, proxy);
          if (i != 0)
            {
              osip_route_free (o_proxy);
              osip_message_free (request);
              return i;
            }

          osip_uri_uparam_get_byname (o_proxy->url, "lr", &lr_param);
          if (lr_param != NULL) /* to is the remote target URI in this case! */
            {
              osip_uri_clone (request->to->url, &(request->req_uri));
              /* "[request] MUST includes a Route header field containing
                 the route set values in order." */
              osip_list_add (&request->routes, o_proxy, 0);
          } else
            /* if the first URI of route set does not contain "lr", the req_uri
               is set to the first uri of route set */
            {
              request->req_uri = o_proxy->url;
              o_proxy->url = NULL;
              osip_route_free (o_proxy);
              /* add the route set */
              /* "The UAC MUST add a route header field containing
                 the remainder of the route set values in order.
                 The UAC MUST then place the remote target URI into
                 the route header field as the last value
               */
              osip_message_set_route (request, to);
            }
      } else                    /* No route set (outbound proxy) is used */
        {
          /* The UAC must put the remote target URI (to field) in the req_uri */
          i = osip_uri_clone (request->to->url, &(request->req_uri));
          if (i != 0)
            {
              osip_message_free (request);
              return i;
            }
        }
    }

  /* set To and From */
  i = osip_message_set_from (request, from);
  if (i != 0 || request->from == NULL)
    {
      if (i >= 0)
        i = OSIP_SYNTAXERROR;
      osip_message_free (request);
      return i;
    }

  /* add a tag */
  osip_from_set_tag (request->from, osip_from_tag_new_random ());

  /* set the cseq and call_id header */
  {
    osip_call_id_t *callid;
    osip_cseq_t *cseq;
    char *num;
    char *cidrand;

    /* call-id is always the same for REGISTRATIONS */
    i = osip_call_id_init (&callid);
    if (i != 0)
      {
        osip_message_free (request);
        return i;
      }
    cidrand = osip_call_id_new_random ();
    osip_call_id_set_number (callid, cidrand);
    if (doing_register)
      register_callid_number = cidrand;

    request->call_id = callid;

    i = osip_cseq_init (&cseq);
    if (i != 0)
      {
        osip_message_free (request);
        return i;
      }
    num = osip_strdup (doing_register ? "1" : "20");
    osip_cseq_set_number (cseq, num);
    osip_cseq_set_method (cseq, osip_strdup (method));
    request->cseq = cseq;

    if (cseq->method == NULL || cseq->number == NULL)
      {
        osip_message_free (request);
        return OSIP_NOMEM;
      }
  }

  i = _eXosip_request_add_via (request, transport, locip);
  if (i != 0)
    {
      osip_message_free (request);
      return i;
    }

  /* always add the Max-Forward header */
  osip_message_set_max_forwards (request, "70");        /* a UA should start a request with 70 */

  if (0 == strcmp ("REGISTER", method))
    {
  } else if (0 == strcmp ("INFO", method))
    {
  } else if (0 == strcmp ("OPTIONS", method))
    {
      osip_message_set_accept (request, "application/sdp");
    }

  osip_message_set_user_agent (request, eXosip.user_agent);
  /*  else if ... */
  *dest = request;
  return OSIP_SUCCESS;
}

int
generating_register (eXosip_reg_t * jreg, osip_message_t ** reg, char *transport,
                     char *from, char *proxy, char *contact, int expires)
{
  int i;
  char locip[65];
  char firewall_ip[65];
  char firewall_port[10];
  if (eXosip.eXtl == NULL)
    return OSIP_NO_NETWORK;

  firewall_ip[0] = '\0';
  firewall_port[0] = '\0';
  if (eXosip.eXtl->tl_get_masquerade_contact != NULL)
    {
      eXosip.eXtl->tl_get_masquerade_contact (firewall_ip, sizeof (firewall_ip),
                                              firewall_port,
                                              sizeof (firewall_port));
    }

  i =
    generating_request_out_of_dialog (reg, "REGISTER", NULL, transport, from,
                                      proxy);
  if (i != 0)
    return i;

  memset (locip, '\0', sizeof (locip));
  eXosip_guess_ip_for_via (eXosip.eXtl->proto_family, locip, 49);

  if (locip[0] == '\0')
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: no default interface defined\n"));
      osip_message_free (*reg);
      *reg = NULL;
      return OSIP_NO_NETWORK;
    }

  if (contact == NULL)
    {
      osip_contact_t *new_contact = NULL;
      osip_uri_t *new_contact_url = NULL;

      i = osip_contact_init (&new_contact);
      if (i == 0)
        i = osip_uri_init (&new_contact_url);

      new_contact->url = new_contact_url;

      if (i == 0 && (*reg)->from != NULL
          && (*reg)->from->url != NULL && (*reg)->from->url->username != NULL)
        {
          new_contact_url->username = osip_strdup ((*reg)->from->url->username);
        }

      if (i == 0 && (*reg)->from != NULL && (*reg)->from->url != NULL)
        {
          /* serach for correct ip */
          if (firewall_ip[0] != '\0')
            {
              char *c_address = (*reg)->req_uri->host;

              struct addrinfo *addrinfo;
              struct __eXosip_sockaddr addr;

              i =
                eXosip_get_addrinfo (&addrinfo, (*reg)->req_uri->host, 5060,
                                     IPPROTO_UDP);
              if (i == 0)
                {
                  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
                  eXosip_freeaddrinfo (addrinfo);
                  c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
                  OSIP_TRACE (osip_trace
                              (__FILE__, __LINE__, OSIP_INFO1, NULL,
                               "eXosip: here is the resolved destination host=%s\n",
                               c_address));
                }

              if (eXosip_is_public_address (c_address))
                {
                  new_contact_url->host = osip_strdup (firewall_ip);
                  new_contact_url->port = osip_strdup (firewall_port);
              } else
                {
                  new_contact_url->host = osip_strdup (locip);
                  new_contact_url->port = osip_strdup (firewall_port);
                }
          } else
            {
              new_contact_url->host = osip_strdup (locip);
              new_contact_url->port = osip_strdup (firewall_port);
            }

          if (transport != NULL && osip_strcasecmp (transport, "UDP") != 0)
            {
              osip_uri_uparam_add (new_contact_url, osip_strdup ("transport"),
                                   osip_strdup (transport));
            }

          if (jreg->r_line[0] != '\0')
            {
              osip_uri_uparam_add (new_contact_url, osip_strdup ("line"),
                                   osip_strdup (jreg->r_line));
            }

          osip_list_add (&(*reg)->contacts, new_contact, -1);
        }
  } else
    {
      osip_message_set_contact (*reg, contact);
    }

  {
    char exp[10];               /* MUST never be ouside 1 and 3600 */

    snprintf (exp, 9, "%i", expires);
    osip_message_set_expires (*reg, exp);
  }

  osip_message_set_content_length (*reg, "0");

  return OSIP_SUCCESS;
}

#ifndef MINISIZE

int
generating_publish (osip_message_t ** message, const char *to,
                    const char *from, const char *route)
{
  int i;

  if (to != NULL && *to == '\0')
    return OSIP_BADPARAMETER;

  if (route != NULL && *route == '\0')
    route = NULL;

  i = generating_request_out_of_dialog (message, "PUBLISH", to, "UDP", from,
                                        route);
  if (i != 0)
    return i;

  /* osip_message_set_organization(*message, "Jack's Org"); */

  return OSIP_SUCCESS;
}

#endif

static int
dialog_fill_route_set (osip_dialog_t * dialog, osip_message_t * request)
{
  /* if the pre-existing route set contains a "lr" (compliance
     with bis-08) then the req_uri should contains the remote target
     URI */
  int i;
  int pos = 0;
  osip_uri_param_t *lr_param;
  osip_route_t *route;
  char *last_route;

  /* AMD bug: fixed 17/06/2002 */

  route = (osip_route_t *) osip_list_get (&dialog->route_set, 0);

  osip_uri_uparam_get_byname (route->url, "lr", &lr_param);
  if (lr_param != NULL)         /* the remote target URI is the req_uri! */
    {
      i = osip_uri_clone (dialog->remote_contact_uri->url, &(request->req_uri));
      if (i != 0)
        return i;
      /* "[request] MUST includes a Route header field containing
         the route set values in order." */
      /* AMD bug: fixed 17/06/2002 */
      pos = 0;                  /* first element is at index 0 */
      while (!osip_list_eol (&dialog->route_set, pos))
        {
          osip_route_t *route2;

          route = osip_list_get (&dialog->route_set, pos);
          i = osip_route_clone (route, &route2);
          if (i != 0)
            return i;
          osip_list_add (&request->routes, route2, -1);
          pos++;
        }
      return OSIP_SUCCESS;
    }

  /* if the first URI of route set does not contain "lr", the req_uri
     is set to the first uri of route set */


  i = osip_uri_clone (route->url, &(request->req_uri));
  if (i != 0)
    return i;
  /* add the route set */
  /* "The UAC MUST add a route header field containing
     the remainder of the route set values in order. */
  pos = 0;                      /* yes it is */

  while (!osip_list_eol (&dialog->route_set, pos))      /* not the first one in the list */
    {
      osip_route_t *route2;

      route = osip_list_get (&dialog->route_set, pos);
      i = osip_route_clone (route, &route2);
      if (i != 0)
        return i;
      if (!osip_list_eol (&dialog->route_set, pos + 1))
        osip_list_add (&request->routes, route2, -1);
      else
        osip_route_free (route2);
      pos++;
    }

  /* The UAC MUST then place the remote target URI into
     the route header field as the last value */
  i = osip_uri_to_str (dialog->remote_contact_uri->url, &last_route);
  if (i != 0)
    return i;
  i = osip_message_set_route (request, last_route);
  osip_free (last_route);
  if (i != 0)
    {
      return i;
    }

  /* route header and req_uri set */
  return OSIP_SUCCESS;
}

int
_eXosip_build_request_within_dialog (osip_message_t ** dest,
                                     const char *method,
                                     osip_dialog_t * dialog, const char *transport)
{
  int i;
  osip_message_t *request;
  char locip[65];
  char firewall_ip[65];
  char firewall_port[10];

  *dest = NULL;

  if (dialog == NULL)
    return OSIP_BADPARAMETER;

  if (eXosip.eXtl == NULL)
    return OSIP_NO_NETWORK;

  firewall_ip[0] = '\0';
  firewall_port[0] = '\0';
  if (eXosip.eXtl->tl_get_masquerade_contact != NULL)
    {
      eXosip.eXtl->tl_get_masquerade_contact (firewall_ip, sizeof (firewall_ip),
                                              firewall_port,
                                              sizeof (firewall_port));
    }

  i = osip_message_init (&request);
  if (i != 0)
    return i;

  if (dialog->remote_contact_uri == NULL)
    {
      /* this dialog is probably not established! or the remote UA
         is not compliant with the latest RFC
       */
      osip_message_free (request);
      return OSIP_SYNTAXERROR;
    }


  memset (locip, '\0', sizeof (locip));
  eXosip_guess_ip_for_via (eXosip.eXtl->proto_family, locip, 49);
  if (locip[0] == '\0')
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "eXosip: no default interface defined\n"));
      osip_message_free (request);
      return OSIP_NO_NETWORK;
    }

  /* prepare the request-line */
  request->sip_method = osip_strdup (method);
  if (request->sip_method == NULL)
    {
      osip_message_free (request);
      return OSIP_NOMEM;
    }
  request->sip_version = osip_strdup ("SIP/2.0");
  if (request->sip_version == NULL)
    {
      osip_message_free (request);
      return OSIP_NOMEM;
    }
  request->status_code = 0;
  request->reason_phrase = NULL;

  /* and the request uri???? */
  if (osip_list_eol (&dialog->route_set, 0))
    {
      /* The UAC must put the remote target URI (to field) in the req_uri */
      i = osip_uri_clone (dialog->remote_contact_uri->url, &(request->req_uri));
      if (i != 0)
        {
          osip_message_free (request);
          return i;
        }
  } else
    {
      /* fill the request-uri, and the route headers. */
      i = dialog_fill_route_set (dialog, request);
      if (i != 0)
        {
          osip_message_free (request);
          return i;
        }
    }

  /* To and From already contains the proper tag! */
  i = osip_to_clone (dialog->remote_uri, &(request->to));
  if (i != 0)
    {
      osip_message_free (request);
      return i;
    }
  i = osip_from_clone (dialog->local_uri, &(request->from));
  if (i != 0)
    {
      osip_message_free (request);
      return i;
    }

  /* set the cseq and call_id header */
  osip_message_set_call_id (request, dialog->call_id);

  if (0 == strcmp ("ACK", method))
    {
      osip_cseq_t *cseq;
      char *tmp;

      i = osip_cseq_init (&cseq);
      if (i != 0)
        {
          osip_message_free (request);
          return i;
        }
      tmp = osip_malloc (20);
      if (tmp == NULL)
        {
          osip_message_free (request);
          return OSIP_NOMEM;
        }
      sprintf (tmp, "%i", dialog->local_cseq);
      osip_cseq_set_number (cseq, tmp);
      osip_cseq_set_method (cseq, osip_strdup (method));
      request->cseq = cseq;
  } else
    {
      osip_cseq_t *cseq;
      char *tmp;

      i = osip_cseq_init (&cseq);
      if (i != 0)
        {
          osip_message_free (request);
          return i;
        }
      dialog->local_cseq++;     /* we should we do that?? */
      tmp = osip_malloc (20);
      if (tmp == NULL)
        {
          osip_message_free (request);
          return OSIP_NOMEM;
        }
      sprintf (tmp, "%i", dialog->local_cseq);
      osip_cseq_set_number (cseq, tmp);
      osip_cseq_set_method (cseq, osip_strdup (method));
      request->cseq = cseq;
    }

  /* always add the Max-Forward header */
  osip_message_set_max_forwards (request, "70");        /* a UA should start a request with 70 */


  i = _eXosip_request_add_via (request, transport, locip);
  if (i != 0)
    {
      osip_message_free (request);
      return i;
    }

  /* add specific headers for each kind of request... */

  {
    char contact[200];

    if (firewall_ip[0] != '\0')
      {
        char *c_address = request->req_uri->host;

        struct addrinfo *addrinfo;
        struct __eXosip_sockaddr addr;

        i =
          eXosip_get_addrinfo (&addrinfo, request->req_uri->host, 5060,
                               IPPROTO_UDP);
        if (i == 0)
          {
            memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
            eXosip_freeaddrinfo (addrinfo);
            c_address = inet_ntoa (((struct sockaddr_in *) &addr)->sin_addr);
            OSIP_TRACE (osip_trace
                        (__FILE__, __LINE__, OSIP_INFO1, NULL,
                         "eXosip: here is the resolved destination host=%s\n",
                         c_address));
          }

        if (eXosip_is_public_address (c_address))
          {
            sprintf (contact, "<sip:%s@%s:%s>",
                     dialog->local_uri->url->username, firewall_ip, firewall_port);
        } else
          {
            sprintf (contact, "<sip:%s@%s:%s>",
                     dialog->local_uri->url->username, locip, firewall_port);
          }
    } else
      {
        sprintf (contact, "<sip:%s@%s:%s>", dialog->local_uri->url->username,
                 locip, firewall_port);
      }
    osip_message_set_contact (request, contact);
    /* Here we'll add the supported header if it's needed! */
    /* the require header must be added by the upper layer if needed */
  }

  if (0 == strcmp ("NOTIFY", method))
    {
  } else if (0 == strcmp ("INFO", method))
    {

  } else if (0 == strcmp ("OPTIONS", method))
    {
      osip_message_set_accept (request, "application/sdp");
  } else if (0 == strcmp ("ACK", method))
    {
      /* The ACK MUST contains the same credential than the INVITE!! */
      /* TODO... */
    }

  osip_message_set_user_agent (request, eXosip.user_agent);
  /*  else if ... */
  *dest = request;
  return OSIP_SUCCESS;
}

/* this request is only build within a dialog!! */
int
generating_bye (osip_message_t ** bye, osip_dialog_t * dialog, char *transport)
{
  int i;

  i = _eXosip_build_request_within_dialog (bye, "BYE", dialog, transport);
  if (i != 0)
    return i;

  return OSIP_SUCCESS;
}

/* It is RECOMMENDED to only cancel INVITE request */
int
generating_cancel (osip_message_t ** dest, osip_message_t * request_cancelled)
{
  int i;
  osip_message_t *request;

  i = osip_message_init (&request);
  if (i != 0)
    return i;

  /* prepare the request-line */
  osip_message_set_method (request, osip_strdup ("CANCEL"));
  osip_message_set_version (request, osip_strdup ("SIP/2.0"));
  osip_message_set_status_code (request, 0);
  osip_message_set_reason_phrase (request, NULL);

  i = osip_uri_clone (request_cancelled->req_uri, &(request->req_uri));
  if (i != 0)
    {
      osip_message_free (request);
      *dest = NULL;
      return i;
    }

  i = osip_to_clone (request_cancelled->to, &(request->to));
  if (i != 0)
    {
      osip_message_free (request);
      *dest = NULL;
      return i;
    }
  i = osip_from_clone (request_cancelled->from, &(request->from));
  if (i != 0)
    {
      osip_message_free (request);
      *dest = NULL;
      return i;
    }

  /* set the cseq and call_id header */
  i = osip_call_id_clone (request_cancelled->call_id, &(request->call_id));
  if (i != 0)
    {
      osip_message_free (request);
      *dest = NULL;
      return i;
    }
  i = osip_cseq_clone (request_cancelled->cseq, &(request->cseq));
  if (i != 0)
    {
      osip_message_free (request);
      *dest = NULL;
      return i;
    }
  osip_free (request->cseq->method);
  request->cseq->method = osip_strdup ("CANCEL");
  if (request->cseq->method == NULL)
    {
      osip_message_free (request);
      *dest = NULL;
      return OSIP_NOMEM;
    }

  /* copy ONLY the top most Via Field (this method is also used by proxy) */
  {
    osip_via_t *via;
    osip_via_t *via2;

    i = osip_message_get_via (request_cancelled, 0, &via);
    if (i < 0)
      {
        osip_message_free (request);
        *dest = NULL;
        return i;
      }
    i = osip_via_clone (via, &via2);
    if (i != 0)
      {
        osip_message_free (request);
        *dest = NULL;
        return i;
      }
    osip_list_add (&request->vias, via2, -1);
  }

  /* add the same route-set than in the previous request */
  {
    int pos = 0;
    osip_route_t *route;
    osip_route_t *route2;

    while (!osip_list_eol (&request_cancelled->routes, pos))
      {
        route = (osip_route_t *) osip_list_get (&request_cancelled->routes, pos);
        i = osip_route_clone (route, &route2);
        if (i != 0)
          {
            osip_message_free (request);
            *dest = NULL;
            return i;
          }
        osip_list_add (&request->routes, route2, -1);
        pos++;
      }
  }

  osip_message_set_max_forwards (request, "70");        /* a UA should start a request with 70 */
  osip_message_set_user_agent (request, eXosip.user_agent);

  *dest = request;
  return OSIP_SUCCESS;
}
