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

#include "gui_new_call.h"
#include "gui_menu.h"

gui_t gui_window_new_call = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_new_call_print,
  &window_new_call_run_command,
  NULL,
  window_new_call_draw_commands,
  10,
  1,
  -1,
  NULL
};

static char static_to[200] = { '\0' };

void window_new_call_with_to(char *_to)
{
  snprintf(static_to, 200, _to);
}

int window_new_call_print()
{
  int y,x;
  char buf[250];
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  if (gui_window_new_call.x1==-999)
    {}
  else x = gui_window_new_call.x1;

  attrset(COLOR_PAIR(0));
  snprintf(buf, 199, "%199.199s", " ");
  mvaddnstr(gui_window_new_call.y0+1,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  mvaddnstr(gui_window_new_call.y0+2,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  mvaddnstr(gui_window_new_call.y0+3,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  mvaddnstr(gui_window_new_call.y0+4,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  
  attrset(COLOR_PAIR(1));
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "From    : %s %80.80s", cfg.identity, " ");
  mvaddnstr(gui_window_new_call.y0,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "To      : ");
  mvaddnstr(gui_window_new_call.y0+1,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  attrset(COLOR_PAIR(0));

  mvaddnstr(gui_window_new_call.y0+1,
	    gui_window_new_call.x0+10,
	    static_to,
	    x-gui_window_new_call.x0-1-10);
  

  attrset(COLOR_PAIR(1));
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "Subject : ");
  mvaddnstr(gui_window_new_call.y0+2,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "Route   : ");
  mvaddnstr(gui_window_new_call.y0+3,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "Attchmnt: ");
  mvaddnstr(gui_window_new_call.y0+4,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  
  window_new_call_draw_commands();

  return 0;
}


int window_new_call_run_command(int c)
{
  int y,x;
  int i;
  getmaxyx(stdscr,y,x);

  if (gui_window_new_call.x1==-999)
    {}
  else x = gui_window_new_call.x1;

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
    case '\r':
    case KEY_ENTER:
    case KEY_DOWN:
      if (gui_window_new_call.ycursor<4)
	{
	  gui_window_new_call.ycursor++;
	  gui_window_new_call.xcursor=10;
	}
      break;
    case KEY_UP:
      if (gui_window_new_call.ycursor>1)
	{
	  gui_window_new_call.ycursor--;
	  gui_window_new_call.xcursor=10;
	}
      break;
    case KEY_RIGHT:
      if (gui_window_new_call.xcursor<(x-gui_window_new_call.x0-1))
	gui_window_new_call.xcursor++;
      break;
    case KEY_LEFT:
      if (gui_window_new_call.xcursor>0)
	gui_window_new_call.xcursor--;
      break;

      /* case 20: */  /* Ctrl-T */
    case 1:  /* Ctrl-A */
      __show_address_book_browse();
      break;
    case 4:  /* Ctrl-D */
      {
	char buf[200];

	if (static_to[0]!='\0' && gui_window_new_call.ycursor==1)
	  static_to[0]='\0';
	attrset(COLOR_PAIR(0));
	snprintf(buf, 199, "%199.199s", " ");
	mvaddnstr(gui_window_new_call.y0+gui_window_new_call.ycursor,
		  gui_window_new_call.x0 + 10,
		  buf,
		  x-gui_window_new_call.x0-10-1);
	gui_window_new_call.xcursor=10;
      }
      break;
    case 5:  /* Ctrl-E */
      if (static_to[0]!='\0')
	static_to[0]='\0';
      gui_window_new_call.xcursor=10;
      gui_window_new_call.ycursor=1;
      window_new_call_print();
      break;
    case 24: /* Ctrl-X */
      {
	int ycur = gui_window_new_call.y0;
	int xcur = gui_window_new_call.x0+10;
	char to[200];
	char subject[200];
	char route[200];
	/* char attachment[200]; */
	ycur++;
	mvinnstr(ycur, xcur, to, x-gui_window_new_call.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, subject, x-gui_window_new_call.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, route, x-gui_window_new_call.x0-10);
	osip_clrspace(to);
	osip_clrspace(subject);
	osip_clrspace(route);
	i = _josua_start_call(cfg.identity, to, subject, route, "10500", NULL);
	if (i<0) beep();
	/* mvinnstr(ycur, xcur, tmp, 199); */
      }
      break;
    case 23:  /* Ctrl-W */
      {
	int ycur = gui_window_new_call.y0;
	int xcur = gui_window_new_call.x0+10;
	char to[200];
	char subject[200];
	char route[200];
	/* char attachment[200]; */
	ycur++;
	mvinnstr(ycur, xcur, to, x-gui_window_new_call.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, subject, x-gui_window_new_call.x0-10);
	ycur++;
	mvinnstr(ycur, xcur, route, x-gui_window_new_call.x0-10);
	osip_clrspace(to);
	osip_clrspace(subject);
	osip_clrspace(route);

	i = _josua_start_message(cfg.identity, to, route, subject);
	if (i!=0) beep();
      }
      break;
#if 0
    case 15: /* Ctrl-O */
      {
	int ycur = gui_window_new_call.y0;
	int xcur = gui_window_new_call.x0+10;
	char to[200];
	char route[200];
	ycur++;
	mvinnstr(ycur, xcur, to, x-gui_window_new_call.x0-10);
	ycur++;
	ycur++;
	mvinnstr(ycur, xcur, route, x-gui_window_new_call.x0-10);

	i = _josua_start_options(cfg.identity, to, route);
	if (i!=0) beep();
	/* mvinnstr(ycur, xcur, tmp, 199); */
      }
      break;
#endif
    case 21: /* Ctrl-U */
      {
	int ycur = gui_window_new_call.y0;
	int xcur = gui_window_new_call.x0+10;
	char to[200];
	char route[200];
	ycur++;
	mvinnstr(ycur, xcur, to, x-gui_window_new_call.x0-10);
	ycur++;
	ycur++;
	mvinnstr(ycur, xcur, route, x-gui_window_new_call.x0-10);

	i = _josua_start_subscribe(cfg.identity, to, route);
	if (i!=0) beep();
	/* mvinnstr(ycur, xcur, tmp, 199); */
      }
      break;
    default:

      if (gui_window_new_call.xcursor<(x-gui_window_new_call.x0-1))
	{
	  gui_window_new_call.xcursor++;
	  attrset(COLOR_PAIR(0));
	  echochar(c);
	}
      else
	beep();
      return -1;
    }

  return 0;
}

void window_new_call_draw_commands()
{
  int x,y;
  char *new_call_commands[] = {
    "<-",  "PrevWindow",
    "->",  "NextWindow",
    "^X", "StartCall" ,
    "^O", "StartOptions" ,
    "^U", "StartSubscribe" ,
    "^A", "GtoAddrBook",
    "^D", "DeleteLine",
    "^W", "SendMessage",
    "^E", "EraseAll",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(new_call_commands,
		      y-5,
		      0);
}
