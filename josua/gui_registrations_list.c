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

#include "gui_registrations_list.h"
#include "gui_new_identity.h"

/* extern eXosip_t eXosip; */

gui_t gui_window_registrations_list = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_registrations_list_print,
  &window_registrations_list_run_command,
  NULL,
  &window_registrations_list_draw_commands,
  -1,
  -1,
  -1,
  NULL
};


int cursor_registrations_list = 0;
int cursor_registrations_start  = 0;
int last_reg_id = -1;

void window_registrations_list_draw_commands()
{
  int x,y;
  char *registrations_list_commands[] = {
    "<-",  "PrevWindow",
    "->",  "NextWindow",
    "a",  "AddRegistration",
    "d",  "DeleteRegistration",
    "r",  "Register",
    "u",  "Un-Register",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(registrations_list_commands,
		      y-5,
		      0);
}

int window_registrations_list_print()
{
  jidentity_t *id;
  int y,x;
  char buf[250];
  int pos;
  int pos_id;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  
  getmaxyx(stdscr,y,x);
  pos_id = 0;

  for (id = jidentity_get(); id!=NULL ; id=id->next)
    {
      if (cursor_registrations_start==pos_id)
	break;
      pos_id++;
    }

  pos = 0;
  for (; id!=NULL ; id=id->next)
    {
      if (pos==y+gui_window_registrations_list.y1
	  -gui_window_registrations_list.y0)
	break;
      if (last_reg_id==pos)
	{
	  snprintf(buf, gui_window_registrations_list.x1 - gui_window_registrations_list.x0,
		   "%c%c %-5.5s %-30.30s %-80.80s",
		   (cursor_registrations_list==pos) ? '-' : ' ',
		   (cursor_registrations_list==pos) ? '>' : ' ',
		   "[ok]",
		   id->i_identity, id->i_registrar);
	}
      else
	{
	  snprintf(buf, gui_window_registrations_list.x1 - gui_window_registrations_list.x0,
		   "%c%c %-5.5s %-30.30s %-80.80s",
		   (cursor_registrations_list==pos) ? '-' : ' ',
		   (cursor_registrations_list==pos) ? '>' : ' ',
		   "[--]",
		   id->i_identity, id->i_registrar);
	}
      attrset((pos==cursor_registrations_list) ? A_REVERSE : COLOR_PAIR(1));
      mvaddnstr(pos+gui_window_registrations_list.y0,
		gui_window_registrations_list.x0,
		buf,
		x-gui_window_registrations_list.x0-1);
      pos++;
    }
  refresh();

  window_registrations_list_draw_commands();
  return 0;
}

void __show_new_entry()
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI]==NULL)
    gui_windows[EXTRAGUI]= &gui_window_new_identity;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI]= &gui_window_new_identity;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_new_identity_print();
}

int window_registrations_list_run_command(int c)
{
  jidentity_t *id;
  int i;
  int y, x;
  int pos;
  int max;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);

  max=0;
  for (id = jidentity_get(); id!=NULL ; id=id->next)
    {
      max++;
    }
  
  switch (c)
    {
    case KEY_DOWN:
      /* cursor_registrations_list++;
	 cursor_registrations_list %= max; */
      if (cursor_registrations_list<y+gui_window_registrations_list.y1
	  -gui_window_registrations_list.y0-1 && cursor_registrations_list<max-1)
	cursor_registrations_list++;
      else if (cursor_registrations_start<max-(
	       y+gui_window_registrations_list.y1
	       -gui_window_registrations_list.y0))
	cursor_registrations_start++;
      else beep();
      break;
    case KEY_UP:
      if (cursor_registrations_list>0)
	cursor_registrations_list--;
      else if (cursor_registrations_start>0)
	cursor_registrations_start--;
      else beep();
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
      /* registrations_list selected! */
      pos=0;
      for (id = jidentity_get(); id!=NULL ; id=id->next)
	{
	  pos++;
	  if (cursor_registrations_list==pos)
	    break;
	}
      if (id!=NULL)
	{
	}
      break;
    case 'r':
      /* start registration */
      i = _josua_register(cursor_registrations_list
			  +cursor_registrations_start);
      if (i!=0) { beep(); return -1; }
      last_reg_id = cursor_registrations_list
	+cursor_registrations_start;
      break;
    case 'u':
      /* start registration */
      i = _josua_unregister(cursor_registrations_list
			    +cursor_registrations_start);
      if (i!=0) { beep(); return -1; }
      last_reg_id = cursor_registrations_list
	+cursor_registrations_start;
      break;
    case 'd':
      /* delete entry */
      break;
    case 'a':
      /* add new entry */
      __show_new_entry();
      break;
    default:
      beep();
      return -1;
    }

  if (gui_window_registrations_list.on_off==GUI_ON)
    window_registrations_list_print();
  return 0;
}

