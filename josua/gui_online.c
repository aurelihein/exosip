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

#include "gui_online.h"

int josua_online_status = EXOSIP_NOTIFY_ONLINE;
int josua_registration_status = -1;
char josua_registration_reason_phrase[100] = { '\0' };
char josua_registration_server[100] = { '\0' };

gui_t gui_window_online = {
  GUI_DISABLED,
  0,
  -999,
  -3,
  0,
  NULL,
  &window_online_print,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  -1,
  NULL
};

int window_online_print()
{
  int y,x;
  char buf[250];
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(14));

  snprintf(buf, x,
	   "whoIam   :   %s %140.140s", cfg.identity, " ");
  mvaddnstr(y+gui_window_online.y0,
	    0,
	    buf,
	    x-1);

  if (josua_online_status==EXOSIP_NOTIFY_UNKNOWN)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "unknown", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_PENDING)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "pending", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_ONLINE)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Online", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_BUSY)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Busy", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_BERIGHTBACK)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Be Right Back", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_AWAY)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Away", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_ONTHEPHONE)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "On The Phone", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_OUTTOLUNCH)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Out To Lunch", " ");
  else if (josua_online_status==EXOSIP_NOTIFY_CLOSED)
    snprintf(buf, x,
	     "IM status:   %s %140.140s", "Closed", " ");
  else
    snprintf(buf, x,
	     "IM status:   ?? %140.140s" , " ");
  mvaddnstr(y+gui_window_online.y0+1,
	    0,
	    buf,
	    x-1);

  if (josua_registration_status==-1)
    {
      snprintf(buf, x,
	       "registred:   --Not registred-- %140.140s", " ");
      mvaddnstr(y+gui_window_online.y0+2,
		0,
		buf,
		x-1);
    }
  else if (199<josua_registration_status && josua_registration_status<300)
    {
      snprintf(buf, x,
	       "registred:   [%i %s] %s %140.140s",
	       josua_registration_status,
	       josua_registration_reason_phrase,
	       josua_registration_server, " ");
      mvaddnstr(y+gui_window_online.y0+2,
		0,
		buf,
		x-1);
    }
  else if (josua_registration_status>299)
    {
      snprintf(buf, x,
	       "registred:   [%i %s] %s %140.140s",
	       josua_registration_status,
	       josua_registration_reason_phrase,
	       josua_registration_server, " ");
      mvaddnstr(y+gui_window_online.y0+2,
		0,
		buf,
		x-1);
    }
  else if (josua_registration_status==0)
    {
      snprintf(buf, x,
	       "registred:   [no answer] %s %140.140s",
	       josua_registration_server, " ");
      mvaddnstr(y+gui_window_online.y0+2,
		0,
		buf,
		x-1);
    }

  return 0;
}

