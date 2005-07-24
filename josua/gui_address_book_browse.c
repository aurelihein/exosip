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

#include "gui_address_book_browse.h"
#include "gui_address_book_newentry.h"

#include "gui_menu.h"
#include "gui_new_call.h"

#include "jfriends.h"

void __show_newentry_abook (void);

/*  extern eXosip_t eXosip; */

gui_t gui_window_address_book_browse = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_address_book_browse_print,
  &window_address_book_browse_run_command,
  NULL,
  &window_address_book_browse_draw_commands,
  -1,
  -1,
  -1,
  NULL
};


int cursor_address_book_browse = 0;
int cursor_address_book_start = 0;
int show_mail = -1;

void
window_address_book_browse_draw_commands ()
{
  int x, y;
  char *address_book_browse_commands[] = {
    "<-", "PrevWindow",
    "->", "NextWindow",
    "X", "StartCall",
    "a", "AddEntry",
    "d", "DeleteEntry",
    "t", "Toggle",
    NULL
  };
  getmaxyx (stdscr, y, x);
  josua_print_command (address_book_browse_commands, y - 5, 0);
}

int
window_address_book_browse_print ()
{
  jfriend_t *fr;
  int y, x;
  char buf[250];
  int pos;
  int pos_fr;

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);
  pos_fr = 0;

  for (fr = jfriend_get (); fr != NULL; fr = fr->next)
    {
      if (cursor_address_book_start == pos_fr)
        break;
      pos_fr++;
    }

  pos = 0;
  for (; fr != NULL; fr = fr->next)
    {
      if (pos == y + gui_window_address_book_browse.y1
          - gui_window_address_book_browse.y0)
        break;
      if (show_mail != 0)
        {
          snprintf (buf,
                    gui_window_address_book_browse.x1 -
                    gui_window_address_book_browse.x0,
                    "%c%c %-15.15s %-80.80s",
                    (cursor_address_book_browse == pos) ? '-' : ' ',
                    (cursor_address_book_browse == pos) ? '>' : ' ',
                    fr->f_nick, fr->f_home);
      } else
        {
          snprintf (buf,
                    gui_window_address_book_browse.x1 -
                    gui_window_address_book_browse.x0,
                    "%c%c %10.10s  %-80.80s",
                    (cursor_address_book_browse == pos) ? '-' : ' ',
                    (cursor_address_book_browse == pos) ? '>' : ' ',
                    fr->f_nick, fr->f_email);
        }
      attrset ((pos == cursor_address_book_browse) ? A_REVERSE : COLOR_PAIR (0));
      mvaddnstr (pos + gui_window_address_book_browse.y0,
                 gui_window_address_book_browse.x0, buf,
                 x - gui_window_address_book_browse.x0 - 1);
      pos++;
    }
  refresh ();

  window_address_book_browse_draw_commands ();
  return 0;
}

void
__show_newentry_abook ()
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI] == NULL)
    gui_windows[EXTRAGUI] = &gui_window_address_book_newentry;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands (gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI] = &gui_window_address_book_newentry;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_address_book_newentry_print ();
}


int
window_address_book_browse_run_command (int c)
{
  jfriend_t *fr;
  int y, x;
  int pos;
  int max;

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);

  max = 0;
  for (fr = jfriend_get (); fr != NULL; fr = fr->next)
    {
      max++;
    }

  switch (c)
    {
      case KEY_DOWN:
        /* cursor_address_book_browse++;
           cursor_address_book_browse %= max; */
        if (cursor_address_book_browse < y + gui_window_address_book_browse.y1
            - gui_window_address_book_browse.y0 - 1
            && cursor_address_book_browse < max - 1)
          cursor_address_book_browse++;
        else if (cursor_address_book_start <
                 max - (y + gui_window_address_book_browse.y1 -
                        gui_window_address_book_browse.y0))
          cursor_address_book_start++;
        else
          beep ();
        break;
      case KEY_UP:
        if (cursor_address_book_browse > 0)
          cursor_address_book_browse--;
        else if (cursor_address_book_start > 0)
          cursor_address_book_start--;
        else
          beep ();
        break;
      case '\n':
      case '\r':
      case KEY_ENTER:
      case 24:                 /* ctrl-X */
        /* start call */
        pos = 0;
        for (fr = jfriend_get (); fr != NULL; fr = fr->next)
          {
            if (cursor_address_book_browse + cursor_address_book_start == pos)
              break;
            pos++;
          }
        if (fr != NULL)
          {
            window_new_call_with_to (fr->f_home);
          }

        __show_initiate_session ();

        break;
      case 't':
        if (show_mail != 0)
          show_mail = 0;
        else
          show_mail = -1;
        break;
      case 'a':
        __show_newentry_abook ();
        break;
      case 'd':
        /* delete entry */
        pos = 0;
        for (fr = jfriend_get (); fr != NULL; fr = fr->next)
          {
            if (cursor_address_book_browse + cursor_address_book_start == pos)
              break;
            pos++;
          }
        if (fr != NULL)
          {
            jfriend_remove (fr);
          }
        break;
      default:
        beep ();
        return -1;
    }

  if (gui_window_address_book_browse.on_off == GUI_ON)
    window_address_book_browse_print ();
  return 0;
}
