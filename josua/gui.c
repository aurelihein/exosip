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

extern main_config_t cfg;

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
  void (*gui_draw_commands)();

  int xcursor;
  int ycursor;
  int len;
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
  NULL,
  NULL,
  -1,
  -1,
  -1
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
  NULL,
  NULL,
  -1,
  -1,
  -1
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
  NULL,
  NULL,
  -1,
  -1,
  -1
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
  NULL,
  NULL,
  -1,
  -1,
  -1
};


typedef struct menu_t {
  const char *key;
  const char *text;
  void (*fn)();
} menu_t;


/* some external methods */

static const menu_t josua_menu[]= {
  { "v", " Start a Voice Conversation.",          &__show_new_call },
  { "c", " Start a Chat Session.",                &__show_new_message },
  { "m", " Manage Pending Calls.",                &__josua_manage_call },
  { "r", " Manage Pending Registrations.",        &__josua_register  },
  { "u", " Manage Pending Subscriptions.",        &__josua_manage_subscribers },
  { "s", " Josua's Setup.",                       &__josua_set_up  },
  { "q", " Quit jack' Open Sip User Agent.",      &__josua_quit  },
  { 0 }
};

#define TOPGUI      0
#define ICONGUI     1
#define MENUGUI     2
#define LOGLINESGUI 3
#define EXTRAGUI    4

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

gui_t *active_gui = NULL;

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

int
josua_print_command(char **commands, int ypos, int xpos)
{
  int maxlen = 0;
  int i;
  int y,x;
  char buf[250];
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);

  for (i=0;commands[i]!=NULL;i=i+2)
    {
      int len = strlen(commands[i+1]);
      if (len>maxlen)
	maxlen = len;
    }

  if (commands[0]!=NULL) /* erase with default background */
    attrset(COLOR_PAIR(6));
  else
    attrset(COLOR_PAIR(0));
  snprintf(buf, 199, "%199.199s", " ");
  mvaddnstr(ypos,
	    xpos,
	    buf,
	    x-xpos-1);
  mvaddnstr(ypos+1,
	    xpos,
	    buf,
	    x-xpos-1);

  for (i=0;commands[i]!=NULL;i=i+2)
    {
      attrset(COLOR_PAIR(1));
      snprintf(buf,
	       strlen(commands[i])+1,
	       commands[i]);
      mvaddnstr(ypos,
		xpos,
		buf,
		strlen(commands[i]));
      attrset(COLOR_PAIR(6));
      snprintf(buf,
	       strlen(commands[i+1])+1,
	       "%s", commands[i+1]);
      mvaddnstr(ypos,
		xpos+3,
		buf,
		maxlen);
      i=i+2;
      if (commands[i]==NULL)
	break;
      attrset(COLOR_PAIR(1));
      snprintf(buf,
	       strlen(commands[i])+1,
	       commands[i]);
      mvaddnstr(ypos+1,
		xpos,
		buf,
		strlen(commands[i]));
      attrset(COLOR_PAIR(6));
      snprintf(buf,
	       strlen(commands[i+1])+1,
	       "%s", commands[i+1]);
      mvaddnstr(ypos+1,
		xpos+3,
		buf,
		maxlen);
      xpos = xpos+maxlen+4; /* position for next column */
    }
  return 0;
}

void josua_clear_commands_only()
{
  int x,y;
  char *commands[] = { NULL };
  getmaxyx(stdscr,y,x);
  josua_print_command(commands,
		      y-5,
		      0);
}

int
josua_clear_box_and_commands(gui_t *box)
{
  int pos;
  int y,x;
  char buf[250];
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);

  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));

  {
    char *commands[] = {
      NULL
    };
    josua_print_command(commands,
			y-5,
			0);
  }

  if (box->x1==-999)
    {}
  else x = box->x1;

  if (box->y1<0)
    y = y + box->y1;
  else
    y = box->y1;

  attrset(COLOR_PAIR(0));
  for (pos=box->y0;pos<y;pos++)
    {
      snprintf(buf, 199, "%199.199s", " ");
      mvaddnstr(pos,
		box->x0,
		buf,
		x-box->x0-1);
    }

  return 0;
}

