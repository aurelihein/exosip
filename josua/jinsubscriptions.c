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

#include "jinsubscriptions.h"

#include <sys/types.h>
#include <sys/socket.h>

jinsubscription_t jinsubscriptions[MAX_NUMBER_OF_INSUBSCRIPTIONS];

static int ___jinsubscription_init = 0;


static int
__jinsubscription_complete_notify (osip_message_t * notify, int ss_status,
                                   int online_status);

static void
__jinsubscription_init ()
{
  int k;

  if (___jinsubscription_init == 0)
    {
      ___jinsubscription_init = -1;
      for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
        {
          memset (&(jinsubscriptions[k]), 0, sizeof (jinsubscription_t));
          jinsubscriptions[k].state = NOT_USED;
        }
    }
}

static int
__jinsubscription_complete_notify (osip_message_t * notify, int ss_status,
                                   int online_status)
{
  char buf[1000];

  char *contact_info;
  char localip[128];
  int i;

  eXosip_guess_localip (AF_INET, localip, 128);

  if (notify == NULL || notify->from == NULL || notify->from->url == NULL)
    return -1;                  /* bug */

  contact_info = NULL;
  i = osip_uri_to_str (notify->from->url, &contact_info);
  if (i != 0)
    return -1;

#ifdef SUPPORT_MSN
  int atom_id = 1000;
#endif

  if (ss_status != EXOSIP_SUBCRSTATE_ACTIVE || contact_info == NULL || contact_info == '\0')    /* mandatory! */
    {
      if (contact_info != NULL)
        osip_free (contact_info);
      return 0;                 /* don't need a body? */
    }
#ifdef SUPPORT_MSN

  if (online_status == EXOSIP_NOTIFY_ONLINE)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else if (online_status == EXOSIP_NOTIFY_BUSY)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else if (online_status == EXOSIP_NOTIFY_BERIGHTBACK)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else if (online_status == EXOSIP_NOTIFY_AWAY)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else if (online_status == EXOSIP_NOTIFY_ONTHEPHONE)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else if (online_status == EXOSIP_NOTIFY_OUTTOLUNCH)
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

  } else
    {
      sprintf (buf, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence\n\
PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s;user=ip\" priority=\"0.800000\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
    }

  osip_message_set_body (notify, buf, strlen (buf));
  osip_message_set_content_type (notify, "application/xpidf+xml");
#else

  if (online_status == EXOSIP_NOTIFY_ONLINE)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>online</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else if (online_status == EXOSIP_NOTIFY_BUSY)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>busy</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>busy</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else if (online_status == EXOSIP_NOTIFY_BERIGHTBACK)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>in-transit</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>be right back</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else if (online_status == EXOSIP_NOTIFY_AWAY)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>away</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>away</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else if (online_status == EXOSIP_NOTIFY_ONTHEPHONE)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>on-the-phone</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>on the phone</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else if (online_status == EXOSIP_NOTIFY_OUTTOLUNCH)
    {
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>meal</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>out to lunch</note\n\
</tuple>\n\
</presence>", contact_info, contact_info);
  } else
    {
      /* */
      sprintf (buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
entity=\"%s\">\n%s", contact_info, "<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>closed</basic>\n\
<es:activities>\n\
  <es:activity>permanent-absence</e:activity>\n\
</es:activities>\n\
</status>\n\
</tuple>\n\
\n</presence>\n");
    }
  osip_message_set_body (notify, buf, strlen (buf));
  osip_message_set_content_type (notify, "application/pidf+xml");

#endif

  osip_free (contact_info);
  return 0;
}

int
__jinsubscription_send_notify (int did, int ss_status,
                               int ss_reason, int online_status)
{
  osip_message_t *notify;
  int i;

  eXosip_lock ();
  i = eXosip_insubscription_build_notify (did, ss_status, ss_reason, &notify);
  if (i == 0)
    {
      __jinsubscription_complete_notify (notify, ss_status, online_status);
      i = eXosip_insubscription_send_request (did, notify);
    }
  eXosip_unlock ();

  return i;
}

int
jinsubscription_get_number_of_pending_insubscriptions ()
{
  int pos = 0;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED)
        {
          pos++;
        }
    }
  return pos;
}

jinsubscription_t *
jinsubscription_find_insubscription (int pos)
{
  int k;

  __jinsubscription_init ();

  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED)
        {
          if (pos == 0)
            return &(jinsubscriptions[k]);
          pos--;
        }
    }
  return NULL;
}

