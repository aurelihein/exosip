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

static char rcsid[] = "main_ncurses:  $Id: main_ncurses.c,v 1.5 2003-03-24 18:17:47 aymeric Exp $";

#ifdef NCURSES_SUPPORT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include <term.h>
#include <ncurses.h>

#include "config.h"
#include <eXosip/eXosip.h>
#include <eXosip/eXosip2.h>

#include <osip2/thread.h>

#include "ppl_getopt.h"

extern eXosip_t eXosip;

static char *from = "<sip:jack@atosc.org>";


typedef struct menu_t {
  const char *command;
  const char *key;
  const char *option;
  const int   menu_id;
  const char *text;
  void (*fn)();
} menu_t;


void __josua_message();
void __josua_start_call();
void __josua_answer_call();
void __josua_on_hold_call();
void __josua_off_hold_call();
void __josua_terminate_call();
void __josua_transfer_call();
void __josua_set_up();
void __josua_quit();

void __josua_register();

/* some external methods */

#define MENU_DEFAULT 0
#define MENU_MG      1
#define MENU_MC      2
#define MENU_TR      3
#define MENU_RG      4
#define MENU_SETUP   5
#define MENU_AN      6
#define MENU_TC      7
#define MENU_HLD     8
#define MENU_OHLD    9

/********************************/
/***** Main Menu definition *****/

#define NBELEMENT_IN_MENU 11

static const menu_t josua_menu[]= {
  { "message",	"m",	" [M]ini-message", MENU_MG,
    " Exchange Mini-message.", &__josua_message },
  { "invite",	"i",	" [I]nvite", MENU_MC,
    " Make a call.",                     &__josua_start_call },
  { "answer",	"a",	" [A]nswer", MENU_AN,
    " Answer a call.",                     &__josua_answer_call },
  { "bye",	"b",	" [B]ye",    MENU_TC,
    " Disconnect active branch.",	&__josua_terminate_call },
  { "cancel",	"c",	" [C]ancel", MENU_TC,
    " Cancel early dialog.",	        &__josua_terminate_call },
  { "hold",	"h",	" [H]old",   MENU_HLD,
    " Put remote party on hold.",	&__josua_on_hold_call },
  { "Off hold",	"o",	" [O]ff Hold",   MENU_OHLD,
    " Put remote party on hold.",	&__josua_off_hold_call },
  { "transfer","t",	" [T]ransfer", MENU_TR,
    " Make a call transfer.",   	        &__josua_transfer_call  },
  { "register","r",	" [R]egister",  MENU_RG,
    " Register your location.",	        &__josua_register  },
  { "set",	"s",	" [S]etup",     MENU_SETUP,
    " Configure user agents and identities.",    &__josua_set_up  },
  { "quit",	"q",	"[Q]uit",      MENU_DEFAULT,
    "Quit jack' Open Sip User Agent.",  &__josua_quit    },
  { 0 }
};


/********************************/
/***** Sub  Menu definition *****/

typedef struct nctab_t {
  char   element[100];
  char   field[100];
  int    field_yoffset; /* positive or negative  */
  int    field_xoffset;
  char   value[100];
  int    value_yoffset;
  int    value_xoffset;
  short  editable;
} nctab_t;

#define JOSUA_NONEDITABLE  0 /* field is ediatable */
#define JOSUA_EDITABLE     1

#define COLORPAIR_NEWCALL       5  /* color pair for new call */
#define COLORPAIR_NEWMESSAGE    4
#define COLORPAIR_ANSWERCALL    3
#define COLORPAIR_JOSUASETUP    6
#define COLORPAIR_TRANSFERCALL  5
#define COLORPAIR_TERMINATECALL 5
#define COLORPAIR_ONHOLDCALL    4
#define COLORPAIR_OFFHOLDCALL   4


#define TABSIZE_NEWCALL         7

nctab_t nctab_newcall[] = {
  { { '\0' },
    "Call informations  %-80.80s\n" , -9 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "from",
    "  From         : %-80.80s\n" , -8 , 0,
    { '\0' } , -8, 18, JOSUA_NONEDITABLE },
  { "to",
    "  To           : %-80.80s\n" , -7 , 0,
    { '\0' } , -7, 18, JOSUA_EDITABLE  },
  { "subject",
    "  Subject      : %-80.80s\n" , -6 , 0,
    { '\0' } , -6, 18, JOSUA_EDITABLE  },
  { "route",
    "  Route        : %-80.80s\n" , -5 , 0,
    { '\0' } , -5, 18, JOSUA_EDITABLE  },
  { "payload",
    "  Audio payload: [0 4 8]%-80.80s\n" , -4 , 0,
    { '\0' } , -4, 18, JOSUA_EDITABLE  },
  { "infos",
    "  Other Info   : %-80.80s\n" , -3 , 0,
    { '\0' } , -3, 18, JOSUA_EDITABLE  }
};

#define TABSIZE_NEWMESSAGE      5

nctab_t nctab_newmessage[] = {
  { { '\0' },
    "Message Sessions  %-80.80s\n" , -9 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "from",
    "  From          : %-80.80s\n" , -8 , 0,
    { '\0' } , -8, 18, JOSUA_NONEDITABLE },
  { "to",
    "  To            : %-80.80s\n" , -7 , 0,
    { '\0' } , -7, 18, JOSUA_EDITABLE  },
  { "route",
    "  Route         : %-80.80s\n" , -6 , 0,
    { '\0' } , -6, 18, JOSUA_EDITABLE  },
  { "message",
    "  Message       : %-80.80s\n" , -5 , 0,
    { '\0' } , -5, 18, JOSUA_EDITABLE  }
};

#define TABSIZE_ANSWERCALL      3

