/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __JFRIENDS_H__
#define __JFRIENDS_H__

#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

typedef struct jfriend_t jfriend_t;

struct jfriend_t
{
  int f_id;
  char *f_nick;
  char *f_home;
  char *f_work;
  char *f_email;
  char *f_e164;

  jfriend_t *next;
  jfriend_t *parent;
};

int jfriend_load (void);
void jfriend_add (char *nickname, char *home, char *work, char *email, char *e164);
void jfriend_unload (void);
char *jfriend_get_home (int fid);
jfriend_t *jfriend_get (void);
void jfriend_remove (jfriend_t * fr);

#endif
