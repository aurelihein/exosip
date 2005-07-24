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

#ifndef _GUI_H_
#define _GUI_H_


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
#include <osip2/osip_mt.h>

#include <eXosip2/eXosip.h>
#include "jcalls.h"
#include "jsubscriptions.h"
#include "jinsubscriptions.h"
#include "commands.h"

#include "ppl_getopt.h"


typedef struct gui
{
#define GUI_DISABLED -1
#define GUI_ON        0
#define GUI_OFF       1
  int on_off;
  int x0;
  int x1;
  int y0;
  int y1;
  int (*gui_clear) ();
  int (*gui_print) ();
  int (*gui_run_command) (int);
  int (*gui_key_pressed) ();
  void (*gui_draw_commands) ();

  int xcursor;
  int ycursor;
  int len;

  WINDOW *win;
} gui_t;



#define TOPGUI      0
#define ICONGUI     1
#define LOGLINESGUI 2
#define MENUGUI     3
#define EXTRAGUI    4
extern gui_t *gui_windows[10];
extern gui_t *active_gui;

int gui_start (void);
int josua_event_get (void);
void josua_printf (char *chfr, ...);

/* usefull method */
int josua_gui_clear (void);
int josua_gui_print (void);
int josua_gui_run_command (int c);
int josua_clear_box_and_commands (gui_t * box);
int josua_print_command (char **commands, int ypos, int xpos);
WINDOW *gui_print_box (gui_t * box, int draw, int color);

void curseson (void);
void cursesoff (void);

#endif
