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

#include "jinsubscriptions.h"
#include "gui_menu.h"
#include "gui_insubscriptions_list.h"

gui_t gui_window_insubscriptions_list = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_insubscriptions_list_print,
  &window_insubscriptions_list_run_command,
  NULL,
  window_insubscriptions_list_draw_commands,
  -1,
  -1,
  -1,
  NULL
};

int cursor_insubscriptions_list = 0;

int window_insubscriptions_list_print()
{
  int k, pos;
  int y,x;
  char buf[250];

  josua_clear_box_and_commands(gui_windows[EXTRAGUI]);

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  if (gui_window_insubscriptions_list.x1==-999)
    {}
  else x = gui_window_insubscriptions_list.x1;

  attrset(COLOR_PAIR(0));

  pos=1;
  for (k=0;k<MAX_NUMBER_OF_INSUBSCRIPTIONS;k++)
    {
      if (jinsubscriptions[k].state != NOT_USED)
	{
	  snprintf(buf, 199, "%c%c %i//%i %i %s with: %s %199.199s",
		   (cursor_insubscriptions_list==pos-1) ? '-' : ' ',
		   (cursor_insubscriptions_list==pos-1) ? '>' : ' ',
		   jinsubscriptions[k].nid,
		   jinsubscriptions[k].did,
		   jinsubscriptions[k].status_code,
		   jinsubscriptions[k].reason_phrase,
		   jinsubscriptions[k].remote_uri, " ");

	  attrset(COLOR_PAIR(5));
	  attrset((pos-1==cursor_insubscriptions_list) ? A_REVERSE : A_NORMAL);

	  mvaddnstr(gui_window_insubscriptions_list.y0+pos-1,
		    gui_window_insubscriptions_list.x0,
		    buf,
		    x-gui_window_insubscriptions_list.x0-1);
	  pos++;
	}
      if (pos>y+gui_window_insubscriptions_list.y1-gui_window_insubscriptions_list.y0+1)
	break; /* do not print next one */
    }

  window_insubscriptions_list_draw_commands();
  return 0;
}

void window_insubscriptions_list_draw_commands()
{
  int x,y;
  char *insubscriptions_list_commands[] = {
    "<",  "PrevWindow",
    ">",  "NextWindow",
    "c",  "Close" ,
    "a",  "Accept",
    "r",  "Reject",
    "t",  "ViewSubscribers",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(insubscriptions_list_commands, y-5, 0);
}


int window_insubscriptions_list_run_command(int c)
{
  /*  jinsubscription_t *js; */
  int i;
  int max;
  int y,x;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);

  if (gui_window_insubscriptions_list.x1==-999)
    {}
  else x = gui_window_insubscriptions_list.x1;

  if (gui_window_insubscriptions_list.y1<0)
    max = y + gui_window_insubscriptions_list.y1 - gui_window_insubscriptions_list.y0+2;
  else
    max = gui_window_insubscriptions_list.y1 - gui_window_insubscriptions_list.y0+2;

  i = jinsubscription_get_number_of_pending_insubscriptions();
  if (i<max) max=i;

  if (max==0)
    {
      beep();
      return -1;
    }
  
  switch (c)
    {
    case KEY_DOWN:
      cursor_insubscriptions_list++;
      cursor_insubscriptions_list %= max;
      break;
    case KEY_UP:
      cursor_insubscriptions_list += max-1;
      cursor_insubscriptions_list %= max;
      break;
	
    case 't':
      __show_subscriptions_list();
      break;

#if 0
    case 'a':
      ca = jinsubscription_find_insubscription(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_answer_call(ca->did, 200);
      eXosip_unlock();
      break;
    case 'r':
      ca = jinsubscription_find_insubscription(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 480);
      if (i==0)
	jinsubscription_remove(ca);
      eXosip_unlock();
      window_insubscriptions_list_print();
      break;
    case 'd':
      ca = jinsubscription_find_insubscription(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 603);
      if (i==0)
	jinsubscription_remove(ca);
      eXosip_unlock();
      window_insubscriptions_list_print();
      break;
    case 'b':
      ca = jinsubscription_find_insubscription(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_answer_call(ca->did, 486);
      if (i==0)
	jinsubscription_remove(ca);
      eXosip_unlock();
      window_insubscriptions_list_print();
      break;
    case 'c':
      ca = jinsubscription_find_insubscription(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      i = eXosip_terminate_call(ca->nid, ca->did);
      if (i==0)
	jinsubscription_remove(ca);
      eXosip_unlock();
      window_insubscriptions_list_print();
      break;
    case 'm':
      ca = jinsubscription_find_call(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_on_hold_call(ca->did);
      eXosip_unlock();
      break;
    case 'u':
      ca = jinsubscription_find_call(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_off_hold_call(ca->did);
      eXosip_unlock();
      break;
    case 'o':
      ca = jinsubscription_find_call(cursor_insubscriptions_list);
      if (ca==NULL) { beep(); break; }
      eXosip_lock();
      eXosip_options_call(ca->did);
      eXosip_unlock();
      break;
#endif
    default:
      beep();
      return -1;
    }

  window_insubscriptions_list_print();
  return 0;
}