nctab_t nctab_answercall[] = {
  { { '\0' },
    "Answer Code  %-80.80s\n" , -9 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "id",
    "  Id for Call  : %-80.80s\n" , -8 , 0,
    { '\0' } , -8, 18, JOSUA_EDITABLE },
  { "code",
    "  Reply Code   : %-80.80s\n" , -7 , 0,
    { '\0' } , -7, 18, JOSUA_EDITABLE }
};

#define TABSIZE_JOSUASETUP      6

nctab_t nctab_josuasetup[] = {
  { { '\0' },
    "Jousa's Config %-80.80s\n" , -8 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "identity",
    "  sip Identity : %-80.80s\n" , -7 , 0,
    { '\0' } , -7, 18, JOSUA_EDITABLE },
  { "registrar",
    "  Registrar    : %-80.80s\n" , -6 , 0,
    { '\0' } , -6, 18, JOSUA_EDITABLE  },
  { "realm",
    "  Realm        : %-80.80s\n" , -5 , 0,
    { '\0' } , -5, 18, JOSUA_EDITABLE  },
  { "userid",
    "  User id      : %-80.80s\n" , -4 , 0,
    { '\0' } , -4, 18, JOSUA_EDITABLE  },
  { "passwd",
    "  Password     : %-80.80s\n" , -3 , 0,
    { '\0' } , -3, 18, JOSUA_EDITABLE  }
};

#define TABSIZE_TRANSFERCALL    4

nctab_t nctab_transfercall[] = {
  { { '\0' },
    "Transfer a call %-80.80s\n" , -8 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "id",
    "  Id for Call  : %-80.80s\n" , -7 , 0,
    { '\0' } , -7, 18, JOSUA_EDITABLE },
  { "refer-to",
    "  Refer-To     : %-80.80s\n" , -6 , 0,
    { '\0' } , -6, 18, JOSUA_EDITABLE },
  { "also",
    "  Also         : %-80.80s\n" , -5 , 0,
    { '\0' } , -5, 18, JOSUA_EDITABLE }
};

#define TABSIZE_TERMINATECALL   2

nctab_t nctab_terminatecall[] = {
  { { '\0' },
    "Terminate a call  %-80.80s\n" , -4 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "id",
    "  Id for Call  : %-80.80s\n" , -3 , 0,
    { '\0' } , -3, 18, JOSUA_EDITABLE }
};

#define TABSIZE_ONHOLDCALL      2

nctab_t nctab_onholdcall[] = {
  { { '\0' },
    "Put on hold a call  %-80.80s\n" , -4 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "id",
    "  Id for Call  : %-80.80s\n" , -3 , 0,
    { '\0' } , -3, 18, JOSUA_EDITABLE }
};

#define TABSIZE_OFFHOLDCALL     2

nctab_t nctab_offholdcall[] = {
  { { '\0' },
    "Put off hold a call  %-80.80s\n" , -4 , 0,
    { '\0' } , 0, 0  , JOSUA_NONEDITABLE },
  { "id",
    "  Id for Call  : %-80.80s\n" , -3 , 0,
    { '\0' } , -3, 18, JOSUA_EDITABLE }
};

static int icon = 0;

#ifndef JD_EMPTY

#define JD_EMPTY          0
#define JD_INITIALIZED    1
#define JD_TRYING         2
#define JD_QUEUED         3
#define JD_RINGING        4
#define JD_ESTABLISHED    5
#define JD_REDIRECTED     6
#define JD_AUTH_REQUIRED  7
#define JD_CLIENTERROR    8
#define JD_SERVERERROR    9
#define JD_GLOBALFAILURE  10
#define JD_TERMINATED     11

#define JD_MAX            11

#endif

static const char *icons[]= {
#define JD_EMPTY          0
    "            " ,
    "            " ,
    " WELCOME    " ,
    "   TO       " ,
    "      JOSUA " ,
    "            " ,
    "  ________  " ,
    " / ______ \\ " ,
    " |_\\    /_/ " ,
    "            " ,
    "            " ,

#define JD_INITIALIZED    1
    "            " ,
    "            " ,
    "     __     " ,
    "    /  \\    " ,
    "       /    " ,
    "      |     " ,
    "  ____O____ " ,
    " / ______ \\ " ,
    " |_\\     /_/" ,
    "            " ,
    "            " ,

#define JD_TRYING         2
    "            " ,
    "            " ,
    "     __     " ,
    "    /  \\    " ,
    "       /    " ,
    "      |     " ,
    "  ____O____ " ,
    " / ______ \\ " ,
    " |_\\     /_/" ,
    "            " ,
    "            " ,

#define JD_QUEUED         3
    "            " ,
    "            " ,
    "            " ,
    "            " ,
    "  QUEUED    " ,
    "            " ,
    "  _________ " ,
    " / ______ \\ " ,
    " |_\\    /_/ " ,
    "            " ,
    "            " ,

#define JD_RINGING        4
    "            " ,
    "            " ,
    "            " ,
    "            " ,
    "    DRING   " ,
    "DRING       " ,
    "  ____DRING " ,
    " / ______ \\ " ,
    " |_\\    /_/ " ,
    "            " ,
    "            " ,

#define JD_ESTABLISHED    5
    "            " ,
    "            " ,
    "       __   " ,
    "      /  /  " ,
    "     /--/BLA" ,
    "    //  BLA " ,
    "   //       " ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_REDIRECTED     6
    "            " ,
    "            " ,
    " REDIRECTED " ,
    "<---- /  /  " ,
    "     /--/   " ,
    "    //      " ,
    "   //       " ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_AUTH_REQUIRED  7
    "            " ,
    "            " ,
    "       ___  " ,
    "      /  /  " ,
    "     /--/   " ,
    " NEED/      " ,
    "  CREDENTIAL" ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_CLIENTERROR    8
    "            " ,
    "            " ,
    "       ___  " ,
    "      /  /  " ,
    "     /--/   " ,
    " CLIENT     " ,
    "   / /ERROR " ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_SERVERERROR    9
    "            " ,
    "            " ,
    "       ___  " ,
    "      /  /  " ,
    "     /--/   " ,
    " SERVER     " ,
    "   / /ERROR " ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_GLOBALFAILURE  10
    "            " ,
    "            " ,
    "       ___  " ,
    "      /  /  " ,
    "     /--/   " ,
    " GLOBAL     " ,
    "   /FAILURE " ,
    "  / /_      " ,
    "  |__/      " ,
    "            " ,
    "            " ,

#define JD_TERMINATED     11
    "            " ,
    "            " ,
    "            " ,
    " CLOSED     " ,
    "            " ,
    "            " ,
    "  _________ " ,
    " / ______  \\" ,
    " |_\\    /_/ " ,
    "            " ,
    "            " ,

    /*
      #define MUTED  5
      "        __  " ,
      " MUTED /  / " ,
      "      /--/  " ,
      "     //     " ,
      "    //      " ,
      "   / /_     " ,
      "   |__/     " ,
      "            " ,
    */
};

