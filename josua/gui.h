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

#include <eXosip/eXosip.h>
#include <eXosip/eXosip2.h>
#include "jcalls.h"
#include "commands.h"

#include "ppl_getopt.h"


int gui_start();
int josua_event_get();
void josua_printf(char *chfr, ...);

void __show_new_call();
void __show_new_message();
void __josua_manage_call();
void __josua_set_up();
void __josua_manage_subscribers();
void __josua_quit();

void __josua_register();


#endif
