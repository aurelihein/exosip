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


#include "gui.h"

extern jcall_t jcalls[];


/* usefull method */
int josua_gui_clear();
int josua_gui_print();
int josua_gui_run_command(int c);

typedef struct gui {
#define GUI_DISABLED -1
#define GUI_ON        0
#define GUI_OFF       1
  int on_off;
  int x0;
  int x1;
  int y0;
  int y1;
  int (*gui_clear)();
  int (*gui_print)();
  int (*gui_run_command)(int);
  int (*gui_key_pressed)();
} gui_t;

/* declaration of possible windows */

int window_topline_print();

gui_t gui_window_topline = {
  GUI_DISABLED,
  0,
  -999,
  0,
  1,
  NULL,
  &window_topline_print,
  NULL,
  NULL
};


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

int window_icon_print();

gui_t gui_window_icon = {
  GUI_DISABLED,
  0,
  19,
  2,
  9,
  NULL,
  &window_icon_print,
  NULL,
  NULL
};


int window_loglines_print();

gui_t gui_window_loglines = {
  GUI_DISABLED,
  0,
  -999,
  -3,
  0,
  NULL,
  &window_loglines_print,
  NULL,
  NULL
};

int window_menu_print();
int window_menu_run_command(int c);

gui_t gui_window_menu = {
  GUI_OFF,
  20,
  -999,
  2,
  9,
  NULL,
  &window_menu_print,
  &window_menu_run_command,
  NULL
};


typedef struct menu_t {
  const char *key;
  const char *text;
  void (*fn)();
} menu_t;


/* some external methods */

static const menu_t josua_menu[]= {
  { "v", " Start a Voice Conversation.",          &__josua_start_call },
  { "c", " Start a Chat Session.",                &__josua_message },
  { "m", " Manage Pending Calls.",                &__josua_manage_call },
  { "r", " Manage Pending Registrations.",        &__josua_register  },
  { "u", " Manage Pending Subscriptions.",        &__josua_manage_subscribers },
  { "s", " Josua's Setup.",                       &__josua_set_up  },
  { "q", " Quit jack' Open Sip User Agent.",      &__josua_quit  },
  { 0 }
};


gui_t *gui_windows[10] =
{
  &gui_window_topline,
  &gui_window_icon,
  &gui_window_loglines,
  &gui_window_menu,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

struct colordata {
       int fore;
       int back;
       int attr;
};

struct colordata color[]= {
  /* fore              back            attr */
  {COLOR_WHITE,        COLOR_BLACK,    0	}, /* 0 default */
  {COLOR_WHITE,        COLOR_GREEN,    A_BOLD	}, /* 1 title */
  {COLOR_WHITE,        COLOR_BLACK,    0	}, /* 2 list */
  {COLOR_WHITE,        COLOR_YELLOW,   A_REVERSE}, /* 3 listsel */
  {COLOR_WHITE,        COLOR_BLUE,     0	}, /* 4 calls */
  {COLOR_WHITE,        COLOR_RED,      0	}, /* 5 query */
  {COLOR_BLACK,        COLOR_CYAN,     0	}, /* 6 info */
  {COLOR_WHITE,        COLOR_BLACK,    0	}, /* 7 help */
};

int use_color = 0; /* 0: yes,      1: no */

/*
  jmosip_message_t *jmsip_context;
*/

static int cursesareon= 0;
void curseson() {
  if (!cursesareon) {
    const char *cup, *smso;
    initscr();
    start_color();


    if (has_colors() && start_color()==OK && COLOR_PAIRS >= 9) {
      int i;
      use_color = 0;
      for (i = 1; i < 9; i++) {
	if (init_pair(i, color[i].fore, color[i].back) != OK)
	  fprintf(stderr, "failed to allocate colour pair");
      }
    }
    else
      use_color = 1;


    cup= tigetstr("cup");
    smso= tigetstr("smso");
    if (!cup || !smso) {
      endwin();
      if (!cup)
        fputs("Terminal does not appear to support cursor addressing.\n",stderr);
      if (!smso)
        fputs("Terminal does not appear to support highlighting.\n",stderr);
      fputs("Set your TERM variable correctly, use a better terminal,\n"
	      "or make do with the per-package management tool ",stderr);
      fputs("JOSUA" ".\n",stderr);
      printf("terminal lacks necessary features, giving up");
      exit(1);
    }
  }
  cursesareon= 1;
}

void cursesoff() {
  if (cursesareon) {
    clear();
    refresh();
    endwin();
  }
  cursesareon=0;
}

int window_topline_print()
{
  int y,x;
  char buf[250];

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  sprintf(buf,"Josua 0.6.2 \\\\//                                     Powered by eXosip/osip2. %-50.50s"," ");
  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));
  mvaddnstr(gui_window_topline.y0,
	    gui_window_topline.x0,
	    buf,x-1);
  return 0;
}

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

