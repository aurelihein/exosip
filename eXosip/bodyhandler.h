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

#ifndef BODYHANDLERINFO_H
#define BODYHANDLERINFO_H

#include "eXosip.h"

struct _BodyHandler
{
	struct _BodyHandlerClass *klass;
	eXosip_t *_eXosip;
};

typedef struct _BodyHandler BodyHandler;

typedef struct _BodyContext * (*BodyContextNewFunc)(struct _BodyHandler *);
typedef void (*BodyHandlerFunc)(BodyHandler *);
struct _BodyHandlerClass
{
	char *mime_type;
	BodyContextNewFunc _body_context_new;  /* the constructor of the bodyhandler managed by the BodyHandler*/	
	BodyHandlerFunc _init;	/* called when the body handler info is placed on the ua list of infos.*/
};
typedef struct _BodyHandlerClass BodyHandlerClass;

#define BODY_HANDLER(obj)		((BodyHandler*)(obj))
#define BODY_HANDLER_CLASS(obj)		((BodyHandlerClass*)(obj))

void body_handler_init(BodyHandler *info);
void body_handler_class_init(BodyHandlerClass *info);
/*
#define body_handler_ref(info)		(info)->ref_count++;
#define body_handler_unref(info)		(info)->ref_count--;
 */
//typedef struct _OsipUA OsipUA;
//typedef struct _OsipCallLeg OsipCallLeg;

struct _BodyContext *body_handler_create_context(BodyHandler *info,
						 eXosip_call_t *call,
						 eXosip_dialog_t *dialog);

void body_handler_init_with_ua(BodyHandler *info, eXosip_t *_eXosip);	

#define body_handler_get_mime(info)	((info)->klass->mime_type)

#define body_handler_get_ua(bh)		((bh)->ua)

#endif










