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

#include "gui_address_book_menu.h"

gui_t gui_window_address_book_menu = {
  GUI_OFF,
  20,
  -999,
  2,
  9,
  NULL,
  &window_address_book_menu_print,
  &window_address_book_menu_run_command,
  NULL,
  NULL,
  -1,
  -1,
  -1
};

static const menu_t josua_address_book_menu[]= {
  { "a", " Add New Freind.",                    &__show_newentry_abook },
  { "s", " Browse Address Book.",               &__show_browse_abook },
  { "q", " Main Menu.",                         &__show_main_menu  },
  { 0 }
};

int cursor_address_book_menu = 0;

int window_address_book_menu_print()
{
  int y,x;
  char buf[250];
  int i;
  int pos;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  pos = 0;
  for (i=gui_window_address_book_menu.y0; i<gui_window_address_book_menu.y1; i++)
    {
      if (i==gui_window_address_book_menu.y0+3)
	break;
      snprintf(buf, gui_window_address_book_menu.x1 - gui_window_address_book_menu.x0,
	      "%c%c %d. %-80.80s ",
	      (cursor_address_book_menu==pos) ? '-' : ' ',
	      (cursor_address_book_menu==pos) ? '>' : ' ',
	      i-gui_window_address_book_menu.y0,
	      josua_address_book_menu[i-gui_window_address_book_menu.y0].text);

      attrset(COLOR_PAIR(5));
      attrset((pos==cursor_address_book_menu) ? A_REVERSE : A_NORMAL);
      mvaddnstr(i,
		gui_window_address_book_menu.x0,
		buf,
		x-gui_window_address_book_menu.x0-1);
      pos++;
    }
  refresh();
  return 0;
}

int window_address_book_menu_run_command(int c)
{
  int max = 3;
  switch (c)
    {
    case KEY_DOWN:
      cursor_address_book_menu++;
      cursor_address_book_menu %= max;
      break;
    case KEY_UP:
      cursor_address_book_menu += max-1;
      cursor_address_book_menu %= max;
      break;
    case '0':
    case '1':
    case '2':
      cursor_address_book_menu = c-48;
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
      /* address_book_menu selected! */
      josua_address_book_menu[cursor_address_book_menu].fn();
      break;

    default:
      beep();
      return -1;
    }

  if (gui_window_address_book_menu.on_off==GUI_ON)
    window_address_book_menu_print();
  return 0;
}

void
__show_newentry_abook()
{
  /*
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI]==NULL)
    gui_windows[EXTRAGUI]= &gui_window_new_call;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI]= &gui_window_new_call;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_new_call_print();
  */
}


void
__show_browse_abook()
{
  /*
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI]==NULL)
    gui_windows[EXTRAGUI]= &gui_window_manage_call;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI]= &gui_window_manage_call;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_manage_call_print();
  */
}

void
__show_main_menu()
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[MENUGUI]==NULL)
    gui_windows[MENUGUI]= &gui_window_menu;
  else
    {
      josua_clear_box_and_commands(gui_windows[MENUGUI]);
      gui_windows[MENUGUI]->on_off = GUI_OFF;
      gui_windows[MENUGUI]= &gui_window_menu;
    }

  if (gui_windows[EXTRAGUI]!=NULL)
    {
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI] = NULL;
    }

  active_gui = gui_windows[MENUGUI];
  active_gui->on_off = GUI_ON;

  window_menu_print();
}
