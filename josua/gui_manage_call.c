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
#include "gui_manage_call.h"

gui_t gui_window_manage_call = {
  GUI_OFF,
  20,
  -999,
  10,
  -6,
  NULL,
  &window_manage_call_print,
  &window_manage_call_run_command,
  NULL,
  window_manage_call_draw_commands,
  -1,
  -1,
  -1
};

int cursor_manage_call = 0;

int window_manage_call_print()
{
  int k, pos;
  int y,x;
  char buf[250];

  josua_clear_box_and_commands(gui_windows[EXTRAGUI]);

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  if (gui_window_manage_call.x1==-999)
    {}
  else x = gui_window_manage_call.x1;

  attrset(COLOR_PAIR(0));

  pos=1;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED)
	{
	  snprintf(buf, 199, "%c%c %i//%i %i %s with: %s %199.199s",
		   (cursor_manage_call==pos-1) ? '-' : ' ',
		   (cursor_manage_call==pos-1) ? '>' : ' ',
		   jcalls[k].cid,
		   jcalls[k].did,
		   jcalls[k].status_code,
		   jcalls[k].reason_phrase,
		   jcalls[k].remote_uri, " ");

	  attrset(COLOR_PAIR(5));
	  attrset((pos-1==cursor_manage_call) ? A_REVERSE : A_NORMAL);

	  mvaddnstr(gui_window_manage_call.y0+pos-1,
		    gui_window_manage_call.x0,
		    buf,
		    x-gui_window_manage_call.x0-1);
	  pos++;
	}
      if (pos>y+gui_window_manage_call.y1-gui_window_manage_call.y0+1)
	break; /* do not print next one */
    }

  window_manage_call_draw_commands();
  return 0;
}

void window_manage_call_draw_commands()
{
  int x,y;
  char *manage_call_commands[] = {
    "<",  "PrevWindow",
    ">",  "NextWindow",
    "c",  "Close" ,
    "a",  "Answer",
    "m",  "Mute"  ,
    "u",  "UnMute",
    "d",  "Decline",
    "r",  "Reject",
    "b",  "AppearBusy",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(manage_call_commands, y-5, 0);
}

int window_manage_call_run_command(int c)
{
  jcall_t *ca;
  int i;
  int max;
  int y,x;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);

  if (gui_window_manage_call.x1==-999)
    {}
  else x = gui_window_manage_call.x1;

  if (gui_window_manage_call.y1<0)
    max = y + gui_window_manage_call.y1 - gui_window_manage_call.y0+2;
  else
    max = gui_window_manage_call.y1 - gui_window_manage_call.y0+2;

  i = jcall_get_number_of_pending_calls();
  if (i<max) max=i;

  if (max==0)
    {
      beep();
      return -1;
    }
  
  switch (c)
    {
    case KEY_DOWN:
      cursor_manage_call++;
      cursor_manage_call %= max;
      break;
    case KEY_UP:
      cursor_manage_call += max-1;
      cursor_manage_call %= max;
      break;
	
    case 'a':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_answer_call(ca->did, 200);
      eXosip_unlock();
      break;
    case 'r':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 480);
      if (i==0)
	jcall_remove(ca);
      eXosip_unlock();
      window_manage_call_print();
      break;
    case 'd':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 603);
      if (i==0)
	jcall_remove(ca);
      eXosip_unlock();
      window_manage_call_print();
      break;
    case 'b':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 486);
      if (i==0)
	jcall_remove(ca);
      eXosip_unlock();
      window_manage_call_print();
      break;
    case 'c':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_terminate_call(ca->cid, ca->did);
      if (i==0)
	jcall_remove(ca);
      eXosip_unlock();
      window_manage_call_print();
      break;
    case 'm':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_on_hold_call(ca->did);
      eXosip_unlock();
      break;
    case 'u':
      ca = jcall_find_call(cursor_manage_call);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_off_hold_call(ca->did);
      eXosip_unlock();
      break;
    default:
      beep();
      return -1;
    }

  window_manage_call_print();
  return 0;
}
