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
  19,
  2,
  9,
  NULL,
  &window_icon_print,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  -1
};

int window_icon_print()
{
  int y,x;
  char buf[250];
  int i;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  for (i=gui_window_icon.y0; i<gui_window_icon.y1; i++)
    {
      snprintf(buf, gui_window_icon.x1 - gui_window_icon.x0,
	       "%s",icons[i-gui_window_icon.y0]);
      mvaddnstr(i,
		gui_window_icon.x0,
		buf,
		gui_window_icon.x1-1);
    }
  return 0;
}

