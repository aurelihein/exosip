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

#ifndef __JINSUBSCRIPTIONS_H__
#define __JINSUBSCRIPTIONS_H__

#include <eXosip/eXosip.h>

struct jinsubscription {
  int nid;
  int did;

  char reason_phrase[50];
  int  status_code;

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

typedef struct jinsubscription jinsubscription_t;

extern jinsubscription_t jinsubscriptions[];

#ifndef MAX_NUMBER_OF_INSUBSCRIPTIONS
#define MAX_NUMBER_OF_INSUBSCRIPTIONS 100
#endif

jinsubscription_t *jinsubscription_find_insubscription(int pos);
int jinsubscription_get_number_of_pending_insubscriptions();

int jinsubscription_new(eXosip_event_t *je);
int jinsubscription_answered(eXosip_event_t *je);
int jinsubscription_proceeding(eXosip_event_t *je);
int jinsubscription_redirected(eXosip_event_t *je);
int jinsubscription_requestfailure(eXosip_event_t *je);
int jinsubscription_serverfailure(eXosip_event_t *je);
int jinsubscription_globalfailure(eXosip_event_t *je);

int jinsubscription_closed(eXosip_event_t *je);

int jinsubscription_remove(jinsubscription_t *ca);

#endif