/*
  Possible colors:

  COLOR_BLACK
  COLOR_RED
  COLOR_GREEN
  COLOR_YELLOW
  COLOR_BLUE
  COLOR_MAGENTA
  COLOR_CYAN
  COLOR_WHITE
*/

struct colordata {
       int fore;
       int back;
       int attr;
};

struct colordata color[]= {
  /* fore              back            attr */
  {COLOR_WHITE,        COLOR_BLACK,    0			}, // 0 default
  {COLOR_WHITE,        COLOR_GREEN,      A_BOLD			}, // 1 title
  {COLOR_WHITE,        COLOR_BLACK,    0			}, // 2 list
  {COLOR_WHITE,        COLOR_YELLOW,   A_REVERSE		}, // 3 listsel
  {COLOR_WHITE,        COLOR_BLUE,     0			}, // 4 calls
  {COLOR_WHITE,        COLOR_RED,      0			}, // 5 query
  {COLOR_BLACK,        COLOR_CYAN,     0			}, // 6 info
  {COLOR_WHITE,        COLOR_BLACK,    0			}, // 7 help
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

void dme(int i, int so) {
  int y,x;
  char buf[120];
  const menu_t *me= &josua_menu[i];
  sprintf(buf,"  %s   %c%c %d. %-11.11s %-80.80s ",
	  icons[icon*11+i],
          so ? '-' : ' ',
          so ? '>' : ' ', i,
          me->option,
          me->text);
  
  getmaxyx(stdscr,y,x);

  attrset(COLOR_PAIR(5));
  if (so)
    attrset(so ? A_REVERSE : A_NORMAL);
  mvaddnstr(i+1,0, buf,x-1);
  attrset(A_NORMAL);
}

/* unused??? 
void print_register_menu()
{
  char buf[120];
  int y,x;

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  refresh();
  getmaxyx(stdscr,y,x);

  attrset(A_BOLD);
  sprintf(buf,"  Info for new Call: %-80.80s\n", " ");
  mvaddnstr(2,0,buf,x-1);
  attrset(A_NORMAL);
}
*/


/* Print a table of elements in a ncurses window.
   nctab_t *nctab       table of element to print.
   short    nctab_size  size of table.
   short    y           offset
   short    x           offset
   int      colorpair   ncurses color pair.
*/   
void
nctab_print(nctab_t (*nctab)[],
	    int    nctab_size,
	    int    colorpair)
{
  char buf[250];
  int i;
  int vlength,hlength;

  cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  getmaxyx(stdscr,vlength,hlength);

  attrset(COLOR_PAIR(colorpair));

  /* (nctab_t (*)[]) */
  for (i=0; i<nctab_size ;i++)
    {
      if ((*nctab)[i].field_xoffset!=0)
	{
	  sprintf(buf, "%-40.40s\n", " ");
	  mvaddnstr(vlength + (*nctab)[i].field_yoffset,
		    0,
		    buf , (*nctab)[i].field_xoffset);
	}

      if ( (*nctab)[i].field[0]!='\0' )
	sprintf(buf, (*nctab)[i].field, " ");
      else
	sprintf(buf, "%-80.80s\n", " ");

      //sprintf(buf, "qsdqsdqdsq\n");
      mvaddnstr(vlength + (*nctab)[i].field_yoffset,
		(*nctab)[i].field_xoffset,
		buf , hlength - 1 - (*nctab)[i].field_xoffset);

      if ( (*nctab)[i].value[0]!='\0' )
	{
	  strncpy(buf, (*nctab)[i].value, 99);
	  mvaddnstr(vlength + (*nctab)[i].value_yoffset,
		    (*nctab)[i].value_xoffset,
		    buf , hlength - 1 - (*nctab)[i].value_xoffset);
	}

    }
}

char *nctab_findvalue(nctab_t (*nctab)[], int nctab_size, char *field)
{
  int i;
  for (i=0;i<nctab_size;i++)
    {
      if ((*nctab)[i].value!='\0')
	{
	  if (0==strcasecmp((*nctab)[i].element, field))
	    return (*nctab)[i].value;
	}
    }
  return NULL;
}

/* Print a table of elements in a ncurses window.
   nctab_t *nctab       table of element to print.
   short    nctab_size  size of table.
   short    y           offset
   short    x           offset
   int      colorpair   ncurses color pair.
*/   
int
nctab_get_values(nctab_t (*nctab)[],
		int    nctab_size,
		int    colorpair)
{
  char buf[250];
  int c;
  int i;
  int vlength,hlength;

  getmaxyx(stdscr,vlength,hlength);
  nocbreak(); echo(); nonl(); keypad(stdscr,TRUE);

  attrset(COLOR_PAIR(colorpair));
  refresh();

  /* (nctab_t (*)[]) */
  for (i=0; i<nctab_size ;i++)
    {
      if ((*nctab)[i].editable==JOSUA_NONEDITABLE)
	{
	  /* go to the next entry */
	}
      else
	{
	  char tmp[100];
	  if ((*nctab)[i].value[0]=='\0')
	    {
	      mvwgetnstr(stdscr, vlength+(*nctab)[i].value_yoffset,
			 (*nctab)[i].value_xoffset,
			 tmp, 99);
	      
	      if (tmp[0]!='\0')
		sprintf((*nctab)[i].value, tmp);
	    }
	}
    }

  sprintf(buf,"Confirm? [y|n] : \n");
  mvaddnstr(vlength-2,40,buf,hlength-1);

  cbreak(); noecho();
  c = mvgetch(vlength-2, 40+18);
  while (c!='y' && c!='n')
    { c = mvgetch(vlength-2, 40+18); }
  
  return c;
}


void print_calls()
{
  char buf[256];
  int yline;
  int y,x;
  eXosip_call_t *jc;
  eXosip_osip_dialog_t *jd;

  cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  getmaxyx(stdscr,y,x);
  

  attrset(COLOR_PAIR(4));
  eXosip_update();
  yline = 12;
  
  eXosip_lock();
  for (jc = eXosip.j_calls; jc!=NULL; jc = jc->next)
    {
      for (jd = jc->c_dialogs; jd!=NULL; jd = jd->next)
	{
	  char *tmp;
	  if (jd->d_dialog!=NULL)
	    {
	      if (jd->d_dialog->state==DIALOG_EARLY)
		{
		  osip_to_to_str(jd->d_dialog->remote_uri, &tmp);
		  sprintf(buf,"C%i D%i: Pending Call To: %-80.80s\n",
			  jc->c_id, jd->d_id,
			  tmp);
		  osip_free(tmp);
		}
	      else /* if (jd->d_dialog->state!=DIALOG_EARLY) */
		{
		  osip_to_to_str(jd->d_dialog->remote_uri, &tmp);
		  sprintf(buf,"C%i D%i: Established Call To: %-80.80s\n",
			  jc->c_id, jd->d_id,
			  tmp);
		  osip_free(tmp);
		}
	    }
	  else 
	    {
	      sprintf(buf,"C%i D%i: Connection closed %-80.80s\n",
		      jc->c_id, jd->d_id, " \0");
	    }
	  mvaddnstr(yline,0,buf,x-1);
	  yline++;
	}
      if (jc->c_dialogs==NULL)
	{
	  osip_transaction_t *tr = jc->c_out_tr;
	  char *tmp;
	  if (tr==NULL)
	    {
	      tr = jc->c_inc_tr;
	      if (tr==NULL) /* Bug ?? */
		{
		  eXosip_unlock();
		  return;
		}
	      osip_from_to_str(tr->orig_request->from, &tmp);
	    }
	  if (tr->orig_request==NULL) /* info not yet available */
	    {}
	  else osip_to_to_str(tr->orig_request->to, &tmp);

	  sprintf(buf,"C%i D-1: with: %-80.80s\n", jc->c_id, tmp);
	  osip_free(tmp);
	  mvaddnstr(yline,0,buf,x-1);
	  yline++;
	}
    }

  jc = eXosip.j_calls;
  if (jc!=NULL && jc->c_dialogs!=NULL)
    icon = jc->c_dialogs->d_STATE;
  else if (jc!=NULL)
    {
      icon = 0;
    }
    
  eXosip_unlock();

}


void print_friend(int i, jfriend_t *fr, int so)
{
  int y,x;
  char buf[120];
  int pos = i;
  eXosip_lock();
  for (;fr!=NULL && (pos!=0); fr=fr->next)
    pos--;
  if (fr==NULL)
    {
      eXosip_unlock();
      return ;
    }
  sprintf(buf,"%c%c %d. %-11.11s %-80.80s ",
          so ? '-' : ' ',
          so ? '>' : ' ', i,
          fr->f_nick,
          fr->f_home);
  eXosip_unlock();
  
  getmaxyx(stdscr,y,x);

  attrset(COLOR_PAIR(6));
  if (so)
    attrset(so ? A_REVERSE : A_NORMAL);
  mvaddnstr(i+1,0, buf,x-1);
  attrset(A_NORMAL);
}

void print_identity(int i, jidentity_t *fr, int so)
{
  int y,x;
  char buf[120];
  int pos = i;
  eXosip_lock();
  for (;fr!=NULL && (pos!=0); fr=fr->next)
    pos--;
  if (fr==NULL)
    {
      eXosip_unlock();
      return ;
    }
  sprintf(buf,"%c%c %d. %-30.30s %-80.80s ",
          so ? '-' : ' ',
          so ? '>' : ' ', i,
          fr->i_identity,
          fr->i_registrar);
  eXosip_unlock();
  
  getmaxyx(stdscr,y,x);

  attrset(COLOR_PAIR(6));
  if (so)
    attrset(so ? A_REVERSE : A_NORMAL);
  mvaddnstr(i+1,0, buf,x-1);
  attrset(A_NORMAL);
}

void print_menu(int menu)
{
  int y,x;
  char buf[120];
  const menu_t *mep;
  int i;

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  refresh();
  clear();

  sprintf(buf,"All right reserved. Copyright 2002 Aymeric Moizard. %-50.50s"," ");
  getmaxyx(stdscr,y,x);
  attrset(A_NORMAL);
  attrset(COLOR_PAIR(1));
  mvaddnstr(0,0, buf,x-1);

  attrset(A_NORMAL);
  for (mep=josua_menu, i=0; mep->option && mep->text; mep++, i++)
    dme(i,0);

  attrset(A_NORMAL);

  if (menu==MENU_DEFAULT)
    {}
  else if (menu==MENU_MC)
    {
      nctab_print(&nctab_newcall,
		  TABSIZE_NEWCALL, 
		  COLORPAIR_NEWCALL);
    }
  else if (menu==MENU_TR)
    {
      nctab_print(&nctab_transfercall,
		  TABSIZE_TRANSFERCALL, 
		  COLORPAIR_TRANSFERCALL);
      //print_transfer_menu();
    }
  else if (menu==MENU_RG)
    {
      // print_register_menu();
    }
  else if (menu==MENU_SETUP)
    {
      nctab_print(&nctab_josuasetup,
		  TABSIZE_JOSUASETUP, 
		  COLORPAIR_JOSUASETUP);
      // print_set_up_menu();
    }
  else if (menu==MENU_MG)
    {
      nctab_print(&nctab_newmessage,
		  TABSIZE_NEWMESSAGE, 
		  COLORPAIR_NEWCALL);
    }
  else if (menu==MENU_AN)
    {
      nctab_print(&nctab_answercall,
		  TABSIZE_ANSWERCALL, 
		  COLORPAIR_ANSWERCALL);
	//print_answer_menu();
    }
  else if (menu==MENU_TC)
    {
      nctab_print(&nctab_terminatecall,
		  TABSIZE_TERMINATECALL, 
		  COLORPAIR_TERMINATECALL);
    }
  else if (menu==MENU_HLD)
    {
      nctab_print(&nctab_onholdcall,
		  TABSIZE_ONHOLDCALL, 
		  COLORPAIR_ONHOLDCALL);
    }
  else if (menu==MENU_OHLD)
    {
      nctab_print(&nctab_offholdcall,
		  TABSIZE_OFFHOLDCALL, 
		  COLORPAIR_OFFHOLDCALL);
    }
  print_calls();
}


int __josua_choose_friend_in_list() {
#define C(x) ((x)-'a'+1)
  int c, i;
  int cursor=0;
  jfriend_t *fr;
  int max;

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  refresh();
  clear();

  i=0;
  if (eXosip.j_friends!=NULL)
    {
      i=1;
      print_friend(0, eXosip.j_friends, 1);
      for (fr = eXosip.j_friends->next; fr!=NULL; fr=fr->next, i++)
	print_friend(i, eXosip.j_friends, 0);
    }

  cursor = 0;
  max = i;
  for (;;) {
    refresh();
    do
      c= getch();
    while (c == ERR && errno == EINTR);
    if (c==ERR)  {
      if(errno != 0)
	fprintf(stderr, "failed to getch in main menu\n");
      else {
	/*
	  clearok(stdscr,TRUE);
	  clear();
	  print_menu(0);
	  dme(cursor,1); */
      }
    }
    
    if (c==C('n') || c==KEY_DOWN || c==' ' || c=='j') {
      print_friend(cursor, eXosip.j_friends, 0);
      cursor++; cursor %= max;
      print_friend(cursor, eXosip.j_friends, 1);
    } else if (c==C('p') || c==KEY_UP || c==C('h') ||
               c==KEY_BACKSPACE || c==KEY_DC || c=='k') {
      print_friend(cursor, eXosip.j_friends, 0);
      cursor+= max-1; cursor %= max;
      print_friend(cursor, eXosip.j_friends, 1);

    } else if (c=='\n' || c=='\r' || c==KEY_ENTER) {
      clear(); refresh();
      print_friend(cursor, eXosip.j_friends, 1);
      return cursor;
    } else if (isdigit(c)) {
      char buf[2]; buf[0]=c; buf[1]=0; c=atoi(buf);
      if (c < max) {
	print_friend(cursor, eXosip.j_friends, 0);
	cursor=c;
	print_friend(cursor, eXosip.j_friends, 1);
      } else {
        beep();
      }
    } else {
      beep();
    }
  }  
}

int __josua_choose_identity_in_list() {
#define C(x) ((x)-'a'+1)
  int c, i;
  int cursor=0;
  jidentity_t *ji;
  int max;

  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
  refresh();
  clear();

  i=0;
  if (eXosip.j_identitys!=NULL)
    {
      i=1;
      print_identity(0, eXosip.j_identitys, 1);
      for (ji = eXosip.j_identitys->next; ji!=NULL; ji=ji->next, i++)
	print_identity(i, eXosip.j_identitys, 0);
    }

  cursor = 0;
  max = i;
  for (;;) {
    refresh();
    do
      c= getch();
    while (c == ERR && errno == EINTR);
    if (c==ERR)  {
      if(errno != 0)
	fprintf(stderr, "failed to getch in main menu\n");
      else {
	/*
	  clearok(stdscr,TRUE);
	  clear();
	  print_menu(0);
	  dme(cursor,1); */
      }
    }
    
    if (c==C('n') || c==KEY_DOWN || c==' ' || c=='j') {
      print_identity(cursor, eXosip.j_identitys, 0);
      cursor++; cursor %= max;
      print_identity(cursor, eXosip.j_identitys, 1);
    } else if (c==C('p') || c==KEY_UP || c==C('h') ||
               c==KEY_BACKSPACE || c==KEY_DC || c=='k') {
      print_identity(cursor, eXosip.j_identitys, 0);
      cursor+= max-1; cursor %= max;
      print_identity(cursor, eXosip.j_identitys, 1);

    } else if (c=='\n' || c=='\r' || c==KEY_ENTER) {
      clear(); refresh();
      print_identity(cursor, eXosip.j_identitys, 1);
      return cursor;
    } else if (isdigit(c)) {
      char buf[2]; buf[0]=c; buf[1]=0; c=atoi(buf);
      if (c < max) {
	print_identity(cursor, eXosip.j_identitys, 0);
	cursor=c;
	print_identity(cursor, eXosip.j_identitys, 1);
      } else {
        beep();
      }
    } else {
      beep();
    }
  }  
}

/*
  curseson(); cbreak(); noecho(); nonl(); keypad(stdscr,TRUE);
*/
void __josua_menu() {
#define C(x) ((x)-'a'+1)
  int c, i;
  int cursor=0;
  dme(0,1);
  for (;;) {
    refresh();
    do
      {	
	print_calls();
	refresh();
	halfdelay(1);
	c= getch();
      }
    while (c == ERR && errno == EINTR);
    if (c==ERR)  {
      if(errno != 0)
	fprintf(stderr, "failed to getch in main menu\n");
      else {
	/*
	  clearok(stdscr,TRUE);
	  clear();
	  print_menu(0);
	  dme(cursor,1); */
      }
    }
    
    if (c==C('n') || c==KEY_DOWN || c==' ' || c=='j') {
      dme(cursor,0); cursor++; cursor %= NBELEMENT_IN_MENU; dme(cursor,1);
    } else if (c==C('p') || c==KEY_UP || c==C('h') ||
               c==KEY_BACKSPACE || c==KEY_DC || c=='k') {
      dme(cursor,0); cursor+= NBELEMENT_IN_MENU-1; cursor %= NBELEMENT_IN_MENU; dme(cursor,1);
    } else if (c=='\n' || c=='\r' || c==KEY_ENTER) {
      clear(); refresh();
      if (cursor==NBELEMENT_IN_MENU)
	return ;
      print_menu(josua_menu[cursor].menu_id); dme(cursor,1);
      josua_menu[cursor].fn();
      cursor++; cursor %= NBELEMENT_IN_MENU;
      print_menu(0); dme(cursor,1);
    } else if (c==C('l')) {
      clearok(stdscr,TRUE); clear(); print_menu(0); dme(cursor,1);
    } else if (isdigit(c)) {
      char buf[2]; buf[0]=c; buf[1]=0; c=atoi(buf);
      if (c < NBELEMENT_IN_MENU) {
        dme(cursor,0); cursor=c; dme(cursor,1);
      } else {
        beep();
      }
    } else if (isalpha(c)) {
      c= tolower(c);
      for (i=0; i<NBELEMENT_IN_MENU && (josua_menu[i].key)[0] != c; i++);
      if (i < NBELEMENT_IN_MENU) {
        dme(cursor,0); cursor=i; dme(cursor,1);
      } else {
        beep();
      }
    } else {
      beep();
    }
  }
}

typedef struct _main_config_t {
  char  config_file[256];    /* -f <config file>   */
  char  identity_file[256];  /* -I <identity file> */
  char  contact_file[256];   /* -C <contact file>  */
  char  log_file[256];       /* -L <log file>      */
  int   debug_level;         /* -d <verbose level>   */
  int   port;                /* -p <SIP port>  default is 5060 */
  char  identity[256];       /* -i <from url>  local identity */

  /* the application can command to make a new call */
  char  to[255];             /* -t <sipurl to call>!  */
  char  route[256];          /* -r <sipurl for route> */
  char  subject[256];        /* -s <subject>          */
  int   timeout;        /* -T <delay> connection is closed after 60s */
} main_config_t;

main_config_t cfg = {
  "\0", "\0", "\0", "\0", 1, 5060, "\0", "\0", "\0", "\0", 60
};

#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif


void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   josua args\n\
\n\
\t [-f <config file>]\n\
\t [-I <identity file>]\n\
\t [-C <contact file>]\n\
\t [-L <log file>]\n\
\t [-f <from url>]\n\
\t [-d <verbose level>]\n\
\t [-p <SIP port>]\n\
\t [-h]\n\
\t [-i]                       interactive mode\n\
\t [-v]\n\
\n\
  arguments can be used to make a new call.\n\
\n\
\t [-t <sipurl to call>]\n\
\t [-r <sipurl for route>]\n\
\t [-s <subject>]\n\
\t [-T <delay>]               close calls after 60s as a default timeout.\n");
    exit (code);
}

