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

#ifndef __GUI_MENU_H__
#define __GUI_MENU_H__

#include "gui.h"

int window_menu_print();
int window_menu_run_command(int c);

extern gui_t gui_window_menu;



typedef struct menu_t {
  const char *key;
  const char *text;
  void (*fn)();
} menu_t;


/* some external methods */


void __show_new_call();
void __show_new_message();
void __josua_manage_call();
void __josua_manage_subscribers();
void __josua_setup();
void __josua_quit();

void __josua_register();

#endif
