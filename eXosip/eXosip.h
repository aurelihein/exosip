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

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#define snprintf _snprintf
#define close(s) closesocket(s)
#endif

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
  osip_dialog_t   *d_dialog;      /* active dialog */

  int              d_timer;
  osip_message_t  *d_200Ok;
  osip_message_t  *d_ack;
  osip_list_t     *media_lines;

  osip_list_t     *d_inc_trs;
  osip_list_t     *d_out_trs;

  eXosip_dialog_t *next;
  eXosip_dialog_t *parent;
};

typedef enum eXosip_ss {
  EXOSIP_SUBCRSTATE_UNKNOWN,
  EXOSIP_SUBCRSTATE_PENDING,
  EXOSIP_SUBCRSTATE_ACTIVE,
  EXOSIP_SUBCRSTATE_TERMINATED
} eXosip_ss_t;

/* should be added when the sub-state="terminated" */
typedef enum eXosip_ss_reason {
  DEACTIVATED,
  PROBATION,
  REJECTED,
  TIMEOUT,
  GIVEUP,
  NORESSOURCE
} eXosip_ss_reason_t;


typedef enum eXosip_ss_status {
  EXOSIP_NOTIFY_UNKNOWN,
  EXOSIP_NOTIFY_PENDING, /* subscription not yet accepted */
  EXOSIP_NOTIFY_ONLINE,
  EXOSIP_NOTIFY_AWAY
} eXosip_ss_status_t;

typedef struct eXosip_subscribe_t eXosip_subscribe_t;

struct eXosip_subscribe_t {

  int                 s_id;
  char                s_uri[255];
  int                 s_online_status;
  int                 s_ss_status;
  int                 s_ss_reason;
  int                 s_ss_expires;
  eXosip_dialog_t    *s_dialogs;

  osip_transaction_t *s_inc_tr;
  osip_transaction_t *s_out_tr;

  eXosip_subscribe_t *next;
  eXosip_subscribe_t *parent;
};

typedef struct eXosip_notify_t eXosip_notify_t;

struct eXosip_notify_t {

  int                 n_id;
  int                 n_online_status;
  int                 n_ss_status;
  int                 n_ss_reason;
  int                 n_ss_expires;
  eXosip_dialog_t    *n_dialogs;

  osip_transaction_t *n_inc_tr;
  osip_transaction_t *n_out_tr;

  eXosip_notify_t    *next;
  eXosip_notify_t    *parent;
};

typedef struct eXosip_call_t eXosip_call_t;

struct eXosip_call_t {

  int                      c_id;
  char                     c_subject[100];
  eXosip_dialog_t         *c_dialogs;
  osip_transaction_t      *c_inc_tr;
  osip_transaction_t      *c_out_tr;

  sdp_negotiation_ctx_t   *c_ctx;

  eXosip_call_t           *next;
  eXosip_call_t           *parent;
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

  osip_transaction_t  *r_last_tr;
  eXosip_reg_t   *next;
  eXosip_reg_t   *parent;
};

typedef struct jfriend_t jfriend_t;

struct jfriend_t {
  int            f_id;
  char          *f_nick;
  char          *f_home;
  char          *f_work;
  char          *f_email;
  char          *f_e164;

  jfriend_t     *next;
  jfriend_t     *parent;
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

typedef struct jsubscriber_t jsubscriber_t;

struct jsubscriber_t {
  int            s_id;
  char          *s_nick;
  char          *s_uri;
  char          *s_allow;

  jsubscriber_t   *next;
  jsubscriber_t   *parent;
};

typedef struct eXosip_t eXosip_t;

struct eXosip_t {

  FILE               *j_input;
  FILE               *j_output;
  eXosip_call_t      *j_calls;        /* my calls        */
  eXosip_subscribe_t *j_subscribes;   /* my friends      */
  eXosip_notify_t    *j_notifies;     /* my susbscribers */
  osip_list_t        *j_transactions;

  eXosip_reg_t       *j_reg;

  void               *j_mutexlock;
  osip_t             *j_osip;
  int                 j_socket;
  int                 j_stop_ua;
  void               *j_thread;

  jsubscriber_t      *j_subscribers;