int main(int argc, const char *const *argv) {

  /* deal with options */
  FILE *log_file = NULL;
  char c;
  int i;
  ppl_getopt_t *opt;
  ppl_status_t rv;
  const char *optarg;

  if (argc > 1 && strlen (argv[1]) == 1 && 0 == strncmp (argv[1], "-", 2))
    usage (0);
  if (argc > 1 && strlen (argv[1]) >= 2 && 0 == strncmp (argv[1], "--", 2))
    usage (0);

  ppl_getopt_init (&opt, argc, argv);

#define __APP_BASEARGS "F:I:C:L:f:d:p:t:r:s:T:vVih?X"
  while ((rv = ppl_getopt (opt, __APP_BASEARGS, &c, &optarg)) == PPL_SUCCESS)
    {
      switch (c)
        {
          case 'F':
            snprintf(cfg.config_file, 255, optarg);
            break;
          case 'I':
            snprintf(cfg.identity_file, 255, optarg);
            break;
          case 'C':
            snprintf(cfg.contact_file, 255, optarg);
            break;
          case 'L':
            snprintf(cfg.log_file, 255, optarg);
            break;
          case 'f':
            snprintf(cfg.identity, 255, optarg);
            break;
          case 'd':
            cfg.debug_level = atoi (optarg);
            break;
          case 'p':
            cfg.port = atoi (optarg);
            break;
          case 't':
            snprintf(cfg.to, 255, optarg);
            break;
          case 'r':
            snprintf(cfg.route, 255, optarg);
            break;
          case 's':
            snprintf(cfg.subject, 255, optarg);
            break;
          case 'T':
            cfg.timeout = atoi(optarg);
            break;
          case 'v':
          case 'V':
            printf ("josua: version:     %s\n", VERSION);
            printf ("build: %s\n", server_built);
            exit (0);
          case 'h':
          case '?':
            printf ("josua: version:     %s\n", VERSION);
            printf ("build: %s\n", server_built);
            usage (0);
	    break;
          default:
            /* bad cmdline option?  then we die */
            usage (1);
        }
    }

  if (rv != PPL_EOF)
    {
      usage (1);
    }

  if (cfg.identity[0]!='\0')
    strncpy(nctab_newcall[1].value, cfg.identity, 99);
  else
    strncpy(nctab_newcall[1].value, from, 99);

  if (cfg.identity[0]!='\0')
    strncpy(nctab_newmessage[1].value, cfg.identity, 99);
  else
    strncpy(nctab_newmessage[1].value, from, 99);

  if (cfg.debug_level > 0)
    {
      printf ("josua: %s\n", VERSION);
      printf ("Debug level:        %i\n", cfg.debug_level);
      printf ("Config name:        %s\n", cfg.config_file);
      if (cfg.log_file == NULL)
        printf ("Log name:           Standard output\n");
      else
        printf ("Log name:           %s\n", cfg.log_file);
    }

  /*********************************/
  /* INIT Log File and Log LEVEL   */

  if (cfg.debug_level > 0)
    {
      if (cfg.log_file[0] != '\0')
        {
          log_file = fopen (cfg.log_file, "w+");
      } else
        log_file = stdout;
      if (NULL == log_file)
        {
          perror ("josua: log file");
          exit (1);
        }
      TRACE_INITIALIZE (cfg.debug_level, log_file);
    }

  osip_free ((void *) (opt->argv));
  osip_free ((void *) opt);

  if (cfg.identity[0]=='\0')
    {
      fprintf (stderr, "josua: specify an identity\n");
      fclose(log_file);
      exit(1);
    }

  i = eXosip_init(stdin, stdout, cfg.port);
  if (i!=0)
    {
      fprintf (stderr, "josua: could not initialize eXosip\n");
      fclose(log_file);
      exit(0);
    }

  jfriend_load();
  jidentity_load();

  if (cfg.to[0]!='\0')
    { /* start a command line call, if needed */
      osip_message_t *invite;
      i = eXosip_build_initial_invite(&invite,
				      cfg.to,
				      cfg.identity,
				      cfg.route,
				      cfg.subject);
      if (i!=0)
	{
	  fprintf (stderr, "josua: (bad arguments?)\n");
	  exit(0);
	}
      eXosip_lock();
      eXosip_start_call(invite);
      eXosip_unlock();
    }

  print_menu(0);
  __josua_menu();
  echo();
  cursesoff();
  
  fclose(log_file);
  exit(1);
  return(0);
}

