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


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#ifndef __EXOSIP2_H__
#define __EXOSIP2_H__

#include <osip2/smsg.h>

/* This is used to initiate the Extented/Open/Sip library */
int   eXosip_init(FILE *input, FILE *output, int port);
void  eXosip_update();
void  eXosip_quit();
int   eXosip_lock();
int   eXosip_unlock();

int   jfreind_load();
char *jfreind_get_home(int fid);
void  jfreind_unload();

void  identitys_add(char *identity, char *registrar, char *realm,
		   char *userid, char *password);
int   jidentity_load();
char *jidentity_get_identity(int fid);
char *jidentity_get_registrar(int fid);
void  jidentity_unload();

int   eXosip_build_initial_invite(sip_t **invite, char *to, char *from,
			       char *route, char *subject);
void  eXosip_start_call    (sip_t *invite);

void  eXosip_answer_call   (int jid, int status);
void  eXosip_on_hold_call  (int jid);
void  eXosip_off_hold_call (int jid);
void  eXosip_transfer_call(int jid, char *refer_to);
void  eXosip_terminate_call(int cid, int jid);

void  eXosip_message    (char *to, char *from, char *route, char *buff);

void  eXosip_register      (int rid);
void  eXosip_register_init (char *from, char *proxy, char *contact);


#endif
