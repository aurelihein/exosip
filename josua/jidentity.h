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

#ifndef __JIDENTITYS_H__
#define __JIDENTITYS_H__

#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

typedef struct jidentity_t jidentity_t;

struct jidentity_t
{
  int i_id;
  char *i_identity;
  char *i_registrar;
  char *i_route;
  char *i_realm;
  char *i_userid;
  char *i_pwd;

  jidentity_t *next;
  jidentity_t *parent;
};

char *jidentity_get_identity (int fid);
char *jidentity_get_registrar (int fid);
char *jidentity_get_route (int fid);

int jidentity_load (void);
void jidentity_add (char *nickname, char *home,
                    char *work, char *email, char *e164);
void jidentity_unload (void);
char *jidentity_get_home (int fid);
jidentity_t *jidentity_get (void);

/* void jidentity_remove(jidentity_t *fr); */



#endif