void __josua_message() {
  //  osip_message_t *message;
  char buf[120];

  int c;
  int y,x;

  int jid;
  char *tmp;
  jid = __josua_choose_friend_in_list();
  if (jid==-1) tmp = NULL;
  else
    {
      eXosip_lock();
      tmp = jfriend_get_home(jid);
      eXosip_unlock();
    }

  print_menu(0); dme(0,1);
    
  nctab_print(&nctab_newmessage,
	      TABSIZE_NEWMESSAGE, 
	      COLORPAIR_NEWMESSAGE);


  getmaxyx(stdscr,y,x);
 
  if (jid!=-1)
    {
      cbreak(); echo(); nonl(); keypad(stdscr,TRUE);
      attrset(COLOR_PAIR(COLORPAIR_NEWMESSAGE)); 
      sprintf(buf,"  To            : %-80.80s\n", tmp);
      mvaddnstr(y-7,0,buf,x-1);
      snprintf(nctab_newmessage[2].value, 99, "%s", tmp);
    }

  c = nctab_get_values(&nctab_newmessage,
		      TABSIZE_NEWMESSAGE,
		      COLORPAIR_NEWMESSAGE);

  if (c!='y')
    return;
  
  eXosip_message(nctab_findvalue(&nctab_newmessage,
				 TABSIZE_NEWMESSAGE,
				 "to"),
		 nctab_findvalue(&nctab_newmessage,
				 TABSIZE_NEWMESSAGE,
				 "from"),
		 nctab_findvalue(&nctab_newmessage,
				 TABSIZE_NEWMESSAGE,
				 "route"),
		 nctab_findvalue(&nctab_newmessage,
				 TABSIZE_NEWMESSAGE,
				 "message"));

}