int josua_gui_key_pressed()
{
  int c;
  int i;

  /*  do
      { */
      refresh();
      halfdelay(1);
      
      if (active_gui->xcursor==-1)
	{
	  noecho();
	  c= getch();
	}
      else
	{
	  int x,y;
	  x = active_gui->x0 + active_gui->xcursor;
	  y = active_gui->y0 + active_gui->ycursor;
	  noecho();
	  keypad(stdscr, TRUE);
	  c= mvgetch(y,x);
	  if ((c & 0x80))
	    {
	    }
	  refresh();
	}
/*
    }
  while (c == ERR && (errno == EINTR || errno == EAGAIN));
*/  
  if (c==ERR)
    {
      if(errno != 0)
	{
	}
      return -1;
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
	      active_gui = gui_windows[i];
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
		  active_gui = gui_windows[i];
		}
	      else if (gui_windows[i]->on_off == GUI_OFF)
		{
		  gui_windows[i]->on_off = GUI_ON;
		  active_gui = gui_windows[i];
		}
	    }
	  if (gui_windows[i]!=NULL &&
	      gui_windows[i]->gui_draw_commands!=NULL)
	    gui_windows[i]->gui_draw_commands();
	  else josua_clear_commands_only();
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
	      active_gui = gui_windows[i];
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
		  active_gui = gui_windows[i];
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
		      active_gui = gui_windows[i];
		    }
		}
	    }
	  if (gui_windows[i]!=NULL &&
	      gui_windows[i]->gui_draw_commands!=NULL)
	    gui_windows[i]->gui_draw_commands();
	  else josua_clear_commands_only();
	}
      else
	{
	  /* probably for the main menu */
	  return c;
	}
    }

  return -1;
}


int window_manage_call_print();
int window_manage_call_run_command(int c);
void window_manage_call_draw_commands();

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

int
gui_start()
{
  gui_window_menu.on_off = GUI_ON;
  active_gui = &gui_window_menu;

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
      i = josua_event_get();
      if (i==0 && gui_windows[EXTRAGUI]==&gui_window_manage_call)
	{
	  window_manage_call_print();
	}
      window_loglines_print();
    }

  echo();
  cursesoff();
}


int window_new_call_print();
int window_new_call_run_command(int c);
int window_new_call_key_pressed();
void window_new_call_draw_commands();

gui_t gui_window_new_call = {
  GUI_OFF,
  20,
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
  -1
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
    case '\n':
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
      break;
    case 4:  /* Ctrl-D */
      {
	char buf[200];
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
      gui_window_new_call.xcursor=10;
      gui_window_new_call.ycursor=1;
      window_new_call_print();
      /*
      snprintf(buf, 199, "%199.199s", " ");
      mvaddnstr(gui_window_new_call.ycursor,
		gui_window_new_call.x0 + 10,
		buf,
		x-gui_window_new_call.x0-10);
      mvaddnstr(gui_window_new_call.ycursor,
		gui_window_new_call.x0 + 10,
		buf,
		x-gui_window_new_call.x0-10);
      mvaddnstr(gui_window_new_call.ycursor,
		gui_window_new_call.x0 + 10,
		buf,
		x-gui_window_new_call.x0-10);
      mvaddnstr(gui_window_new_call.ycursor,
		gui_window_new_call.x0 + 10,
		buf,
		x-gui_window_new_call.x0-10);
      */
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

	_josua_start_call(cfg.identity, to, subject, route);
	/* mvinnstr(ycur, xcur, tmp, 199); */
      }
      break;
    default:
      /*
	fprintf(stderr, "c=%i", c);
	exit(0);
      */
      if (gui_window_new_call.xcursor<(x-gui_window_new_call.x0-1))
	{
	  gui_window_new_call.xcursor++;
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
    "<",  "PrevWindow",
    ">",  "NextWindow",
    "^X", "StartCall" ,
    "^D", "DeleteLine",
    "^E", "EraseAll",
    "^A", "GtoAddrBook",
    NULL
  };
  getmaxyx(stdscr,y,x);
  josua_print_command(new_call_commands,
		      y-5,
		      0);
}


void
__show_new_call()
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI]==NULL)
    gui_windows[EXTRAGUI]= &gui_window_new_call;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI]= &gui_window_new_call;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_new_call_print();
}


void
__josua_manage_call()
{
  active_gui->on_off = GUI_OFF;
  if (gui_windows[EXTRAGUI]==NULL)
    gui_windows[EXTRAGUI]= &gui_window_manage_call;
  else
    {
      gui_windows[EXTRAGUI]->on_off = GUI_OFF;
      josua_clear_box_and_commands(gui_windows[EXTRAGUI]);
      gui_windows[EXTRAGUI]= &gui_window_manage_call;
    }

  active_gui = gui_windows[EXTRAGUI];
  active_gui->on_off = GUI_ON;

  window_manage_call_print();
}

void __show_new_message()
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
