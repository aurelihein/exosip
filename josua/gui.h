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

#include "ppl_getopt.h"

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


int gui_start();
void josua_printf(char *chfr, ...);

void __josua_message();
void __josua_manage_call();
void __josua_start_call();
void __josua_set_up();
void __josua_manage_subscribers();
void __josua_quit();

void __josua_register();


#endif