void __josua_start_call() {
  osip_message_t *invite;
  int i;
  char buf[120];

  int c;
  int y,x;

  int jid;
  char *tmp;
  jid = __josua_choose_friend_in_list();
  if (jid==-1) tmp = NULL;
  else
    {
      eXosip_lock();
      tmp = jfriend_get_home(jid);
      eXosip_unlock();
    }
  
  print_menu(MENU_MC); dme(1,1);

  getmaxyx(stdscr,y,x);

  if (jid!=-1)
    {
      cbreak(); echo(); nonl(); keypad(stdscr,TRUE);
      attrset(COLOR_PAIR(COLORPAIR_NEWCALL)); 
      sprintf(buf,"  To           :  %-80.80s\n", tmp);
      mvaddnstr(y-7,0,buf,x-1);
      snprintf(nctab_newcall[2].value, 99, "%s", tmp);
    }

  c = nctab_get_values(&nctab_newcall,
		      TABSIZE_NEWCALL,
		      COLORPAIR_NEWCALL);

  if (c!='y')
    return;
  
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
  eXosip_start_call(invite);
  eXosip_unlock();

  refresh();
  /* make a call */
}

void __josua_answer_call() {

  int c;
  int cid;

  c = nctab_get_values(&nctab_answercall,
		      TABSIZE_ANSWERCALL,
		      COLORPAIR_ANSWERCALL);

  if (c!='y') return;

  c = osip_atoi(nctab_findvalue(&nctab_answercall,
			    TABSIZE_ANSWERCALL,
			    "code"));
  cid = osip_atoi(nctab_findvalue(&nctab_answercall,
			      TABSIZE_ANSWERCALL,
			      "id"));
  if (cid==-1) return;
  if (c==-1) return;
 
  eXosip_lock();
  eXosip_answer_call(cid , c); /* close 1st call with code (like 200) */
  eXosip_unlock();
  
}

