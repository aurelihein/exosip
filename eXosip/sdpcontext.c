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

#include "sdphandler.h"
#include <osipparser2/osip_message.h>
#include <osipparser2/sdp_message.h>
//#include "utils.h"

int sdp_context_gen_out_req(SdpContext *obj, osip_message_t *req)
{
	char *tmp;
	SdpHandler *sh;
	sdp_message_t *local;
	
	/* first, if it is not an invite, go away */
	if ( !MSG_IS_INVITE(req)) return -1;
	
	sdp_message_init(&local);
	sh=SDP_HANDLER(body_context_get_handler(BODY_CONTEXT(obj)));
	sdp_handler_generate_offer(sh,obj);
	if (obj->offer==NULL){
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"sdp_context_gen_out_req: unable to generate a sdp offer.\n"));
	  return 500;
	}
	sdp_message_to_str(obj->offer,&tmp);
	osip_message_set_body(req,tmp);
	osip_message_set_content_type(req,"application/sdp");
	obj->state=SDP_CONTEXT_STATE_NEGOCIATION_OPENED;
	return 0;
}	


int sdp_context_notify_inc_resp(SdpContext *obj, osip_message_t *resp, char *body)
{
	sdp_message_t *sdpmsg;
	int err;
	SdpHandler *sdph=SDP_HANDLER(body_context_get_handler(BODY_CONTEXT(obj)));
	
	sdp_message_init(&sdpmsg);
	err=sdp_message_parse(sdpmsg,body);
	if (err<0){
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"sdp_context_notify_inc_resp: could not parse sdp msg.\n"));
	  sdp_message_free(sdpmsg);
	  osip_free(sdpmsg);
	  return 500;
	}
	if (obj->remote!=NULL){
	  sdp_message_free(obj->remote);
	  osip_free(obj->remote);
	}
	obj->remote=sdpmsg;
	sdp_handler_read_remote_answer(sdph,obj);
	obj->state=SDP_CONTEXT_STATE_NEGOCIATION_CLOSED;
	return 0;
}

int sdp_context_gen_out_resp(SdpContext *obj, osip_message_t *resp)
{
	char *p;
	/* if the response is not a 200 OK for invite, exit */
	if (strcmp(osip_message_get_statuscode(resp),"200")!=0){
		return 0;
	}
	/* if in bad state exit too */
	if (obj->state!=SDP_CONTEXT_STATE_NEGOCIATION_OPENED) return 0;
	/* set the remote negociated sdp in the outgoing response */
	if (obj->answer==NULL){
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"sdp_context_gen_out_resp: there is no sdp answer!\n"));
	  return 500;
	}
	sdp_message_to_str(obj->answer,&p);
	osip_message_set_body(resp,p);
	osip_message_set_content_type(resp,"application/sdp");
	obj->state=SDP_CONTEXT_STATE_NEGOCIATION_CLOSED;
	return 0;
}

int sdp_context_notify_inc_req(SdpContext *obj, osip_message_t *resp,char *body)
{
	int err;
	sdp_message_t *sdpmsg;
	SdpHandler *sdph=SDP_HANDLER(body_context_get_handler(BODY_CONTEXT(obj)));
	sdp_message_init(&sdpmsg);
	err=sdp_message_parse(sdpmsg,body);
	if (err<0){
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"sdp_context_notify_inc_req: Could not parse incoming sdp message:%s\n",body));
	  sdp_message_free(sdpmsg);
	  osip_free(sdpmsg);
	  return 606;
	}
	if (obj->remote!=NULL){
	  sdp_message_free(obj->remote);
	  osip_free(obj->remote);
	}
	obj->remote=sdpmsg;
	obj->state=SDP_CONTEXT_STATE_NEGOCIATION_OPENED;
	sdp_handler_generate_answer(sdph,obj);
	OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			      "sdp_context_notify_inc_req: negociation returned: %i\n",obj->negoc_status));
	return obj->negoc_status;
}

static SdpContextClass *sdp_context_class=NULL;

void sdp_context_init(SdpContext *obj, SdpHandler *info)
{
	body_context_init(BODY_CONTEXT(obj), BODY_HANDLER(info));
	obj->offer=NULL;
	obj->answer=NULL;
	obj->remote=NULL;
	obj->state=SDP_CONTEXT_STATE_INIT;
}

void sdp_context_class_init(SdpContextClass *klass)
{
	BodyContextClass *bcc=BODY_CONTEXT_CLASS(klass);
	body_context_class_init(bcc);
	bcc->_notify_inc_request=(BodyContextNotifyMessageFunc)sdp_context_notify_inc_req;
	bcc->_notify_inc_response=(BodyContextNotifyMessageFunc)sdp_context_notify_inc_resp;;
	bcc->_gen_out_request=(BodyContextAddBodyFunc)sdp_context_gen_out_req;
	bcc->_gen_out_response=(BodyContextAddBodyFunc)sdp_context_gen_out_resp;
	bcc->_destroy=(BodyContextDestroyFunc)sdp_context_destroy;
}

BodyContext *sdp_context_new(SdpHandler *info)
{
	SdpContext *obj=osip_malloc(sizeof(SdpContext));
	if (sdp_context_class==NULL)
	{
		sdp_context_class=osip_malloc(sizeof(SdpContextClass));
		sdp_context_class_init(sdp_context_class);
	}
	BODY_CONTEXT(obj)->klass=BODY_CONTEXT_CLASS(sdp_context_class);
	sdp_context_init(obj,info);
	return BODY_CONTEXT(obj);
}

void sdp_context_uninit(SdpContext *obj)
{
	if (obj->offer!=NULL){
		sdp_message_free(obj->offer);
		osip_free(obj->offer);
	}
	if (obj->answer!=NULL){
		sdp_message_free(obj->answer);
		osip_free(obj->answer);
	}
	if (obj->remote!=NULL){
		sdp_message_free(obj->remote);
		osip_free(obj->remote);
	}
}

void sdp_context_destroy(SdpContext *obj)
{
	sdp_context_uninit(obj);
	osip_free(obj);
}




