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


#include "gui_loglines.h"

gui_t gui_window_loglines = {
  GUI_DISABLED,
  0,
  -999,
  -3,
  0,
  NULL,
  &window_loglines_print,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  -1,
  NULL
};

char log_buf3[200] = { '\0' };
char log_buf2[200] = { '\0' };
char log_buf1[200] = { '\0' };
struct osip_mutex *log_mutex = NULL;


int window_loglines_print()
{
  char buf1[200];
  int x, y;
  getmaxyx(stdscr,y,x);

  if (log_buf1!='\0')
    {
      /* int xpos; */
      osip_mutex_lock(log_mutex);
      snprintf(buf1,199, "%199.199s", " ");
      attrset(COLOR_PAIR(4));
      mvaddnstr(y-1,0,buf1,x-1);
      /* xpos = (x - strlen(log_buf1))/2;
	 if (xpos<0)
	 xpos = 0; */
      /* mvaddnstr(y-1,xpos,log_buf1,x-1); */
      mvaddnstr(y-1,0,log_buf1,x-1);
      osip_mutex_unlock(log_mutex);
    }
  if (log_buf2!='\0')
    {
      /* int xpos; */
      osip_mutex_lock(log_mutex);
      snprintf(buf1,199, "%199.199s", " ");
      attrset(COLOR_PAIR(4));
      mvaddnstr(y-2,0,buf1,x-1);
      /* xpos = (x - strlen(log_buf2))/2;
	 if (xpos<0)
	 xpos = 0; */
      /* mvaddnstr(y-2,xpos,log_buf2,x-1); */
      mvaddnstr(y-2,0,log_buf2,x-1);
      osip_mutex_unlock(log_mutex);
    }
  if (log_buf3!='\0')
    {
      /* int xpos; */
      osip_mutex_lock(log_mutex);
      snprintf(buf1,199, "%199.199s", " ");
      attrset(COLOR_PAIR(4));
      mvaddnstr(y-3,0,buf1,x-1);
      /* xpos = (x - strlen(log_buf3))/2;
	 if (xpos<0)
	 xpos = 0; */
      /* mvaddnstr(y-3,xpos,log_buf3,x-1); */
      mvaddnstr(y-3,0,log_buf3,x-1);
      osip_mutex_unlock(log_mutex);
    }
  return 0;
}