void __josua_on_hold_call() {
  int c;

  c = nctab_get_values(&nctab_onholdcall,
		      TABSIZE_ONHOLDCALL,
		      COLORPAIR_ONHOLDCALL);

  if (c!='y') return;

  c = osip_atoi(nctab_findvalue(&nctab_onholdcall,
			    TABSIZE_ONHOLDCALL,
			    "id"));
  if (c==-1) return;

  eXosip_lock();
  eXosip_on_hold_call(c); /* number of the call in the list? */
  eXosip_unlock();
}

void __josua_off_hold_call() {
  int c;

  c = nctab_get_values(&nctab_offholdcall,
		      TABSIZE_OFFHOLDCALL,
		      COLORPAIR_OFFHOLDCALL);

  if (c!='y') return;

  c = osip_atoi(nctab_findvalue(&nctab_offholdcall,
			    TABSIZE_OFFHOLDCALL,
			    "id"));
  if (c==-1) return;

  eXosip_lock();
  eXosip_off_hold_call(c); /* number of the call in the list? */
  eXosip_unlock();
}

void __josua_terminate_call() {

  int c;

  c = nctab_get_values(&nctab_terminatecall,
		  TABSIZE_TERMINATECALL,
		  COLORPAIR_TERMINATECALL);

  if (c!='y') return;

  c = osip_atoi(nctab_findvalue(&nctab_terminatecall,
			    TABSIZE_TERMINATECALL,
			    "id"));
  if (c==-1) return;

  eXosip_lock();
  eXosip_terminate_call(c, 1); /* a SIP call can have several SIP dialog. */
  eXosip_unlock();
}

