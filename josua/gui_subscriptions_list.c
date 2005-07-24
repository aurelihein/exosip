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
#include "gui_subscriptions_list.h"
#include "gui_insubscriptions_list.h"

gui_t gui_window_subscriptions_list = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_subscriptions_list_print,
  &window_subscriptions_list_run_command,
  NULL,
  window_subscriptions_list_draw_commands,
  -1,
  -1,
  -1,
  NULL
};

int cursor_subscriptions_list = 0;

void __show_insubscriptions_list (void);

int
window_subscriptions_list_print ()
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

  if (gui_window_subscriptions_list.x1 == -999)
    {
  } else
    x = gui_window_subscriptions_list.x1;

  attrset (COLOR_PAIR (0));

  pos = 1;
  for (k = 0; k < MAX_NUMBER_OF_SUBSCRIPTIONS; k++)
    {
      if (jsubscriptions[k].state != NOT_USED)
        {
          char *tmp;

          snprintf (buf, 199, "%c%c %3.3i//%3.3i    %-40.40s ",
                    (cursor_subscriptions_list == pos - 1) ? '-' : ' ',
                    (cursor_subscriptions_list == pos - 1) ? '>' : ' ',
                    jsubscriptions[k].sid,
                    jsubscriptions[k].did, jsubscriptions[k].remote_uri);
          tmp = buf + strlen (buf);
          if (jsubscriptions[k].ss_status == EXOSIP_SUBCRSTATE_UNKNOWN)
            snprintf (tmp, 199 - strlen (buf), " %14.14s", "--unknown--");
          else if (jsubscriptions[k].ss_status == EXOSIP_SUBCRSTATE_PENDING)
            snprintf (tmp, 199 - strlen (buf), " %14.14s", "--pending--");
          else if (jsubscriptions[k].ss_status == EXOSIP_SUBCRSTATE_ACTIVE)
            {
              if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_UNKNOWN)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "unknown");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_PENDING)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "pending");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_ONLINE)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "online");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_BUSY)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "Busy");
              else if (jsubscriptions[k].online_status ==
                       EXOSIP_NOTIFY_BERIGHTBACK)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "Be Right Back");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_AWAY)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "away");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_ONTHEPHONE)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "On The Phone");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_OUTTOLUNCH)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "Out To Lunch");
              else if (jsubscriptions[k].online_status == EXOSIP_NOTIFY_CLOSED)
                snprintf (tmp, 199 - strlen (buf), " %14.14s", "Closed");
          } else if (jsubscriptions[k].ss_status == EXOSIP_SUBCRSTATE_TERMINATED)
            snprintf (tmp, 199 - strlen (buf), " %14.14s", "--terminated--");

          tmp = buf + strlen (buf);
          snprintf (tmp, 199 - strlen (buf), " %100.100s", " ");

          attrset (COLOR_PAIR (5));
          attrset ((pos - 1 == cursor_subscriptions_list) ? A_REVERSE : A_NORMAL);

          mvaddnstr (gui_window_subscriptions_list.y0 + pos - 1,
                     gui_window_subscriptions_list.x0,
                     buf, x - gui_window_subscriptions_list.x0 - 1);
          pos++;
        }
      if (pos >
          y + gui_window_subscriptions_list.y1 -
          gui_window_subscriptions_list.y0 + 1)
        break;                  /* do not print next one */
    }

  window_subscriptions_list_draw_commands ();
  return 0;
}

void
__show_insubscriptions_list (void)
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI] == NULL)
    gui_windows[EXTRAGUI] = &gui_window_insubscriptions_list;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands (gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI] = &gui_window_insubscriptions_list;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_insubscriptions_list_print ();
}

void
window_subscriptions_list_draw_commands ()
{
  int x, y;
  char *subscriptions_list_commands[] = {
    "<-", "PrevWindow",
    "->", "NextWindow",
    "r", "Refresh",
    "c", "Close",
    "t", "ViewSubscribers",
    NULL
  };
  getmaxyx (stdscr, y, x);
  josua_print_command (subscriptions_list_commands, y - 5, 0);
}

int
window_subscriptions_list_run_command (int c)
{
  jsubscription_t *js;
  int i;
  int max;
  int y, x;

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);

  if (gui_window_subscriptions_list.x1 == -999)
    {
  } else
    x = gui_window_subscriptions_list.x1;

  if (gui_window_subscriptions_list.y1 < 0)
    max =
      y + gui_window_subscriptions_list.y1 - gui_window_subscriptions_list.y0 + 2;
  else
    max = gui_window_subscriptions_list.y1 - gui_window_subscriptions_list.y0 + 2;

  i = jsubscription_get_number_of_pending_subscriptions ();
  if (i < max)
    max = i;

  if (max == 0 && c != 't')
    {
      beep ();
      return -1;
    }

  switch (c)
    {
      case KEY_DOWN:
        cursor_subscriptions_list++;
        cursor_subscriptions_list %= max;
        break;
      case KEY_UP:
        cursor_subscriptions_list += max - 1;
        cursor_subscriptions_list %= max;
        break;

      case 't':
        __show_insubscriptions_list ();
        break;

      case 'r':
        js = jsubscription_find_subscription (cursor_subscriptions_list);
        if (js == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *sub = NULL;

          i = eXosip_subscribe_build_refresh_request (js->did, &sub);
          if (i == 0 && sub != NULL)
            {
#ifdef SUPPORT_MSN
              osip_message_set_accept (sub, "application/xpidf+xml");
#else
              osip_message_set_accept (sub, "application/pidf+xml");
#endif
              osip_message_set_header (sub, "Event", "presence");
              osip_message_set_expires (sub, "900");

              i = eXosip_subscribe_send_refresh_request (js->did, sub);
            }
          if (i != 0)
            beep ();
        }
        eXosip_unlock ();
        window_subscriptions_list_print ();
        break;
      case 'c':
        js = jsubscription_find_subscription (cursor_subscriptions_list);
        if (js == NULL)
          {
            beep ();
            break;
          }
        eXosip_lock ();
        {
          osip_message_t *sub = NULL;

          i = eXosip_subscribe_build_refresh_request (js->did, &sub);
          if (i == 0 && sub != NULL)
            {
#ifdef SUPPORT_MSN
              osip_message_set_accept (sub, "application/xpidf+xml");
#else
              osip_message_set_accept (sub, "application/pidf+xml");
#endif
              osip_message_set_header (sub, "Event", "presence");
              osip_message_set_expires (sub, "0");

              i = eXosip_subscribe_send_refresh_request (js->did, sub);
            }
        }
        eXosip_unlock ();
        if (i == 0)
          jsubscription_remove (js);
        window_subscriptions_list_print ();
        break;
      default:
        beep ();
        return -1;
    }

  if (gui_window_subscriptions_list.on_off == GUI_ON)
    window_subscriptions_list_print ();
  return 0;
}
