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

#ifndef __EXOSIP_H__
#define __EXOSIP_H__

#include <stdio.h>

#include <osip2/osip.h>
#include <osip2/dialog.h>
#include <osip2/sdp_negoc.h>

#ifndef JD_EMPTY

#define JD_EMPTY          0
#define JD_INITIALIZED    1
#define JD_TRYING         2
#define JD_QUEUED         3
#define JD_RINGING        4
#define JD_ESTABLISHED    5
#define JD_REDIRECTED     6
#define JD_AUTH_REQUIRED  7
#define JD_CLIENTERROR    8
#define JD_SERVERERROR    9
#define JD_GLOBALFAILURE  10
#define JD_TERMINATED     11

#define JD_MAX            11

#endif

typedef struct eXosip_dialog_t eXosip_dialog_t;

struct eXosip_dialog_t {

  int              d_id;
  int              d_STATE;
  dialog_t        *d_dialog;      /* active dialog */

  int              d_timer;
  sip_t           *d_200Ok;
  sip_t           *d_ack;
  list_t          *media_lines;

  list_t          *d_inc_trs;
  list_t          *d_out_trs;

  eXosip_dialog_t *next;
  eXosip_dialog_t *parent;
};

/* presentation of dialogs ***********

   1
   State: TALKING
   Subject: You have a Call 
   <sip:jack@osip.org>
   at <sip:jack@anode.osip.org:5080>
   audio: 80.200.197.80 9300 RTP/AVP 0 speex/8000
   video: 80.200.197.80 9320 RTP/AVP 0 speex/8000
   
   2
   State: QUEUED
   Subject: Let's talk 
   <sip:jack@osip.org>
   at <sip:jack@anode.osip.org:5080>
   audio: 9300 RTP/AVP   0 PCMU/8000
   video: 9320 RTP/AVP   0 XXXqsxqs

   3
   State: ONHOLD
   Subject: Let's talk 
   <sip:jack@osip.org>
   at <sip:jack@anode.osip.org:5080>
   audio: 9300 RTP/AVP   0 PCMU/8000
   video: 9320 RTP/AVP   0 XXXqsxqs

*/

typedef struct eXosip_call_t eXosip_call_t;

struct eXosip_call_t {

  int              c_id;
  char             c_subject[100];
  eXosip_dialog_t *c_dialogs;
  transaction_t   *c_inc_tr;
  transaction_t   *c_out_tr;

  sdp_context_t   *c_ctx;

  eXosip_call_t   *next;
  eXosip_call_t   *parent;
};


typedef struct eXosip_realm_t eXosip_realm_t;

struct eXosip_realm_t {

  int             r_id;

  char           *r_realm;
  char           *r_username;
  char           *r_passwd;

  eXosip_realm_t *next;
  eXosip_realm_t *parent;
};

typedef struct eXosip_reg_t eXosip_reg_t;

struct eXosip_reg_t {

  int             r_id;

  int             r_reg_period;     /* delay between registration */
  char           *r_aor;            /* sip identity */
  char           *r_registrar;      /* registrar */
  eXosip_realm_t *r_realms;         /* list of realms */
  char           *r_contact;        /* list of contacts string */

  transaction_t  *r_last_tr;
  eXosip_reg_t   *next;
  eXosip_reg_t   *parent;
};

typedef struct jfreind_t jfreind_t;

struct jfreind_t {
  int            f_id;
  char          *f_nick;
  char          *f_home;
  char          *f_work;
  char          *f_email;
  char          *f_e164;

  jfreind_t     *next;
  jfreind_t     *parent;
};

typedef struct jidentity_t jidentity_t;

struct jidentity_t {
  int            i_id;
  char          *i_identity;
  char          *i_registrar;
  char          *i_realm;
  char          *i_userid;
  char          *i_pwd;

  jidentity_t   *next;
  jidentity_t   *parent;
};

typedef struct eXosip_t eXosip_t;

struct eXosip_t {

  FILE          *j_input;
  FILE          *j_output;
  eXosip_call_t *j_calls;
  list_t        *j_transactions;

  eXosip_reg_t  *j_reg;

  void          *j_mutexlock;
  osip_t        *j_osip;
  int            j_socket;
  int            j_stop_ua;
  void          *j_thread;

  /* configuration informations */
  jfreind_t     *j_freinds;
  jidentity_t   *j_identitys;
};

typedef struct jinfo_t jinfo_t;

struct jinfo_t {
  eXosip_dialog_t *jd;
  eXosip_call_t   *jc;
};


void freinds_add(char *nickname, char *home,
		 char *work, char *email, char *e164);

char *eXosip_guess_ip_for_via ();

