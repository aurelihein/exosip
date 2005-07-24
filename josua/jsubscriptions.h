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

#ifndef __JSUBSCRIPTIONS_H__
#define __JSUBSCRIPTIONS_H__

#include <eXosip2/eXosip.h>

struct jsubscription
{
  int sid;
  int did;

  char reason_phrase[50];
  int status_code;

  char textinfo[256];
  char req_uri[256];
  char local_uri[256];
  char remote_uri[256];

  int online_status;
  int ss_status;
  int ss_reason;

#define NOT_USED      0
  int state;

};

typedef struct jsubscription jsubscription_t;

extern jsubscription_t jsubscriptions[];

#ifndef MAX_NUMBER_OF_SUBSCRIPTIONS
#define MAX_NUMBER_OF_SUBSCRIPTIONS 100
#endif

jsubscription_t *jsubscription_find_subscription (int pos);
int jsubscription_get_number_of_pending_subscriptions (void);

int jsubscription_new (eXosip_event_t * je);
int jsubscription_answered (eXosip_event_t * je);
int jsubscription_proceeding (eXosip_event_t * je);
int jsubscription_redirected (eXosip_event_t * je);
int jsubscription_requestfailure (eXosip_event_t * je);
int jsubscription_serverfailure (eXosip_event_t * je);
int jsubscription_globalfailure (eXosip_event_t * je);
int jsubscription_notify (eXosip_event_t * je);

int jsubscription_closed (eXosip_event_t * je);

int jsubscription_remove (jsubscription_t * ca);

#endif
