/*
  * The osipua library is a library based on oSIP that implements CallLeg and User Agent
  * level.
  * Copyright (C) 2001  Simon MORLAT simon.morlat@free.fr
  * Aymeric MOIZARD jack@atosc.org
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  * 
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
#ifndef SDPHANDLER_H
#define SDPHANDLER_H

#include <bodyhandler.h>

struct _SdpHandler;
struct _SdpContext;
	
typedef struct _SdpPayload
{
	int line;	/* the index of the m= line */
	int pt;		/*payload type */
	int localport;
	int remoteport;
	int b_as_bandwidth;	/* application specific bandwidth */
	char *proto;
	char *c_nettype;
	char *c_addrtype;
	char *c_addr;
	char *c_addr_multicast_ttl;
	char *c_addr_multicast_int;
	char *a_rtpmap;
	char *a_fmtp;
} SdpPayload;

typedef int (*SdpHandlerReadCodecFunc) (struct _SdpHandler *, struct _SdpContext *,
											SdpPayload *);
typedef int (*SdpHandlerWriteCodecFunc) (struct _SdpHandler *, struct _SdpContext *);

struct _SdpHandler
{
	BodyHandler parent;
	SdpHandlerReadCodecFunc accept_audio_codecs;  /*from remote sdp */
	SdpHandlerReadCodecFunc accept_video_codecs;  /*from remote sdp */
	SdpHandlerWriteCodecFunc set_audio_codecs;    /*to local sdp */
	SdpHandlerWriteCodecFunc set_video_codecs;    /*to local sdp */
	SdpHandlerReadCodecFunc get_audio_codecs;     /*from incoming answer*/
	SdpHandlerReadCodecFunc get_video_codecs;     /*from incoming answer*/
};



typedef struct _SdpHandler SdpHandler;

struct _SdpHandlerClass
{
	BodyHandlerClass parent_class;
};

typedef struct _SdpHandlerClass SdpHandlerClass;

#include "sdpcontext.h"

void sdp_handler_init (SdpHandler * obj);
void sdp_handler_class_init (SdpHandlerClass * klass);

BodyHandler *sdp_handler_new ();
/* guess the sdp config from the config of the ua it is attached */
void sdp_handler_set_accept_offer_fcn(SdpHandler *sh,SdpHandlerReadCodecFunc audiofunc,
										SdpHandlerReadCodecFunc videofunc);
void sdp_handler_set_write_offer_fcn(SdpHandler *sh,SdpHandlerWriteCodecFunc audiofunc,
										SdpHandlerWriteCodecFunc videofunc);
void sdp_handler_set_read_answer_fcn(SdpHandler *sh,SdpHandlerReadCodecFunc audiofunc,
										SdpHandlerReadCodecFunc videofunc);
/* to add payloads to the offer, must be called inside the write_offer callback */
void sdp_handler_add_audio_payload(SdpHandler *sh,SdpContext *ctx,SdpPayload *payload);

#define SDP_HANDLER(obj)	((SdpHandler*)(obj))


/* INTERNAL USE*/
sdp_message_t * sdp_handler_generate_template(SdpHandler * obj);
sdp_message_t * sdp_handler_generate_offer(SdpHandler *sdph, struct _SdpContext *ctx);
sdp_message_t * sdp_handler_gererate_answer(SdpHandler *sdph, struct _SdpContext *ctx);
void sdp_handler_read_remote_answer(SdpHandler *sdph,struct _SdpContext *ctx);
#endif

