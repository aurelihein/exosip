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

#include "gui_icon.h"

static const char *icons[]= {
#define JD_EMPTY          0
    "             ___  " ,
    " WELCOME    /  /  " ,
    "           /__/   " ,
    "   TO     //      " ,
    "         //       " ,
    " JOSUA  / \\_      " ,
    "        |__/      "
};

gui_t gui_window_icon = {
  GUI_DISABLED,
  0,
  20,
  1,
  10,
  NULL,
  &window_icon_print,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  -1,
  NULL
};

int window_icon_print()
{
  int y,x;
  char buf[250];
  int i;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  gui_print_box(&gui_window_icon, -1, 1);

  getmaxyx(gui_window_icon.win,y,x);
  wattrset(gui_window_icon.win, A_NORMAL);
  wattrset(gui_window_icon.win, COLOR_PAIR(1));

  for (i=1; i<y-1; i++)
    {
      snprintf(buf, x,
	       "%s",icons[i-1]);
      mvaddnstr(i+1,
		0,
		buf,
		x-1);
    }
  return 0;
}