int
jinsubscription_new (eXosip_event_t * je)
{
  osip_message_t *answer;

  jinsubscription_t *ca;
  int k;
  int i;

  __jinsubscription_init ();

  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid)
        break;
    }
  if (k != MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      int code;

      /* new subscrib for existing subscription */
      ca = &(jinsubscriptions[k]);

      ca->tid = je->tid;

      ca->ss_status = je->ss_status;
      ca->ss_reason = je->ss_reason;

      code = 202;
      if (ca->ss_status == EXOSIP_SUBCRSTATE_ACTIVE)
        code = 200;
      if (ca->ss_status == EXOSIP_SUBCRSTATE_TERMINATED)
        code = 200;

      eXosip_lock ();
      i = eXosip_insubscription_build_answer (je->tid, code, &answer);
      if (i == 0)
        {
          if (ca->ss_status == EXOSIP_SUBCRSTATE_TERMINATED)
            {
              if (answer->reason_phrase != NULL)
                osip_free (answer->reason_phrase);
              answer->reason_phrase = osip_strdup ("Closed Subscription");
            }
          i = eXosip_insubscription_send_answer (je->tid, code, answer);
          if (i != 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "Could not send answer %i for subscribe\n", code));
            }
      } else
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Could not build answer %i for subscribe\n", code));
        }
      eXosip_unlock ();

      if (i == 0)
        {
          i =
            __jinsubscription_send_notify (je->did, ca->ss_status,
                                           ca->ss_reason, ca->online_status);
          if (i != 0)
            {
              OSIP_TRACE (osip_trace
                          (__FILE__, __LINE__, OSIP_ERROR, NULL,
                           "Could not build or send NOTIFY for subscribe\n"));
            }
        }

      if (ca->ss_status == EXOSIP_SUBCRSTATE_TERMINATED)
        ca->state = NOT_USED;

      return 0;
    }

  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state == NOT_USED)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  if (je->did < 1 && je->nid < 1)
    {
      return -1;                /* not enough information for this event?? */
    }

  ca = &(jinsubscriptions[k]);
  memset (&(jinsubscriptions[k]), 0, sizeof (jinsubscription_t));

  ca->nid = je->nid;
  ca->did = je->did;
  ca->tid = je->tid;

  eXosip_lock ();
  i = eXosip_insubscription_build_answer (je->tid, 202, &answer);
  if (i == 0)
    {
      i = eXosip_insubscription_send_answer (je->tid, 202, answer);
      if (i != 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Could not send answer 202 for subscribe\n"));
        }
  } else
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "Could not build answer 202 for subscribe\n"));
    }
  eXosip_unlock ();

  if (i == 0)
    {
      i = __jinsubscription_send_notify (je->did, EXOSIP_SUBCRSTATE_PENDING,
                                         ca->ss_reason, EXOSIP_NOTIFY_AWAY);
      if (i != 0)
        {
          OSIP_TRACE (osip_trace
                      (__FILE__, __LINE__, OSIP_ERROR, NULL,
                       "Could not send NOTIFY for subscribe\n"));
        }
    }


  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  ca->state = je->type;
  return 0;
}

int
jinsubscription_remove (jinsubscription_t * ca)
{
  __jinsubscription_init ();
  if (ca == NULL)
    return -1;
  ca->state = NOT_USED;
  return 0;
}

int
jinsubscription_proceeding (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
        {
          if (jinsubscriptions[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
        return -1;

      ca = &(jinsubscriptions[k]);
      memset (&(jinsubscriptions[k]), 0, sizeof (jinsubscription_t));

      ca->nid = je->nid;
      ca->did = je->did;

      if (ca->did < 1 && ca->did < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  ca->state = je->type;
  return 0;

}

int
jinsubscription_answered (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    {
      for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
        {
          if (jinsubscriptions[k].state == NOT_USED)
            break;
        }
      if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
        return -1;
      ca = &(jinsubscriptions[k]);
      memset (&(jinsubscriptions[k]), 0, sizeof (jinsubscription_t));

      ca->nid = je->nid;
      ca->did = je->did;

      if (ca->did < 1 && ca->did < 1)
        {
          exit (0);
          return -1;            /* not enough information for this event?? */
        }
    }

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  osip_strncpy (ca->textinfo, je->textinfo, 255);

  ca->state = je->type;
  return 0;
}

int
jinsubscription_redirected (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int
jinsubscription_requestfailure (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int
jinsubscription_serverfailure (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int
jinsubscription_globalfailure (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid
          && jinsubscriptions[k].did == je->did)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}

int
jinsubscription_closed (eXosip_event_t * je)
{
  jinsubscription_t *ca;
  int k;

  __jinsubscription_init ();
  for (k = 0; k < MAX_NUMBER_OF_INSUBSCRIPTIONS; k++)
    {
      if (jinsubscriptions[k].state != NOT_USED
          && jinsubscriptions[k].nid == je->nid)
        break;
    }
  if (k == MAX_NUMBER_OF_INSUBSCRIPTIONS)
    return -1;

  ca = &(jinsubscriptions[k]);

  ca->online_status = EXOSIP_NOTIFY_UNKNOWN;
  ca->ss_status = je->ss_status;
  ca->ss_reason = je->ss_reason;

  ca->state = NOT_USED;
  return 0;
}
