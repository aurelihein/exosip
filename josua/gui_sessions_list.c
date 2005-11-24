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

#include "jcalls.h"
#include "gui_sessions_list.h"

#include "sdptools.h"

gui_t gui_window_sessions_list = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_sessions_list_print,
  &window_sessions_list_run_command,
  NULL,
  window_sessions_list_draw_commands,
  -1,
  -1,
  -1,
  NULL
};

int cursor_sessions_list = 0;

int
window_sessions_list_print ()
{
  int k, pos;
  int y, x;
  char buf[250];

  josua_clear_box_and_commands (gui_windows[EXTRAGUI]);

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);
  attrset (A_NORMAL);
  attrset (COLOR_PAIR (1));

  if (gui_window_sessions_list.x1 == -999)
    {
  } else
    x = gui_window_sessions_list.x1;

  attrset (COLOR_PAIR (0));

  pos = 1;
  for (k = 0; k < MAX_NUMBER_OF_CALLS; k++)
    {
      if (calls[k].state != NOT_USED)
        {
          snprintf (buf, 199, "%c%c %i//%i %i %s with: %s %199.199s",
                    (cursor_sessions_list == pos - 1) ? '-' : ' ',
                    (cursor_sessions_list == pos - 1) ? '>' : ' ',
                    calls[k].cid,
                    calls[k].did,
                    calls[k].status_code,
                    calls[k].reason_phrase, calls[k].remote_uri, " ");

          attrset (COLOR_PAIR (5));
          attrset ((pos - 1 == cursor_sessions_list) ? A_REVERSE : A_NORMAL);

          mvaddnstr (gui_window_sessions_list.y0 + pos - 1,
                     gui_window_sessions_list.x0,
                     buf, x - gui_window_sessions_list.x0 - 1);
          pos++;
        }
      if (pos > y + gui_window_sessions_list.y1 - gui_window_sessions_list.y0 + 1)
        break;                  /* do not print next one */
    }

  window_sessions_list_draw_commands ();
  return 0;
}

void
window_sessions_list_draw_commands ()
{
  int x, y;
  char *sessions_list_commands[] = {
    "<-", "PrevWindow",
    "->", "NextWindow",
    "c", "Close",
    "a", "Answer",
    "m", "Mute",
    "u", "UnMute",
    "d", "Decline",
    "r", "Reject",
    "b", "AppearBusy",
    "o", "SendOptions",
    "R", "Transfer",
    "digit", "SendInfo",
    NULL
  };
  getmaxyx (stdscr, y, x);
  josua_print_command (sessions_list_commands, y - 5, 0);
}

int
window_sessions_list_run_command (int c)
{
  call_t *ca;
  int i;
  int max;
  int y, x;

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);

  if (gui_window_sessions_list.x1 == -999)
    {
  } else
    x = gui_window_sessions_list.x1;

  if (gui_window_sessions_list.y1 < 0)
    max = y + gui_window_sessions_list.y1 - gui_window_sessions_list.y0 + 2;
  else
    max = gui_window_sessions_list.y1 - gui_window_sessions_list.y0 + 2;

  i = call_get_number_of_pending_calls ();
  if (i < max)
    max = i;

  if (max == 0)
    {
      beep ();
      return -1;
    }

  switch (c)
    {
      case KEY_DOWN:
        cursor_sessions_list++;
        cursor_sessions_list %= max;
        break;
      case KEY_UP:
        cursor_sessions_list += max - 1;
        cursor_sessions_list %= max;
        break;

      case 'a':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *answer = NULL;

          i = eXosip_call_build_answer (ca->tid, 200, &answer);
          if (i != 0)
            {
              eXosip_call_send_answer (ca->tid, 400, NULL);
              beep ();
          } else
            {
              i = sdp_complete_200ok (ca->did, answer);
              if (i != 0)
                {
                  osip_message_free (answer);
                  eXosip_call_send_answer (ca->tid, 415, NULL);
                  beep ();
              } else
                eXosip_call_send_answer (ca->tid, 200, answer);
            }
        }
        eXosip_unlock ();
        break;
      case 'r':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        i = eXosip_call_send_answer (ca->tid, 480, NULL);
        if (i == 0)
          call_remove (ca);
        eXosip_unlock ();
        window_sessions_list_print ();
        break;
      case 'd':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        i = eXosip_call_send_answer (ca->tid, 603, NULL);
        if (i == 0)
          call_remove (ca);
        eXosip_unlock ();
        window_sessions_list_print ();
        break;
      case 'b':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        i = eXosip_call_send_answer (ca->tid, 486, NULL);
        if (i == 0)
          call_remove (ca);
        eXosip_unlock ();
        window_sessions_list_print ();
        break;
      case 'c':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        i = eXosip_call_terminate (ca->cid, ca->did);
        if (i == 0)
          call_remove (ca);
        eXosip_unlock ();
        window_sessions_list_print ();
        break;
      case 'm':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        i = _josua_mute_call (ca->did, NULL);
        if (i != 0)
          beep ();
        break;
      case 'u':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        i = _josua_unmute_call (ca->did);
        if (i != 0)
          beep ();
        break;
      case 'o':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *options;

          i = eXosip_call_build_options (ca->did, &options);
          if (i == 0)
            {
              i = eXosip_call_send_request (ca->did, options);
            }
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;
      case 'R':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *refer;

          i =
            eXosip_call_build_refer (ca->did, "sip:800@192.168.2.2:5070", &refer);
          osip_message_set_header (refer, "Referred-By", cfg.identity);
          if (i == 0)
            i = eXosip_call_send_request (ca->did, refer);
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;
      case 'E':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *notify;

          i =
            eXosip_call_build_notify (ca->did, EXOSIP_SUBCRSTATE_TERMINATED,
                                      &notify);
          if (i == 0)
            {
              osip_message_set_header (notify, "Event", "refer");
              osip_message_set_content_type (notify, "message/sipfrag");
              osip_message_set_body (notify, "SIP/2.0 200 Ok",
                                     strlen ("SIP/2.0 200 Ok"));
              i = eXosip_call_send_request (ca->did, notify);

            }
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;
      case 'T':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *refer;

          i =
            eXosip_call_build_refer (ca->did, "sips:800@192.168.2.2:5000", &refer);
          if (i == 0)
            i = eXosip_call_send_request (ca->did, refer);
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;
      case 'Y':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *refer;

          i = eXosip_call_build_refer (ca->did, "http://antisip.com", &refer);
          if (i == 0)
            i = eXosip_call_send_request (ca->did, refer);
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;
      case 'U':
        ca = call_find_call (cursor_sessions_list);
        if (ca == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *refer;

          i = eXosip_call_build_refer (ca->did, "sipoleantisip.com", &refer);
          if (i == 0)
            i = eXosip_call_send_request (ca->did, refer);
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '#':
      case '*':
        {
          char dtmf_body[1000];

          ca = call_find_call (cursor_sessions_list);
          if (ca == NULL)
            {
              beep ();
              break;
            }
          eXosip_lock ();
          {
            osip_message_t *info;

            i = eXosip_call_build_info (ca->did, &info);
            if (i == 0)
              {
                snprintf (dtmf_body, 999, "Signal=%c\r\nDuration=250\r\n", c);
                osip_message_set_content_type (info, "application/dtmf-relay");
                osip_message_set_body (info, dtmf_body, strlen (dtmf_body));
                i = eXosip_call_send_request (ca->did, info);
              }
            if (i != 0)
              beep ();
          }
          eXosip_unlock ();
          break;
        }
      default:
        beep ();
        return -1;
    }

  window_sessions_list_print ();
  return 0;
}
