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

#include <osipparser2/osip_parser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* This is used to initiate the Extented/Open/Sip library */
int   eXosip_init(FILE *input, FILE *output, int port);
void  eXosip_update();
void  eXosip_quit();
int   eXosip_lock();
int   eXosip_unlock();

int   jfriend_load();
void  jfriend_unload();
void  friends_add(char *nickname, char *home,
		  char *work, char *email, char *e164);
char *jfriend_get_home(int fid);

int   jsubscriber_load();
void  jsubscriber_unload();
void  subscribers_add(char *nickname, char *uri, int black_list);
char *jsubscriber_get_uri(int fid);

int   jidentity_load();
void  jidentity_unload();
void  identitys_add(char *identity, char *registrar, char *realm,
		    char *userid, char *password);
char *jidentity_get_identity(int fid);
char *jidentity_get_registrar(int fid);

int   eXosip_build_initial_options(osip_message_t **info, char *to,
				   char *from, char *route);
int   eXosip_start_options(osip_message_t *options, void *reference,
			   void *sdp_context_reference,
			   char *local_sdp_port);
int   eXosip_build_initial_invite(osip_message_t **invite, char *to,
				  char *from, char *route, char *subject);
int   eXosip_start_call    (osip_message_t *invite, void *reference, void *sdp_context_reference, char *local_sdp_port);
int   eXosip_answer_call   (int jid, int status);
int   eXosip_answer_options   (int cid, int jid, int status);
int   eXosip_options_call     (int jid);
int   eXosip_on_hold_call  (int jid);
int   eXosip_off_hold_call (int jid);
int   eXosip_transfer_call(int jid, char *refer_to);
int   eXosip_terminate_call(int cid, int jid);

void  eXosip_message    (char *to, char *from, char *route, char *buff);

int   eXosip_register      (int rid);
int   eXosip_register_init (char *from, char *proxy, char *contact);

/* This is to manage outgoing subscription */
void  eXosip_subscribe(char *to, char *from, char *route);
void  eXosip_subscribe_terminate(int sid);
void  eXosip_subscribe_refresh(int sid);
void  eXosip_subscribe_close(int sid);

/* This is to manage incoming subscription */
void  eXosip_notify_accept_subscribe   (int nid, int code, int subscription_status, int online_status);
void  eXosip_notify(int nid, int subscription_status, int online_status);

#ifdef __cplusplus
}
#endif

#endif
