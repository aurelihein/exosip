 /*
  The osipua library is a library based on oSIP that implements CallLeg and User Agent
  level.
  Copyright (C) 2001  Simon MORLAT simon.morlat@free.fr
  											Aymeric MOIZARD jack@atosc.org
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef BODYCONTEXT_H
#define BODYCONTEXT_H

#include <eXosip.h>

struct _BodyContext
{
	struct _BodyContextClass *klass;
	struct _BodyHandler *handler; 	/* the handler of the context  */
	eXosip_call_t       *call;      /* the related dialog */
	eXosip_dialog_t     *dialog;    /* the related call */
	void *data;
};

typedef struct _BodyContext BodyContext;


typedef void (*BodyContextDestroyFunc)(BodyContext*);
typedef int (*BodyContextNotifyMessageFunc)(BodyContext*,
					    osip_message_t *,
					    char *body);
typedef int (*BodyContextAddBodyFunc)(BodyContext*,osip_message_t *);

struct _BodyContextClass
{
	BodyContextDestroyFunc _destroy;
	BodyContextNotifyMessageFunc _notify_inc_request;
	BodyContextNotifyMessageFunc _notify_inc_response;
	BodyContextAddBodyFunc _gen_out_request;
	BodyContextAddBodyFunc _gen_out_response;
};

typedef struct _BodyContextClass BodyContextClass;

#define BODY_CONTEXT(b)  ((BodyContext*)(b))
#define BODY_CONTEXT_CLASS(klass) 	((BodyContextClass*)(klass))

void body_context_init(BodyContext *obj, BodyHandler *info);

#define body_context_class_init(k)

#define body_context_get_mime(context)	((context)->handler->klass->mime_type)

#define body_context_notify_inc_request(context, msg, body) \
			(context)->klass->_notify_inc_request((context),(msg), (body))

#define body_context_notify_inc_response(context, msg, body) \
			(context)->klass->_notify_inc_response((context),(msg),(body))

#define body_context_gen_out_request(context, msg) \
			(context)->klass->_gen_out_request((context),(msg))

#define body_context_gen_out_response(context, msg) \
			(context)->klass->_gen_out_response((context),(msg))


#define body_context_get_call(context)          ((context)->call)
#define body_context_get_dialog(context)        ((context)->dialog)

#define body_context_get_handler(context)	((context)->handler)


#endif