int    eXosip_sdp_config_init();
sdp_t *eXosip_get_local_sdp_info(transaction_t *invite_tr);
int    eXosip_set_callbacks(osip_t *osip);
char  *to_tag_new_random();
unsigned int via_branch_new_random();
jinfo_t *new_jinfo(eXosip_call_t *jc, eXosip_dialog_t *jd);

int  eXosip_dialog_init_as_uac(eXosip_dialog_t **jd, sip_t *_200Ok);
int  eXosip_dialog_init_as_uas(eXosip_dialog_t **jd, sip_t *_invite, sip_t *_200Ok);
void eXosip_dialog_free(eXosip_dialog_t *jd);
void eXosip_dialog_set_state(eXosip_dialog_t *jd, int state);
void eXosip_delete_early_dialog(eXosip_dialog_t *jd);

int generating_message(sip_t **message, char *to, char *from,
		       char *route, char *buff);
int  generating_cancel(sip_t **dest, sip_t *request_cancelled);
int  generating_info(sip_t **info, dialog_t *dialog);
int  generating_bye(sip_t **bye, dialog_t *dialog);
int  generating_refer(sip_t **refer, dialog_t *dialog, char *refer_to);
int  generating_invite_on_hold(sip_t **invite, dialog_t *dialog,
				char *subject, char *sdp);
int  generating_invite_off_hold(sip_t **invite, dialog_t *dialog,
				char *subject, char *sdp);
int  generating_options(sip_t **options, char *from, char *to,
			char *sdp, char *proxy);

int  eXosip_reg_init(eXosip_reg_t **jr, char *from, char *proxy, char *contact);
int  generating_register(sip_t **reg, char *transport, char *from, char *proxy);

int eXosip_dialog_find(int jid, eXosip_call_t **jc, eXosip_dialog_t **jd);
int eXosip_call_find(int cid, eXosip_call_t **jc);
int eXosip_dialog_set_200ok(eXosip_dialog_t *_jd, sip_t *_200Ok);

void eXosip_answer_invite_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);
void eXosip_answer_invite_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);
void eXosip_answer_invite_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);

int _eXosip_build_response_default(sip_t **dest, dialog_t *dialog,
				  int status, sip_t *request);
int complete_answer_that_establish_a_dialog(sip_t *response, sip_t *request, char *contact);
int _eXosip_build_request_within_dialog(sip_t **dest, char *method_name,
				       dialog_t *dialog, char *transport);

transaction_t *eXosip_find_last_invite(eXosip_call_t *jc, eXosip_dialog_t *jd );
transaction_t *eXosip_find_last_inc_invite(eXosip_call_t *jc, eXosip_dialog_t *jd);
transaction_t *eXosip_find_last_out_invite(eXosip_call_t *jc, eXosip_dialog_t *jd);
transaction_t *eXosip_find_last_inc_bye(eXosip_call_t *jc, eXosip_dialog_t *jd);
transaction_t *eXosip_find_last_out_bye(eXosip_call_t *jc, eXosip_dialog_t *jd);
transaction_t *eXosip_find_last_inc_refer(eXosip_call_t *jc, eXosip_dialog_t *jd);
transaction_t *eXosip_find_last_out_refer(eXosip_call_t *jc, eXosip_dialog_t *jd);


int  eXosip_call_init(eXosip_call_t **jc);
void eXosip_call_free(eXosip_call_t *jc);
void eXosip_call_set_subject(eXosip_call_t *jc, char *subject);
int  eXosip_read_message(int max_message_nb, int sec_max, int usec_max);
void eXosip_free_terminated_calls ( void );



#define REMOVE_ELEMENT(first_element, element)   \
       if (element->parent==NULL)                \
	{ first_element = element->next;         \
          if (first_element!=NULL)               \
          first_element->parent = NULL; }        \
       else \
        { element->parent->next = element->next; \
          if (element->next!=NULL)               \
	element->next->parent = element->parent; \
	element->next = NULL;                    \
	element->parent = NULL; }

#define ADD_ELEMENT(first_element, element) \
   if (first_element==NULL)                 \
    {                                       \
      first_element   = element;            \
      element->next   = NULL;               \
      element->parent = NULL;               \
    }                                       \
  else                                      \
    {                                       \
      element->next   = first_element;      \
      element->parent = NULL;               \
      element->next->parent = element;      \
      first_element = element;              \
    }

#define APPEND_ELEMENT(type_of_element_t, first_element, element) \
  if (first_element==NULL)                            \
    { first_element = element;                        \
      element->next   = NULL; /* useless */           \
      element->parent = NULL; /* useless */ }         \
  else                                                \
    { type_of_element_t *f;                           \
      for (f=first_element; f->next!=NULL; f=f->next) \
         { }                                          \
      f->next    = element;                           \
      element->parent = f;                            \
      element->next   = NULL;                         \
    }

#endif