static char log_buf3[200] = { '\0' };
static char log_buf2[200] = { '\0' };
static char log_buf1[200] = { '\0' };
static struct osip_mutex *log_mutex = NULL;

int window_loglines_print()
{
  char buf1[200];
  int x, y;
  getmaxyx(stdscr,y,x);

  if (log_buf1!='\0')
    {
      /* int xpos; */
      osip_mutex_lock(log_mutex);
      snprintf(buf1,199, "%80.80s", " ");
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
      snprintf(buf1,199, "%80.80s", " ");
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
      snprintf(buf1,199, "%80.80s", " ");
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

int cursor_menu = 0;

int window_menu_print()
{
  int y,x;
  char buf[250];
  int i;
  int pos;
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  pos = 0;
  for (i=gui_window_menu.y0; i<gui_window_menu.y1; i++)
    {
      snprintf(buf, gui_window_menu.x1 - gui_window_menu.x0,
	      "%c%c %d. %-80.80s ",
	      (cursor_menu==pos) ? '-' : ' ',
	      (cursor_menu==pos) ? '>' : ' ',
	      i-gui_window_menu.y0,
	      josua_menu[i-gui_window_menu.y0].text);

      attrset(COLOR_PAIR(5));
      attrset((pos==cursor_menu) ? A_REVERSE : A_NORMAL);
      mvaddnstr(i,
		gui_window_menu.x0,
		buf,
		x-gui_window_menu.x0-1);
      pos++;
    }
  return 0;
}

int window_menu_run_command(int c)
{
  int max = 7;
  switch (c)
    {
    case KEY_DOWN:
      cursor_menu++;
      cursor_menu %= max;
      break;
    case KEY_UP:
      cursor_menu += max-1;
      cursor_menu %= max;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
      cursor_menu = c-48;
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
      /* menu selected! */
      josua_menu[cursor_menu].fn();
      break;

    default:
      beep();
      return -1;
    }

  window_menu_print();
  return 0;
}


void josua_printf(char *chfr, ...)
{
  va_list ap;  
  char buf1[256];
  
  VA_START (ap, chfr);
  vsnprintf(buf1,255, chfr, ap);

  if (log_mutex==NULL)
    {
      log_mutex = osip_mutex_init();
    }

  osip_mutex_lock(log_mutex);
  /* snprintf(log_buf1,199, "%80.80s\n", buf1); */
  if (log_buf1=='\0')
    snprintf(log_buf1,255, "[%s]", buf1);
  else if (log_buf2=='\0')
    {
      if (log_buf1!='\0')
	snprintf(log_buf2,255, "%s", log_buf1);
      snprintf(log_buf1,255, "[%s]", buf1);
    }
  else
    {
      if (log_buf2!='\0')
	snprintf(log_buf3,255, "%s", log_buf2);
      snprintf(log_buf2,255, "%s", log_buf1);
      snprintf(log_buf1,255, "[%s]", buf1);
    }
    
  osip_mutex_unlock(log_mutex);

  va_end (ap);

}

int josua_gui_clear()
{
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  refresh();
  clear();


  return 0;
}

int josua_gui_print()
{
  int i;
  for (i = 0; gui_windows[i]!=NULL;i++) 
    {
      gui_windows[i]->gui_print();
      refresh();
    }
  return 0;
}

int josua_gui_run_command(int c)
{
  int i;
  /* find the active window */
  for (i = 0; gui_windows[i]!=NULL
	 && gui_windows[i]->on_off != GUI_ON ;i++)
    { }
  if (gui_windows[i]==NULL)
    {
      beep();
      return -1;
    }

  if (gui_windows[i]->gui_run_command!=NULL)
    gui_windows[i]->gui_run_command(c);

  return 0;
}

int josua_gui_key_pressed()
{
  int c;
  int i;

  do
    {
      refresh();
      halfdelay(1);

      c= getch();
    }
  while (c == ERR && (errno == EINTR || errno == EAGAIN));
  
  if (c==ERR)
    {
      if(errno != 0)
	{
	  fprintf(stderr, "failed to getch in main menu\n");
	  exit(1);
	}
      else {
      }
    }
  else 
    {

      if (c==KEY_RIGHT || c=='>')
	{
	  /* activate previous windows */
	  for (i = 0; gui_windows[i]!=NULL
		 && gui_windows[i]->on_off != GUI_ON ;i++)
	    { }
	  
	  if (gui_windows[i]==NULL)
	    {
	      /* activate previous windows */
	      for (i = 0; gui_windows[i]!=NULL
		     && gui_windows[i]->on_off != GUI_OFF ;i++)
		{ }
	      if (gui_windows[i]==NULL)
		return -2;
	      gui_windows[i]->on_off = GUI_ON;
	    }
	  else if (gui_windows[i]->on_off == GUI_ON)
	    {
	      gui_windows[i]->on_off = GUI_OFF;
	      i++;
	      for (; gui_windows[i]!=NULL
		     && gui_windows[i]->on_off != GUI_OFF ;i++)
		{ }
	      if (gui_windows[i]==NULL)
		{
		  for (i = 0; gui_windows[i]!=NULL
			 && gui_windows[i]->on_off != GUI_OFF ;i++)
		    { }
		  if (gui_windows[i]==NULL)
		    return -2;
		  gui_windows[i]->on_off = GUI_ON;
		}
	      else if (gui_windows[i]->on_off == GUI_OFF)
		gui_windows[i]->on_off = GUI_ON;
	    }
	}
      else if (c==KEY_LEFT || c=='<')
	{
	  /* activate previous windows */
	  for (i = 0; gui_windows[i]!=NULL
		 && gui_windows[i]->on_off != GUI_ON ;i++)
	    { }
	  
	  if (gui_windows[i]==NULL)
	    {
	      for (i=0; gui_windows[i]!=NULL
		     && gui_windows[i]->on_off != GUI_OFF ;i++)
		{ }
	      if (gui_windows[i]==NULL)
		return -2; /* no window with GUI_OFF */
	      gui_windows[i]->on_off = GUI_ON;
	    }
	  else if (gui_windows[i]->on_off == GUI_ON)
	    {
	      gui_windows[i]->on_off = GUI_OFF;
	      i--;
	      for (; i>=0 && gui_windows[i]!=NULL
		     && gui_windows[i]->on_off != GUI_OFF ;i--)
		{ }
	      if (i>=0
		  && gui_windows[i]!=NULL
		  && gui_windows[i]->on_off == GUI_OFF)
		{
		  gui_windows[i]->on_off = GUI_ON;
		}
	      else
		{
		  for (i=0; gui_windows[i]!=NULL ;i++)
		    { }
		  i--;
		  for (;i>=0 && gui_windows[i]->on_off != GUI_OFF ;i--)
		    { }
		  if (i>=0 &&  gui_windows[i]->on_off == GUI_OFF)
		    {
		      gui_windows[i]->on_off = GUI_ON;
		    }
		}
	    }
	}
      else
	{
	  /* probably for the main menu */
	  return c;
	}
    }

  return -1;
}

int
gui_start()
{
  gui_window_menu.on_off = GUI_ON;

  josua_gui_clear();
  josua_gui_print();

  for (;;)
    {
      int key;
      /* find the active gui window */
      int i;
      for (i = 0; gui_windows[i]!=NULL
	     && gui_windows[i]->on_off != GUI_ON ;i++)
	{ }
      if (gui_windows[i]->on_off != GUI_ON)
	return -1; /* no active window?? */

      if (gui_windows[i]->gui_key_pressed==NULL)
	key = josua_gui_key_pressed();
      else
	key = gui_windows[i]->gui_key_pressed();
      if (key==-1)
	{
	  /* no interesting key */
	    }
      else
	{
	  josua_gui_run_command(key);
	}
    }

  echo();
  cursesoff();
}

int window_new_call_print();
int window_new_call_run_command(int c);
int window_new_call_key_pressed();

gui_t gui_window_new_call = {
  GUI_OFF,
  20,
  -999,
  10,
  -6,
  NULL,
  &window_new_call_print,
  &window_new_call_run_command,
  &window_new_call_key_pressed
};

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
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "From    : %-80.80s"," ");
  mvaddnstr(gui_window_new_call.y0,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "To      : %-80.80s"," ");
  mvaddnstr(gui_window_new_call.y0+1,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "Subject : %-80.80s"," ");
  mvaddnstr(gui_window_new_call.y0+2,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  snprintf(buf,
	   x - gui_window_new_call.x0,
	   "Attchmnt: %-80.80s"," ");
  mvaddnstr(gui_window_new_call.y0+3,
	    gui_window_new_call.x0,
	    buf,
	    x-gui_window_new_call.x0-1);
  return 0;

}

int window_new_call_run_command(int c)
{

  return 0;
}

int new_call_cursor = 0;

int window_new_call_key_pressed()
{
  int key;
  int y,x;
  char buf[250];
  curseson(); echo(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));
  halfdelay(1);

  if (gui_window_new_call.x1==-999)
    {}
  else x = gui_window_new_call.x1;

  key = mvgetch(gui_window_new_call.y0 + new_call_cursor,
		gui_window_new_call.x0+11);

  if (c == ERR && (errno == EINTR || errno == EAGAIN))
    return -1;
  if (c==ERR)
    {
      if(errno != 0)
	{
	  fprintf(stderr, "failed to getch in main menu\n");
	  exit(1);
	}
      return -1;
    }
    /*
      mvwgetnstr(stdscr, gui_window_new_call.y0 + new_call_cursor,
      gui_window_new_call.x0+11,
      buf,
      x-gui_window_new_call.x0-11-1); */

  new_call_cursor++;
  xxx
  return 0;
}

