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

#include "gui_address_book_newentry.h"

gui_t gui_window_address_book_newentry = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_address_book_newentry_print,
  &window_address_book_newentry_run_command,
  NULL,
  window_address_book_newentry_draw_commands,
  10,
  0,
  -1,
  NULL
};

int window_address_book_newentry_print()
{
  int y,x;
  char buf[250];
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  if (gui_window_address_book_newentry.x1==-999)
    {}
  else x = gui_window_address_book_newentry.x1;

  attrset(COLOR_PAIR(0));
  snprintf(buf, 199, "%199.199s", " ");
  mvaddnstr(gui_window_address_book_newentry.y0+1,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  mvaddnstr(gui_window_address_book_newentry.y0+2,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  mvaddnstr(gui_window_address_book_newentry.y0+3,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  mvaddnstr(gui_window_address_book_newentry.y0+4,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  
  attrset(COLOR_PAIR(1));
  snprintf(buf,
	   x - gui_window_address_book_newentry.x0,
	   "SIP url : ");
  mvaddnstr(gui_window_address_book_newentry.y0,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  snprintf(buf,
	   x - gui_window_address_book_newentry.x0,
	   "TEL url : ");
  mvaddnstr(gui_window_address_book_newentry.y0+1,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  snprintf(buf,
	   x - gui_window_address_book_newentry.x0,
	   "Email   : ");
  mvaddnstr(gui_window_address_book_newentry.y0+2,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  snprintf(buf,
	   x - gui_window_address_book_newentry.x0,
	   "Phone   : ");
  mvaddnstr(gui_window_address_book_newentry.y0+3,
	    gui_window_address_book_newentry.x0,
	    buf,
	    x-gui_window_address_book_newentry.x0-1);
  
  window_address_book_newentry_draw_commands();

  return 0;
}


int window_address_book_newentry_run_command(int c)
{
  int y,x;
  getmaxyx(stdscr,y,x);

  if (gui_window_address_book_newentry.x1==-999)
    {}
  else x = gui_window_address_book_newentry.x1;

  switch (c)
    {
    case KEY_DC:
      delch();
      break;
    case KEY_BACKSPACE:
    case 127:
      if (active_gui->xcursor>10)
	{
	  int xcur,ycur;
	  active_gui->xcursor--;
	  getyx(stdscr,ycur,xcur);
	  move(ycur,xcur-1);
	  delch();
	}
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
    case KEY_DOWN:
      if (gui_window_address_book_newentry.ycursor<3)
	{
	  gui_window_address_book_newentry.ycursor++;
	  gui_window_address_book_newentry.xcursor=10;
	}
      break;
    case KEY_UP:
      if (gui_window_address_book_newentry.ycursor>0)
	{
	  gui_window_address_book_newentry.ycursor--;
	  gui_window_address_book_newentry.xcursor=10;
	}
      break;
    case KEY_RIGHT:
      if (gui_window_address_book_newentry.xcursor<(x-gui_window_address_book_newentry.x0-1))
	gui_window_address_book_newentry.xcursor++;
      break;
    case KEY_LEFT:
      if (gui_window_address_book_newentry.xcursor>0)
	gui_window_address_book_newentry.xcursor--;
      break;

      /* case 20: */  /* Ctrl-T */
    case 1:  /* Ctrl-A */
      {
	int ycur = gui_window_address_book_newentry.y0;
	int xcur = gui_window_address_book_newentry.x0+10;
	char sipurl[200];
	char telurl[200];
	char email[200];
	char phone[200];
	mvinnstr(ycur, xcur, sipurl, x-gui_window_address_book_newentry.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, telurl, x-gui_window_address_book_newentry.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, email, x-gui_window_address_book_newentry.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, phone, x-gui_window_address_book_newentry.x0-10);
	
	_josua_add_contact(sipurl, telurl, email, phone);
	/* mvinnstr(ycur, xcur, tmp, 199); */
      }
      break;
    case 4:  /* Ctrl-D */
      {
	char buf[200];
	attrset(COLOR_PAIR(0));
	snprintf(buf, 199, "%199.199s", " ");
	mvaddnstr(gui_window_address_book_newentry.y0+gui_window_address_book_newentry.ycursor,
		  gui_window_address_book_newentry.x0 + 10,
		  buf,
		  x-gui_window_address_book_newentry.x0-10-1);
	gui_window_address_book_newentry.xcursor=10;
      }
      break;
    case 5:  /* Ctrl-E */
      gui_window_address_book_newentry.xcursor=10;
      gui_window_address_book_newentry.ycursor=0;
      window_address_book_newentry_print();
      break;
    default:
      /*
	fprintf(stderr, "c=%i", c);
	exit(0);
      */
      if (gui_window_address_book_newentry.xcursor<(x-gui_window_address_book_newentry.x0-1))
	{
	  gui_window_address_book_newentry.xcursor++;
	  attrset(COLOR_PAIR(0));
	  echochar(c);
	}
      else
	beep();
      return -1;
    }

  return 0;
}

void window_address_book_newentry_draw_commands()
{
  int x,y;
  char *address_book_newentry_commands[] = {
    "<-",  "PrevWindow",
    "->",  "NextWindow",
    "^A", "AddEntry" ,
    "^D", "DeleteLine",
    "^E", "EraseAll",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(address_book_newentry_commands,
		      y-5,
		      0);
}