  /* configuration informations */
  jfriend_t          *j_friends;
  jidentity_t        *j_identitys;
};

typedef struct jinfo_t jinfo_t;

struct jinfo_t {
  eXosip_dialog_t     *jd;
  eXosip_call_t       *jc;
  eXosip_subscribe_t  *js;
  eXosip_notify_t     *jn;
};



char *eXosip_guess_ip_for_via ();

int    eXosip_sdp_negotiation_init();
sdp_message_t *eXosip_get_local_sdp_info(osip_transaction_t *invite_tr);
int    eXosip_set_callbacks(osip_t *osip);
char  *osip_to_tag_new_random();
unsigned int via_branch_new_random();
void __eXosip_delete_jinfo(osip_transaction_t *transaction);
jinfo_t *__eXosip_new_jinfo(eXosip_call_t *jc, eXosip_dialog_t *jd,
			    eXosip_subscribe_t *js, eXosip_notify_t *jn);

int  eXosip_dialog_init_as_uac(eXosip_dialog_t **jd, osip_message_t *_200Ok);
int  eXosip_dialog_init_as_uas(eXosip_dialog_t **jd, osip_message_t *_invite, osip_message_t *_200Ok);
void eXosip_dialog_free(eXosip_dialog_t *jd);
void eXosip_dialog_set_state(eXosip_dialog_t *jd, int state);
void eXosip_delete_early_dialog(eXosip_dialog_t *jd);

int generating_initial_subscribe(osip_message_t **message, char *to,
				 char *from, char *route);
int generating_message(osip_message_t **message, char *to, char *from,
		       char *route, char *buff);
int  generating_cancel(osip_message_t **dest, osip_message_t *request_cancelled);
int  generating_info(osip_message_t **info, osip_dialog_t *dialog);
int  generating_bye(osip_message_t **bye, osip_dialog_t *dialog);
int  generating_refer(osip_message_t **refer, osip_dialog_t *dialog, char *refer_to);
int  generating_invite_on_hold(osip_message_t **invite, osip_dialog_t *dialog,
				char *subject, char *sdp);
int  generating_invite_off_hold(osip_message_t **invite, osip_dialog_t *dialog,
				char *subject, char *sdp);
int  generating_options(osip_message_t **options, char *from, char *to,
			char *sdp, char *proxy);

int  eXosip_reg_init(eXosip_reg_t **jr, char *from, char *proxy, char *contact);
void eXosip_reg_free(eXosip_reg_t *jreg);
int  generating_register(osip_message_t **reg, char *transport, char *from, char *proxy);

int eXosip_call_dialog_find(int jid, eXosip_call_t **jc, eXosip_dialog_t **jd);
int eXosip_notify_dialog_find(int nid, eXosip_notify_t **jn, eXosip_dialog_t **jd);
int eXosip_subscribe_dialog_find(int nid, eXosip_subscribe_t **js, eXosip_dialog_t **jd);
int eXosip_call_find(int cid, eXosip_call_t **jc);
int eXosip_dialog_set_200ok(eXosip_dialog_t *_jd, osip_message_t *_200Ok);

void eXosip_answer_invite_1xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);
void eXosip_answer_invite_2xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);
void eXosip_answer_invite_3456xx(eXosip_call_t *jc, eXosip_dialog_t *jd, int code);
void eXosip_notify_answer_subscribe_1xx(eXosip_notify_t *jc,
					eXosip_dialog_t *jd, int code);
void eXosip_notify_answer_subscribe_2xx(eXosip_notify_t *jn,
					eXosip_dialog_t *jd, int code);
void eXosip_notify_answer_subscribe_3456xx(eXosip_notify_t *jn,
					   eXosip_dialog_t *jd, int code);

int _eXosip_build_response_default(osip_message_t **dest, osip_dialog_t *dialog,
				  int status, osip_message_t *request);
int complete_answer_that_establish_a_dialog(osip_message_t *response, osip_message_t *request, char *contact);
int _eXosip_build_request_within_dialog(osip_message_t **dest, char *method_name,
				       osip_dialog_t *dialog, char *transport);

osip_transaction_t *eXosip_find_last_inc_notify(eXosip_subscribe_t *jn, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_out_notify(eXosip_notify_t *jn, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_inc_subscribe(eXosip_notify_t *jn, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_invite(eXosip_call_t *jc, eXosip_dialog_t *jd );
osip_transaction_t *eXosip_find_last_inc_invite(eXosip_call_t *jc, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_out_invite(eXosip_call_t *jc, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_inc_bye(eXosip_call_t *jc, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_out_bye(eXosip_call_t *jc, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_inc_refer(eXosip_call_t *jc, eXosip_dialog_t *jd);
osip_transaction_t *eXosip_find_last_out_refer(eXosip_call_t *jc, eXosip_dialog_t *jd);


int  eXosip_call_init(eXosip_call_t **jc);
void eXosip_call_free(eXosip_call_t *jc);
void eXosip_call_set_subject(eXosip_call_t *jc, char *subject);
int  eXosip_read_message(int max_message_nb, int sec_max, int usec_max);
void eXosip_release_terminated_calls ( void );


int  eXosip_subscribe_init(eXosip_subscribe_t **js, char *uri);
void eXosip_subscribe_free(eXosip_subscribe_t *js);
int  _eXosip_subscribe_set_refresh_interval(eXosip_subscribe_t *js, osip_message_t *inc_subscribe);
int  eXosip_subscribe_need_refresh(eXosip_subscribe_t *js, int now);
void eXosip_subscribe_send_subscribe(eXosip_subscribe_t *js,
				     eXosip_dialog_t *jd, const char *expires);

int  eXosip_notify_init(eXosip_notify_t **jn);
void eXosip_notify_free(eXosip_notify_t *jn);
int  _eXosip_notify_set_refresh_interval(eXosip_notify_t *jn,
					 osip_message_t *inc_subscribe);
void _eXosip_notify_add_expires_in_2XX_for_subscribe(eXosip_notify_t *jn,
						     osip_message_t *answer);
int  _eXosip_notify_add_body(eXosip_notify_t *jn, osip_message_t *notify);
int  eXosip_notify_add_allowed_subscriber(char *sip_url);
int  _eXosip_notify_is_a_known_subscriber(osip_message_t *sip);
void eXosip_notify_send_notify(eXosip_notify_t *jn, eXosip_dialog_t *jd,
			       int subsciption_status,
			       int online_status);


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
