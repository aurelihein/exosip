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

#include "gui_menu.h"
#include "gui_new_call.h"
#include "gui_manage_call.h"

gui_t gui_window_menu = {
  GUI_OFF,
  20,
  -999,
  2,
  9,
  NULL,
  &window_menu_print,
  &window_menu_run_command,
  NULL,
  NULL,
  -1,
  -1,
  -1
};

static const menu_t josua_menu[]= {
  { "v", " Start a Voice Conversation.",          &__show_new_call },
  { "c", " Start a Chat Session.",                &__show_new_message },
  { "m", " Manage Pending Calls.",                &__josua_manage_call },
  { "r", " Manage Pending Registrations.",        &__josua_register  },
  { "u", " Manage Pending Subscriptions.",        &__josua_manage_subscribers },
  { "s", " Josua's Setup.",                       &__josua_setup  },
  { "q", " Quit jack' Open Sip User Agent.",      &__josua_quit  },
  { 0 }
};

int cursor_menu = 0;

int window_menu_print()
{
  int y,x;
  char buf[250];
  int i;
  int pos;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  pos = 0;
  for (i=gui_window_menu.y0; i<gui_window_menu.y1; i++)
    {
      snprintf(buf, gui_window_menu.x1 - gui_window_menu.x0,
	      "%c%c %d. %-80.80s ",
	      (cursor_menu==pos) ? '-' : ' ',
	      (cursor_menu==pos) ? '>' : ' ',
	      i-gui_window_menu.y0,
	      josua_menu[i-gui_window_menu.y0].text);

      attrset(COLOR_PAIR(5));
      attrset((pos==cursor_menu) ? A_REVERSE : A_NORMAL);
      mvaddnstr(i,
		gui_window_menu.x0,
		buf,
		x-gui_window_menu.x0-1);
      pos++;
    }
  return 0;
}

int window_menu_run_command(int c)
{
  int max = 7;
  switch (c)
    {
    case KEY_DOWN:
      cursor_menu++;
      cursor_menu %= max;
      break;
    case KEY_UP:
      cursor_menu += max-1;
      cursor_menu %= max;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
      cursor_menu = c-48;
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
      /* menu selected! */
      josua_menu[cursor_menu].fn();
      break;

    default:
      beep();
      return -1;
    }

  window_menu_print();
  return 0;
}

void
__show_new_call()
{
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
}


void
__josua_manage_call()
{
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
}

void __show_new_message()
{

}

void __josua_register()
{

}

void __josua_setup()
{

}

void __josua_manage_subscribers()
{

}

void __josua_quit() {

  eXosip_quit();

  cursesoff();  
  exit(1);
}

