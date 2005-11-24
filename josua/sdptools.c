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


#ifndef WIN32
#include <sys/socket.h>
#endif
#include <osip2/osip_mt.h>
#include "sdptools.h"
#include "jcalls.h"

int
sdp_complete_message (int did, sdp_message_t * remote_sdp, osip_message_t * msg)
{
  sdp_media_t *remote_med;
  char *tmp = NULL;
  char buf[4096];
  int pos;

  char localip[128];

  if (remote_sdp == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_WARNING, NULL,
                   "No remote SDP body found for call\n"));
      return -1;
    }
  if (msg == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_WARNING, NULL,
                   "No message to complete\n"));
      return -1;
    }

  eXosip_guess_localip (AF_INET, localip, 128);
  snprintf (buf, 4096,
            "v=0\r\n"
            "o=josua 0 0 IN IP4 %s\r\n"
            "s=conversation\r\n" "c=IN IP4 %s\r\n" "t=0 0\r\n", localip, localip);

  pos = 0;
  while (!osip_list_eol (remote_sdp->m_medias, pos))
    {
      char payloads[128];
      int pos2;

      memset (payloads, '\0', sizeof (payloads));
      remote_med = (sdp_media_t *) osip_list_get (remote_sdp->m_medias, pos);

      if (0 == osip_strcasecmp (remote_med->m_media, "audio"))
        {
          pos2 = 0;
          while (!osip_list_eol (remote_med->m_payloads, pos2))
            {
              tmp = (char *) osip_list_get (remote_med->m_payloads, pos2);
              if (tmp != NULL &&
                  (0 == osip_strcasecmp (tmp, "0")
                   || 0 == osip_strcasecmp (tmp, "8")))
                {
                  strcat (payloads, tmp);
                  strcat (payloads, " ");
                }
              pos2++;
            }
          strcat (buf, "m=");
          strcat (buf, remote_med->m_media);
          if (pos2 == 0 || payloads[0] == '\0')
            {
              strcat (buf, " 0 RTP/AVP \r\n");
              return -1;        /* refuse anyway */
          } else
            {
              strcat (buf, " 10500 RTP/AVP ");
              strcat (buf, payloads);
              strcat (buf, "\r\n");

#if 0
              if (NULL != strstr (payloads, " 0 ")
                  || (payloads[0] == '0' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:0 PCMU/8000/1\r\n");
              if (NULL != strstr (payloads, " 8 ")
                  || (payloads[0] == '8' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:8 PCMA/8000/1\r\n");
#else
              if (NULL != strstr (payloads, " 0 ")
                  || (payloads[0] == '0' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:0 PCMU/8000\r\n");
              if (NULL != strstr (payloads, " 8 ")
                  || (payloads[0] == '8' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:8 PCMA/8000\r\n");
#endif
            }
      } else
        {
          strcat (buf, "m=");
          strcat (buf, remote_med->m_media);
          strcat (buf, " 0 ");
          strcat (buf, remote_med->m_proto);
          strcat (buf, " \r\n");
        }
      pos++;
    }

  osip_message_set_body (msg, buf, strlen (buf));
  osip_message_set_content_type (msg, "application/sdp");
  return 0;
}

int
sdp_complete_200ok (int did, osip_message_t * answer)
{
  sdp_message_t *remote_sdp;
  sdp_media_t *remote_med;
  char *tmp = NULL;
  char buf[4096];
  int pos;

  char localip[128];


  remote_sdp = eXosip_get_remote_sdp (did);
  if (remote_sdp == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_WARNING, NULL,
                   "No remote SDP body found for call\n"));
      return -1;                /* no existing body? */
    }

  eXosip_guess_localip (AF_INET, localip, 128);
  snprintf (buf, 4096,
            "v=0\r\n"
            "o=josua 0 0 IN IP4 %s\r\n"
            "s=conversation\r\n" "c=IN IP4 %s\r\n" "t=0 0\r\n", localip, localip);

  pos = 0;
  while (!osip_list_eol (remote_sdp->m_medias, pos))
    {
      char payloads[128];
      int pos2;

      memset (payloads, '\0', sizeof (payloads));
      remote_med = (sdp_media_t *) osip_list_get (remote_sdp->m_medias, pos);

      if (0 == osip_strcasecmp (remote_med->m_media, "audio"))
        {
          pos2 = 0;
          while (!osip_list_eol (remote_med->m_payloads, pos2))
            {
              tmp = (char *) osip_list_get (remote_med->m_payloads, pos2);
              if (tmp != NULL &&
                  (0 == osip_strcasecmp (tmp, "0")
                   || 0 == osip_strcasecmp (tmp, "8")))
                {
                  strcat (payloads, tmp);
                  strcat (payloads, " ");
                }
              pos2++;
            }
          strcat (buf, "m=");
          strcat (buf, remote_med->m_media);
          if (pos2 == 0 || payloads[0] == '\0')
            {
              strcat (buf, " 0 RTP/AVP \r\n");
              sdp_message_free (remote_sdp);
              return -1;        /* refuse anyway */
          } else
            {
              strcat (buf, " 10500 RTP/AVP ");
              strcat (buf, payloads);
              strcat (buf, "\r\n");

#if 0
              if (NULL != strstr (payloads, " 0 ")
                  || (payloads[0] == '0' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:0 PCMU/8000/1\r\n");
              if (NULL != strstr (payloads, " 8 ")
                  || (payloads[0] == '8' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:8 PCMA/8000/1\r\n");
#else
              if (NULL != strstr (payloads, " 0 ")
                  || (payloads[0] == '0' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:0 PCMU/8000\r\n");
              if (NULL != strstr (payloads, " 8 ")
                  || (payloads[0] == '8' && payloads[1] == ' '))
                strcat (buf, "a=rtpmap:8 PCMA/8000\r\n");
#endif
            }
      } else
        {
          strcat (buf, "m=");
          strcat (buf, remote_med->m_media);
          strcat (buf, " 0 ");
          strcat (buf, remote_med->m_proto);
          strcat (buf, " \r\n");
        }
      pos++;
    }

  osip_message_set_body (answer, buf, strlen (buf));
  osip_message_set_content_type (answer, "application/sdp");
  sdp_message_free (remote_sdp);
  return 0;
}

int
_sdp_off_hold_call (sdp_message_t * sdp)
{
  int pos;
  int pos_media = -1;
  char *rcvsnd;

  pos = 0;
  rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
  while (rcvsnd != NULL)
    {
      if (rcvsnd != NULL && (0 == strcmp (rcvsnd, "sendonly")
                             || 0 == strcmp (rcvsnd, "recvonly")))
        {
          sprintf (rcvsnd, "sendrecv");
        }
      pos++;
      rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
    }

  pos_media = 0;
  while (!sdp_message_endof_media (sdp, pos_media))
    {
      pos = 0;
      rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
      while (rcvsnd != NULL)
        {
          if (rcvsnd != NULL && (0 == strcmp (rcvsnd, "sendonly")
                                 || 0 == strcmp (rcvsnd, "recvonly")))
            {
              sprintf (rcvsnd, "sendrecv");
            }
          pos++;
          rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
        }
      pos_media++;
    }

  return 0;
}

int
_sdp_hold_call (sdp_message_t * sdp)
{
  int pos;
  int pos_media = -1;
  char *rcvsnd;
  int recv_send = -1;

  pos = 0;
  rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
  while (rcvsnd != NULL)
    {
      if (rcvsnd != NULL && 0 == strcmp (rcvsnd, "sendonly"))
        {
          recv_send = 0;
      } else if (rcvsnd != NULL && (0 == strcmp (rcvsnd, "recvonly")
                                    || 0 == strcmp (rcvsnd, "sendrecv")))
        {
          recv_send = 0;
          sprintf (rcvsnd, "sendonly");
        }
      pos++;
      rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
    }

  pos_media = 0;
  while (!sdp_message_endof_media (sdp, pos_media))
    {
      pos = 0;
      rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
      while (rcvsnd != NULL)
        {
          if (rcvsnd != NULL && 0 == strcmp (rcvsnd, "sendonly"))
            {
              recv_send = 0;
          } else if (rcvsnd != NULL && (0 == strcmp (rcvsnd, "recvonly")
                                        || 0 == strcmp (rcvsnd, "sendrecv")))
            {
              recv_send = 0;
              sprintf (rcvsnd, "sendonly");
            }
          pos++;
          rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
        }
      pos_media++;
    }

  if (recv_send == -1)
    {
      /* we need to add a global attribute with a field set to "sendonly" */
      sdp_message_a_attribute_add (sdp, -1, osip_strdup ("sendonly"), NULL);
    }

  return 0;
}

int
_sdp_analyse_attribute (sdp_message_t * sdp, sdp_media_t * med)
{
  int pos;
  int pos_media;

  /* test media attributes */
  pos = 0;
  while (!osip_list_eol (med->a_attributes, pos))
    {
      sdp_attribute_t *at;

      at = (sdp_attribute_t *) osip_list_get (med->a_attributes, pos);
      if (at->a_att_field != NULL && 0 == strcmp (at->a_att_field, "sendonly"))
        {
          return _SENDONLY;
      } else if (at->a_att_field != NULL &&
                 0 == strcmp (at->a_att_field, "recvonly"))
        {
          return _RECVONLY;
      } else if (at->a_att_field != NULL &&
                 0 == strcmp (at->a_att_field, "sendrecv"))
        {
          return _SENDRECV;
        }
      pos++;
    }

  /* test global attributes */
  pos_media = -1;
  pos = 0;
  while (!osip_list_eol (sdp->a_attributes, pos))
    {
      sdp_attribute_t *at;

      at = (sdp_attribute_t *) osip_list_get (sdp->a_attributes, pos);
      if (at->a_att_field != NULL && 0 == strcmp (at->a_att_field, "sendonly"))
        {
          return _SENDONLY;
      } else if (at->a_att_field != NULL &&
                 0 == strcmp (at->a_att_field, "recvonly"))
        {
          return _RECVONLY;
      } else if (at->a_att_field != NULL &&
                 0 == strcmp (at->a_att_field, "sendrecv"))
        {
          return _SENDRECV;
        }
      pos++;
    }

  return _SENDRECV;
}