void
__josua_start_call()
{
  int i;
  for (i = 0; gui_windows[i]!=NULL ;i++)
    { }

  new_call_cursor = 0;
  if (gui_windows[i]==NULL)
    gui_windows[i] = &gui_window_new_call;

  window_new_call_print();
#if 0
  osip_message_t *invite;
  int i;

  i = eXosip_build_initial_invite(&invite,
				  nctab_findvalue(&nctab_newcall,
						  TABSIZE_NEWCALL,
						  "to"),
				  nctab_findvalue(&nctab_newcall,
						  TABSIZE_NEWCALL,
						  "from"),
				  nctab_findvalue(&nctab_newcall,
						  TABSIZE_NEWCALL,
						  "route"),
				  nctab_findvalue(&nctab_newcall,
						  TABSIZE_NEWCALL,
						  "subject"));

  if (i!=0) return;
  eXosip_lock();
  eXosip_start_call(invite, NULL, NULL, "10500");
  eXosip_unlock();
#endif

  refresh();
  /* make a call */
}

void
__josua_manage_call()
{

}

void __josua_message()
{

}

void __josua_register()
{

}

void __josua_set_up()
{

}

void __josua_manage_subscribers()
{

}

void __josua_quit() {

  eXosip_quit();

  cursesoff();  
  exit(1);
}
