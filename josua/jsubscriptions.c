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

#include "jsubscriptions.h"

jsubscription_t jsubscriptions[MAX_NUMBER_OF_SUBSCRIPTIONS];

static int ___jsubscription_init = 0;
static int jsubscriptions_set_presence_status (jsubscription_t * ca,
                                               eXosip_event_t * je);

static void
__jsubscription_init ()
{
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      memset (&(jsubscriptions[k]), 0, sizeof (jsubscription_t));
      jsubscriptions[k].state = NOT_USED;
    }
}

int
jsubscriptions_set_presence_status (jsubscription_t * ca, eXosip_event_t * je)
{
  osip_content_type_t *ctype;
  osip_body_t *body = NULL;

  if (je->ss_status == EXOSIP_SUBCRSTATE_TERMINATED)
    {
      ca->online_status = EXOSIP_NOTIFY_UNKNOWN;        /* isn't it unknown? */
      return 0;
    }
  if (je->ss_status == EXOSIP_SUBCRSTATE_PENDING)
    {
      ca->online_status = EXOSIP_NOTIFY_PENDING;
      return 0;
    }

  ctype = osip_message_get_content_type (je->request);
  if (ctype == NULL || ctype->type == NULL || ctype->subtype == NULL)
    {
      return -1;                /* missing body */
    }

  osip_message_get_body (je->request, 0, &body);
  if (body == NULL)
    {
      return -1;
    }

  /* first test interop with MSN */
  if (0 == osip_strcasecmp (ctype->type, "application")
      && 0 == osip_strcasecmp (ctype->subtype, "xpidf+xml"))
    {
      /* search for the open string */
      char *tmp = body->body;

      while (tmp[0] != '\0')
        {
          if (tmp[0] == 'o' && 0 == strncmp (tmp, "online", 6))
            {
              ca->online_status = EXOSIP_NOTIFY_ONLINE;
              /* search for the contact entry */
              while (tmp[0] != '\0')
                {
                  if (tmp[0] == 'c' && 0 == strncmp (tmp, "contact", 7))
                    {
                      /* ... */
                    }
                  tmp++;
                }
              break;
          } else if (tmp[0] == 'b' && 0 == strncmp (tmp, "busy", 4))
            {
              ca->online_status = EXOSIP_NOTIFY_BUSY;
              break;
          } else if (tmp[0] == 'b' && 0 == strncmp (tmp, "berightback", 11))
            {
              ca->online_status = EXOSIP_NOTIFY_BERIGHTBACK;
              break;
          } else if (tmp[0] == 'a' && 0 == strncmp (tmp, "away", 4))
            {
              ca->online_status = EXOSIP_NOTIFY_AWAY;
              break;
          } else if (tmp[0] == 'o' && 0 == strncmp (tmp, "onthephone", 10))
            {
              ca->online_status = EXOSIP_NOTIFY_ONTHEPHONE;
              break;
          } else if (tmp[0] == 'o' && 0 == strncmp (tmp, "outtolunch", 10))
            {
              ca->online_status = EXOSIP_NOTIFY_OUTTOLUNCH;
              break;
            }
          tmp++;
        }
  } else if (0 == osip_strcasecmp (ctype->type, "application")
             && 0 == osip_strcasecmp (ctype->subtype, "pidf+xml"))
    {
      /* latest rfc! */

      /* search for the open string */
      char *tmp = body->body;

      while (tmp[0] != '\0')
        {
          if (tmp[0] == 'o' && 0 == osip_strncasecmp (tmp, "open", 4))
            {
              ca->online_status = EXOSIP_NOTIFY_ONLINE;
              /* search for the contact entry */
              while (tmp[0] != '\0')
                {
                  if (tmp[0] == 'a' && 0 == osip_strncasecmp (tmp, "away", 4))
                    ca->online_status = EXOSIP_NOTIFY_AWAY;
                  else if (tmp[0] == 'm' && 0 == osip_strncasecmp (tmp, "meal", 4))
                    ca->online_status = EXOSIP_NOTIFY_OUTTOLUNCH;
                  else if (tmp[0] == 'i'
                           && 0 == osip_strncasecmp (tmp, "in-transit", 10))
                    ca->online_status = EXOSIP_NOTIFY_BERIGHTBACK;
                  else if (tmp[0] == 'b' && 0 == osip_strncasecmp (tmp, "busy", 4))
                    ca->online_status = EXOSIP_NOTIFY_BUSY;
                  else if (tmp[0] == 'o'
                           && 0 == osip_strncasecmp (tmp, "on-the-phone", 12))
                    ca->online_status = EXOSIP_NOTIFY_ONTHEPHONE;

                  if (tmp[0] == '/' && 0 == osip_strncasecmp (tmp, "/status", 7))
                    break;
                  if (ca->online_status != EXOSIP_NOTIFY_ONLINE)
                    break;
                  tmp++;
                }

              while (tmp[0] != '\0')
                {
                  if (tmp[0] == 'c' && 0 == osip_strncasecmp (tmp, "contact", 7))
                    {
                      /* ... */
                    }
                  tmp++;
                }
              break;
          } else if (tmp[0] == 'c' && 0 == osip_strncasecmp (tmp, "closed", 6))
            {
              ca->online_status = EXOSIP_NOTIFY_CLOSED;
              break;
            }
          tmp++;
        }
  } else
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
                              "Unknown body: %s/%s\n",
                              ctype->type, ctype->subtype));
      return -1;
    }
  return 0;
}

