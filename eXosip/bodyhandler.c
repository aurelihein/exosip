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
#include "bodyhandler.h"
#include "sdphandler.h"
#include "eXosip.h"

void body_handler_init(BodyHandler *info)
{
	
}

void body_handler_class_init(BodyHandlerClass *info)
{
	info->mime_type=NULL;
	info->_body_context_new=NULL;
	info->_init=NULL;
}

/**
 *body_handler_create_context:
 *@info: a body handler information.
 *
 *Creates a BodyContext from a BodyHandler. The BodyContext should use the information
 *given by the BodyHandler to initialize its context.
**/

BodyContext *body_handler_create_context(BodyHandler *info,
					 eXosip_call_t *call,
					 eXosip_dialog_t *dialog)
{
	BodyContext *ctx;
	ctx=info->klass->_body_context_new(info);
	ctx->call=call;
	ctx->dialog=dialog;
	return ctx;
}

void body_handler_init_with_ua(BodyHandler *info, eXosip_t *_eXosip)
{
	info->_eXosip=_eXosip;
	info->klass->_init(info);
}
