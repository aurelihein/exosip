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


#ifndef SDPCONTEXT_H
#define SDPCONTEXT_H

#include <osip/sdp.h>
#include <osip/smsg.h>
#include <bodycontext.h>


typedef enum _SdpContextState
{
	SDP_CONTEXT_STATE_INIT,
	SDP_CONTEXT_STATE_NEGOCIATION_OPENED,
	SDP_CONTEXT_STATE_NEGOCIATION_CLOSED
} SdpContextState;

struct _SdpContext
{
	BodyContext parent;
	sdp_message_t *offer;		/* the local sdp to be used for outgoing request */
	sdp_message_t *answer;		/* the local sdp generated from an inc request */
	sdp_message_t *remote;  	
	int negoc_status;	/* in sip code */
	int incb;
	SdpContextState state;
};

struct _SdpContextClass
{
	BodyContextClass parent_class;
};

typedef struct  _SdpContext SdpContext;
typedef struct  _SdpContextClass SdpContextClass;

void sdp_context_init(SdpContext *obj, SdpHandler *info);
void sdp_context_class_init(SdpContextClass *klass);

BodyContext *sdp_context_new(SdpHandler *info);
void sdp_context_destroy(SdpContext *obj);

#define SDP_CONTEXT(obj)  ((SdpContext*)(obj))
#define SDP_CONTEXT_CLASS(klass)  ((SdpContextClass*)(klass))

/* get the local sdp after negociation */
#define sdp_context_get_answer(h)	((h)->localneg)
/* get the remote sdp after the response from remote server is received */
#define sdp_context_get_remote(h)	((h)->remote)

#define sdp_context_get_negociation_status(ctx)		((ctx)->negoc_status)

#endif