void __josua_transfer_call() {

  int cid;
  int c;

  c = nctab_get_values(&nctab_transfercall,
		      TABSIZE_TRANSFERCALL, 
		      COLORPAIR_TRANSFERCALL);
  
  if (c!='y') return;

  cid = osip_atoi(nctab_findvalue(&nctab_transfercall,
			      TABSIZE_TRANSFERCALL,
			      "id"));

  if (cid==-1) return;

  eXosip_lock();
  eXosip_transfer_call(cid, nctab_findvalue(&nctab_transfercall,
					    TABSIZE_TRANSFERCALL,
					    "refer-to"));
  eXosip_unlock();
  
}

void __josua_register() {
  char *tmp;
  char *tmp2;
  int jid;
  static int previous_jid = -2;
  jid = __josua_choose_identity_in_list();

  if (jid==-1) return;

  eXosip_lock();
  tmp = jidentity_get_identity(jid);
  tmp2 = jidentity_get_registrar(jid);
  eXosip_unlock();
  if (tmp==NULL||tmp2==NULL) return;

  if (jid!=previous_jid)
    {
      eXosip_register_init(tmp,
			  tmp2, NULL);
      previous_jid = jid;
    }

  eXosip_lock();
  eXosip_register(-1);
  eXosip_unlock();
}

void __josua_set_up() {

  osip_to_t *_to;
  osip_uri_t *_url;
  int c;
  int i;

  c = nctab_get_values(&nctab_josuasetup,
		      TABSIZE_JOSUASETUP, 
		      COLORPAIR_JOSUASETUP);
  
  if (c!='y') return;


  i = osip_to_init(&_to);
  if (i!=0) return;
  i = osip_to_parse(_to, nctab_findvalue(&nctab_josuasetup,
				    TABSIZE_JOSUASETUP,
				    "identity"));
  osip_to_free(_to);
  _to = NULL;
  if (i!=0) return;

  i = osip_uri_init(&_url);
  if (i!=0) return;
  i = osip_uri_parse(_url, nctab_findvalue(&nctab_josuasetup,
				    TABSIZE_JOSUASETUP,
				    "registrar"));
  osip_uri_free(_url);
  _url = NULL;
  if (i!=0) return;


  if (nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "realm") !='\0'
      || nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "password") !='\0'
      || nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "userid") !='\0')
    {
      if (nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "realm") =='\0'
	  || nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "password")=='\0'
	  || nctab_findvalue(&nctab_josuasetup, TABSIZE_JOSUASETUP, "userid") =='\0')
	{
	  fprintf(stderr, "josua: incomplete user informations.\n");
	  return;
	}
    }  

  identitys_add(nctab_findvalue(&nctab_josuasetup,
				TABSIZE_JOSUASETUP,
				"identity"),
		nctab_findvalue(&nctab_josuasetup,
				TABSIZE_JOSUASETUP,
				"registrar"),
		nctab_findvalue(&nctab_josuasetup,
				TABSIZE_JOSUASETUP,
				"realm"),
		nctab_findvalue(&nctab_josuasetup,
				TABSIZE_JOSUASETUP,
				"userid"),
		nctab_findvalue(&nctab_josuasetup,
				TABSIZE_JOSUASETUP,
				"password"));

}

void __josua_quit() {

  eXosip_quit();

  cursesoff();  
  exit(1);
}

#endif

