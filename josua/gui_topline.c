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


#include "gui_topline.h"


gui_t gui_window_topline = {
  GUI_DISABLED,
  0,
  -999,
  0,
  1,
  NULL,
  &window_topline_print,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  -1,
  NULL
};

int
window_topline_print ()
{
  int y, x;
  char buf[250];

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  sprintf (buf,
           "Josua 0.6.2 \\\\//                                     Powered by eXosip/osip2. %-50.50s",
           " ");
  getmaxyx (stdscr, y, x);
  attrset (A_NORMAL);
  attrset (COLOR_PAIR (1));
  mvaddnstr (gui_window_topline.y0, gui_window_topline.x0, buf, x - 1);
  return 0;
}