int
jsubscription_get_number_of_pending_subscriptions ()
{
  int pos = 0;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED)
        {
          pos++;
        }
    }
  return pos;
}

jsubscription_t *
jsubscription_find_subscription (int pos)
{
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED)
        {
          if (pos == 0)
            return &(jsubscriptions[k]);
          pos--;
        }
    }
  return NULL;
}

int
jsubscription_new (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  if (___jsubscription_init == 0)
    {
      ___jsubscription_init = -1;
      __jsubscription_init ();
    }

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state == NOT_USED)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);
  memset (&(jsubscriptions[k]), 0, sizeof (jsubscription_t));

  ca->sid = je->sid;
  ca->did = je->did;

  if (ca->did < 1 && ca->did < 1)
    {
      exit (0);
      return -1;                /* not enough information for this event?? */
    }

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_from_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = je->type;
  return 0;
}

int
jsubscription_remove (jsubscription_t * ca)
{
  if (ca == NULL)
    return -1;
  ca->state = NOT_USED;
  return 0;
}

int
jsubscription_proceeding (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
        {
          if (jsubscriptions[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
        return -1;

      ca = &(jsubscriptions[k]);
      memset (&(jsubscriptions[k]), 0, sizeof (jsubscription_t));

      ca->sid = je->sid;
      ca->did = je->did;

      if (ca->did < 1 && ca->did < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(jsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = je->type;
  return 0;

}

int
jsubscription_answered (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
        {
          if (jsubscriptions[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
        return -1;
      ca = &(jsubscriptions[k]);
      memset (&(jsubscriptions[k]), 0, sizeof (jsubscription_t));

      ca->sid = je->sid;
      ca->did = je->did;

      if (ca->did < 1 && ca->did < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }

      ca = &(jsubscriptions[k]);
      ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
    }

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = je->type;
  return 0;
}

int
jsubscription_redirected (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = NOT_USED;
  return 0;
}

int
jsubscription_requestfailure (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }

  if ((je->response != NULL && je->response->status_code == 407)
      || (je->response != NULL && je->response->status_code == 401))
    {
      /* try authentication */
      return 0;
    }

  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = NOT_USED;
  return 0;
}

int
jsubscription_serverfailure (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = NOT_USED;
  return 0;
}

int
jsubscription_globalfailure (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  if (je->response != NULL)
    {
      ca->status_code = je->response->status_code;
      snprintf (ca->reason_phrase, 50, je->response->reason_phrase);
    }

  if (je->request != NULL)
    {
      char *tmp = NULL;

      osip_to_to_str (je->request->to, &tmp);
      if (tmp != NULL)
        {
          snprintf (ca->remote_uri, 256, "%s", tmp);
          osip_free (tmp);
        }
    }

  ca->state = NOT_USED;
  return 0;
}


int
jsubscription_notify (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED
          && jsubscriptions[k].sid == je->sid && jsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    {
      for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
        {
          if (jsubscriptions[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
        return -1;
      ca = &(jsubscriptions[k]);
      memset (&(jsubscriptions[k]), 0, sizeof (jsubscription_t));

      ca->sid = je->sid;
      ca->did = je->did;

      if (ca->did < 1 && ca->did < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(jsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;
  osip_strncpy (ca->textinfo, je->textinfo, 255);

  ca->state = je->type;

  jsubscriptions_set_presence_status (ca, je);

  return 0;
}

int
jsubscription_closed (eXosip_event_t * je)
{
  jsubscription_t *ca;
  int k;

  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED && jsubscriptions[k].sid == je->sid)
        break;
    }
  if (k == MAX_NUMBER_OF_SUBSCRIPTIONS)
    return -1;

  ca = &(jsubscriptions[k]);

  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}
