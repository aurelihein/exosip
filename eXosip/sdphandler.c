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

#include "sdphandler.h"

#ifdef INET6
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#endif

extern char    *localip;

int
sdp_payload_init (SdpPayload * payload)
{
	memset (payload, 0, sizeof (SdpPayload));
	return 0;
}

void
sdp_payload_uninit (SdpPayload * payload)
{

}

static SdpHandlerClass *sdp_handler_class = NULL;

void
sdp_handler_init (SdpHandler * config)
{
	memset (config, 0, sizeof (SdpHandler));
	body_handler_init (BODY_HANDLER (config));
}

static void
stub ()
{
}

void
sdp_handler_class_init (SdpHandlerClass * klass)
{
	BodyHandlerClass *bklass = BODY_HANDLER_CLASS (klass);
	body_handler_class_init (bklass);
	bklass->mime_type = "application/sdp";
	bklass->_body_context_new = (BodyContextNewFunc) sdp_context_new;
	bklass->_init = (BodyHandlerFunc) stub;

}

BodyHandler *
sdp_handler_new ()
{
	SdpHandler *obj;
	if (sdp_handler_class == NULL)
	{
		sdp_handler_class = osip_malloc (sizeof (SdpHandlerClass));
		sdp_handler_class_init (sdp_handler_class);
	}
	obj = osip_malloc (sizeof (SdpHandler));
	sdp_handler_init (obj);
	BODY_HANDLER (obj)->klass = BODY_HANDLER_CLASS (sdp_handler_class);
	return BODY_HANDLER (obj);
}


void
sdp_handler_uninit (SdpHandler * config)
{

}

void
sdp_handler_destroy (SdpHandler * obj)
{
	sdp_handler_uninit (obj);
	osip_free (obj);
}

void
sdp_handler_set_accept_offer_fcn (SdpHandler * sh,
				  SdpHandlerReadCodecFunc audiofunc,
				  SdpHandlerReadCodecFunc videofunc)
{
	sh->accept_audio_codecs = audiofunc;
	sh->accept_video_codecs = videofunc;
}

void
sdp_handler_set_write_offer_fcn (SdpHandler * sh,
				 SdpHandlerWriteCodecFunc audiofunc,
				 SdpHandlerWriteCodecFunc videofunc)
{
	sh->set_audio_codecs = audiofunc;
	sh->set_video_codecs = videofunc;
}

void
sdp_handler_set_read_answer_fcn (SdpHandler * sh,
				 SdpHandlerReadCodecFunc audiofunc,
				 SdpHandlerReadCodecFunc videofunc)
{
	sh->get_audio_codecs = audiofunc;
	sh->get_video_codecs = videofunc;
}


/* guess the sdp config from the ua config, and generate a template sdp */
sdp_message_t *
sdp_handler_generate_template (SdpHandler * obj)
{
	eXosip_t *_eXosip = BODY_HANDLER (obj)->_eXosip;
	sdp_message_t *local;

	if (_eXosip == NULL)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"SdpHandler: _eXosip is NULL"));
	  return NULL;
	}
	sdp_message_init (&local);
#ifdef INET6
	switch (_eXosip->ua_family)
	{
	case AF_INET:
		sdp_message_v_version_set (local, osip_strdup ("0"));
		sdp_message_o_origin_set (local, osip_strdup ("somebody")
					  /* url_getusername (url) */ ,
				  osip_strdup ("123456"), osip_strdup ("654321"),
				  osip_strdup ("IN"), osip_strdup ("IP4"),
				  osip_strdup (localip));
		sdp_message_s_name_set (local, osip_strdup ("A conversation"));
		sdp_message_c_connection_add (local, -1,
				      osip_strdup ("IN"), osip_strdup ("IP4"),
				      osip_strdup (localip), NULL, NULL);
		sdp_message_t_time_descr_add (local, osip_strdup ("0"), osip_strdup ("0"));
		break;
	case AF_INET6:
		sdp_message_v_version_set (local, osip_strdup ("0"));
		sdp_message_o_origin_set (local, osip_strdup ("somebody")
					  /* url_getusername (url) */ ,
				  osip_strdup ("123456"), osip_strdup ("654321"),
				  osip_strdup ("IN"), osip_strdup ("IP6"),
				  osip_strdup (localip));
		sdp_message_s_name_set (local, osip_strdup ("A conversation"));
		sdp_message_c_connection_add (local, -1,
				      osip_strdup ("IN"), osip_strdup ("IP6"),
				      osip_strdup (localip), NULL, NULL);
		sdp_message_t_time_descr_add (local, osip_strdup ("0"), osip_strdup ("0"));
		break;
	default:
		break;
	}
#else
	sdp_message_v_version_set (local, osip_strdup ("0"));
	sdp_message_o_origin_set (local, osip_strdup ("somebody")
					  /* url_getusername (url) */ ,
			  osip_strdup ("123456"), osip_strdup ("654321"),
			  osip_strdup ("IN"), osip_strdup ("IP4"),
			  osip_strdup (localip));
	sdp_message_s_name_set (local, osip_strdup ("A conversation"));
	sdp_message_c_connection_add (local, -1,
			      osip_strdup ("IN"), osip_strdup ("IP4"),
			      osip_strdup (localip), NULL, NULL);
	sdp_message_t_time_descr_add (local, osip_strdup ("0"), osip_strdup ("0"));
#endif
	return local;
}

/* to add payloads to the offer, must be called inside the write_offer callback */
void
sdp_handler_add_payload (SdpHandler * sh, SdpContext * ctx,
			 SdpPayload * payload, char *media)
{
	sdp_message_t *offer = ctx->offer;
	char *attr_field;
	char tmp[256];
	if (!ctx->incb)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"You must not call sdp_handler_add_*_payload outside the write_offer callback"));
	  abort ();
	}
	if (payload->proto == NULL)
		payload->proto = "RTP/AVP";
	if (sdp_message_m_media_get (offer, payload->line) == NULL)
	{
		/* need a new line */
	  sprintf(tmp, "%i", payload->localport);
	  sdp_message_m_media_add (offer, osip_strdup (media),
				 osip_strdup (tmp), NULL,
				 osip_strdup (payload->proto));
	}
	sprintf(tmp, "%i", payload->pt);
	sdp_message_m_payload_add (offer, payload->line, osip_strdup(tmp));
	if (payload->a_rtpmap != NULL)
	{
	  sprintf(tmp, "%i %s", payload->pt, payload->a_rtpmap);
	  attr_field = osip_strdup(tmp);
		sdp_message_a_attribute_add (offer, payload->line,
				     osip_strdup ("rtpmap"), attr_field);
	}
	if (payload->a_fmtp != NULL)
	{
	  sprintf(tmp, "%i %s", payload->pt, payload->a_fmtp);
	  attr_field = osip_strdup(tmp);
		sdp_message_a_attribute_add (offer, payload->line, osip_strdup ("fmtp"),
				     attr_field);
	}
	if (payload->b_as_bandwidth != 0)
	{
	  sprintf(tmp, "%i %i", payload->pt, payload->b_as_bandwidth);
	  attr_field = osip_strdup(tmp);
		sdp_message_b_bandwidth_add (offer, payload->line, osip_strdup ("AS"),
				     attr_field);
	}
}

void
sdp_handler_add_audio_payload (SdpHandler * sh, SdpContext * ctx,
			       SdpPayload * payload)
{
	sdp_handler_add_payload (sh, ctx, payload, "audio");
}

void
sdp_handler_add_video_payload (SdpHandler * sh, SdpContext * ctx,
			       SdpPayload * payload)
{
	sdp_handler_add_payload (sh, ctx, payload, "video");
}

sdp_message_t *
sdp_handler_generate_offer (SdpHandler * sdph, SdpContext * ctx)
{
	sdp_message_t *offer;

	offer = sdp_handler_generate_template (sdph);
	/* add audio codecs */
	ctx->offer = offer;
	ctx->incb = 1;
	if (sdph->set_audio_codecs != NULL)
		sdph->set_audio_codecs (sdph, ctx);
	if (sdph->set_video_codecs != NULL)
		sdph->set_video_codecs (sdph, ctx);
	ctx->incb = 0;
	return offer;
}

/* return the value of attr "field" for payload pt at line pos (field=rtpmap,fmtp...)*/
char *sdp_a_attr_value_get_with_pt(sdp_message_t *sdp,int pos,
				   int pt,char *field)
{
	int i,tmppt=0,scanned=0;
	char *tmp;
	sdp_attribute_t *attr;
	for (i=0;(attr=sdp_message_attribute_get(sdp,pos,i))!=NULL;i++){
		if (osip_strcasecmp(field,attr->a_att_field)==0){
			sscanf(attr->a_att_value,"%i %n",&tmppt,&scanned);
			if (pt==tmppt){
				tmp=attr->a_att_value+scanned;
				if (strlen(tmp)>0)
					return tmp;
				else {
				  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_WARNING,NULL,
							"sdp has a strange a= line."));
				  continue;
				}
			}
		}
	}
	return NULL;
}

int sdp_b_bandwidth_get_with_pt(sdp_message_t *sdp,int pos,int pt)
{
	int i,tmppt=0,bwvalue=0;
	sdp_bandwidth_t *bw;
	
	for (i=0;(bw=sdp_message_bandwidth_get(sdp,pos,i))!=NULL;i++){
		if (osip_strcasecmp("AS",bw->b_bwtype)==0){
			sscanf(bw->b_bandwidth,"%i %i",&tmppt,&bwvalue);
			if (pt==tmppt){
				return bwvalue;
			}
		}
	}
	return 0;
}

sdp_message_t *
sdp_handler_generate_answer (SdpHandler * sdph, struct _SdpContext * ctx)
{
	sdp_message_t *remote = ctx->remote;
	sdp_message_t *answer;
	char *mtype, tmp[256];
	char *proto, *port, *pt;
	int i, j, ncodec, m_lines_accepted = 0;
	int err;
	SdpPayload payload;
#ifdef INET6
	struct ifaddrs *ifa0, *ifa;
#endif

	answer = sdp_handler_generate_template (sdph);

#ifdef INET6
#ifdef HAVE_GETIFADDRS
	/* XXX : I must rewrite. */
	if (strncmp (remote->o_addrtype, answer->o_addrtype, 3) != 0)
	{
		char namebuf[BUFSIZ];
		OsipUA *ua = BODY_HANDLER (sdph)->ua;

		if (ua == NULL)
		{
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
					"SdpHandler: ua is NULL"));
		  return;
		}

		if (getifaddrs (&ifa0))
			return;

		for (ifa = ifa0; ifa; ifa = ifa->ifa_next)
		{
			if (!ifa->ifa_addr)
				continue;

#if 0
			/* XXX: Is correct flag IFF_* ? */
			if (ifp->ifa_flags & IFF_LOOPBACK)
				continue;
#endif

			if (ifa->ifa_addr->sa_family == AF_INET
			    && (strncmp (remote->o_addrtype, "IP4", 3) == 0))
			{
				getnameinfo (ifa->ifa_addr,
					     sizeof (struct sockaddr_in),
					     namebuf, sizeof (namebuf), NULL,
					     0, NI_NUMERICHOST);


				/* XXX: umm... */
				if (strcmp (namebuf, "127.0.0.1") == 0)
					continue;

				_eXosip->ua_family = AF_INET;
				strncpy (localip, namebuf, IP_SIZE);
				sdp_message_v_version_set (answer, osip_strdup ("0"));
				sdp_message_o_origin_set (answer,
							  osip_strdup ("somebody")
							  /* url_getusername(url) */ ,
						  osip_strdup ("123456"),
						  osip_strdup ("654321"),
						  osip_strdup ("IN"),
						  osip_strdup ("IP4"),
						  osip_strdup (localip));
				sdp_message_s_name_set (answer,
						osip_strdup ("A conversation"));
				sdp_message_c_connection_add (answer, -1,
						      osip_strdup ("IN"),
						      osip_strdup ("IP4"),
						      osip_strdup (localip),
						      NULL, NULL);
				sdp_message_t_time_descr_add (answer, osip_strdup ("0"),
						      osip_strdup ("0"));
				break;
			}
			else if (ifa->ifa_addr->sa_family == AF_INET6
				 && (strncmp (remote->o_addrtype, "IP6", 3) ==
				     0))
			{
				if (IN6_IS_ADDR_LINKLOCAL
				    (&
				     ((struct sockaddr_in6 *) ifa->ifa_addr)->
				     sin6_addr))
					continue;

				/* XXX: umm... */
				if (strcmp (namebuf, "::1") == 0)
					continue;


				getnameinfo (ifa->ifa_addr,
					     sizeof (struct sockaddr_in6),
					     namebuf, sizeof (namebuf), NULL,
					     0, NI_NUMERICHOST);

				_eXosip->ua_family = AF_INET6;
				strncpy (localip, namebuf, IP_SIZE);
				sdp_message_v_version_set (answer, osip_strdup ("0"));
				sdp_message_o_origin_set (answer,
							  osip_strdup ("somebody")
							  /* url_getusername(url) */ ,
						  osip_strdup ("123456"),
						  osip_strdup ("654321"),
						  osip_strdup ("IN"),
						  osip_strdup ("IP6"),
						  osip_strdup (localip));
				sdp_message_s_name_set (answer,
						osip_strdup ("A conversation"));
				sdp_message_c_connection_add (answer, -1,
						      osip_strdup ("IN"),
						      osip_strdup ("IP6"),
						      osip_strdup (localip),
						      NULL, NULL);
				sdp_message_t_time_descr_add (answer, osip_strdup ("0"),
						      osip_strdup ("0"));
				break;
			}	/* else {} */

		}

	}
#endif
#endif

	/* for each m= line */
	for (i = 0; !sdp_message_endof_media (remote, i); i++)
	{
		memset (&payload, 0, sizeof (SdpPayload));
		mtype = sdp_message_m_media_get (remote, i);
		proto = sdp_message_m_proto_get (remote, i);
		port = sdp_message_m_port_get (remote, i);
		payload.remoteport = osip_atoi (port);
		payload.proto = proto;
		payload.line = i;
		payload.c_addr = sdp_message_c_addr_get (remote, i, 0);
		if (payload.c_addr == NULL)
			payload.c_addr = sdp_message_c_addr_get (remote, -1, 0);
		if (osip_strcasecmp ("audio", mtype) == 0)
		{
			if (sdph->accept_audio_codecs != NULL)
			{
				ncodec = 0;
				/* for each payload type */
				for (j = 0;
				     ((pt =
				       sdp_message_m_payload_get (remote, i,
							  j)) != NULL); j++)
				{
					payload.pt = osip_atoi (pt);
					/* get the rtpmap associated to this codec, if any */
					payload.a_rtpmap =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "rtpmap");
					/* get the fmtp, if any */
					payload.a_fmtp =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "fmtp");
					/* get application specific bandwidth, if any */
					payload.b_as_bandwidth =
						sdp_b_bandwidth_get_with_pt
						(remote, i, payload.pt);
					/* ask the application if this codec is supported */
					err = sdph->accept_audio_codecs (sdph,
									 ctx,
									 &payload);
					if (err == 0 && payload.localport > 0)
					{
						ncodec++;
						/* codec accepted */
						if (ncodec == 1)
						{
							/* first codec accepted, setup the line  */
						  sprintf(tmp, "%i", payload.localport);
						  sdp_message_m_media_add
						    (answer,
						     osip_strdup
						     (mtype),
						     osip_strdup(tmp),
						     NULL,
						     osip_strdup(proto));
						}
						/* add the payload, rtpmap, fmtp */
						sprintf(tmp, "%i", payload.pt);
						sdp_message_m_payload_add (answer, i,
									   tmp);
						if (payload.a_rtpmap != NULL)
						{
						  sprintf(tmp, "%i %s",
							  payload.pt,
							  payload.a_rtpmap);
						    sdp_message_a_attribute_add
						    (answer, i,
						     osip_strdup
						     ("rtpmap"),
						     osip_strdup(tmp));
						}
						if (payload.a_fmtp != NULL)
						{
						  sprintf(tmp, "%i %s",
							  payload.pt,
							  payload.a_fmtp);
						    sdp_message_a_attribute_add
						    (answer, i,
						     osip_strdup
						     ("fmtp"),
						     osip_strdup(tmp));
						}
						if (payload.b_as_bandwidth !=
						    0)
						{
						  sprintf(tmp, "%i %i",
							  payload.pt,
							  payload.b_as_bandwidth);
						    sdp_message_b_bandwidth_add
						    (answer, i,
						     osip_strdup
						     ("AS"),
						     osip_strdup(tmp));
						}
					}
				}
				if (ncodec == 0)
				{
				    /* refuse the line */
				    sdp_message_m_media_add (answer,
							     osip_strdup (mtype),
							     osip_strdup("0"),
							     NULL,
							     osip_strdup (proto));
				}
				else
					m_lines_accepted++;
			}
			else
			{
				/* refuse this line (leave port to 0) */
				sdp_message_m_media_add (answer,
							 osip_strdup (mtype),
							 osip_strdup ("0"),
							 NULL,
							 osip_strdup (proto));
			}

		}
		else if (osip_strcasecmp ("video", mtype) == 0)
		{
			if (sdph->accept_video_codecs != NULL)
			{
				ncodec = 0;
				/* for each payload type */
				for (j = 0;
				     ((pt =
				       sdp_message_m_payload_get (remote, i,
							  j)) != NULL); j++)
				{
					payload.pt = osip_atoi (pt);
					/* get the rtpmap associated to this codec, if any */
					payload.a_rtpmap =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "rtpmap");
					/* get the fmtp, if any */
					payload.a_fmtp =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "fmtp");
					/* ask the application if this codec is supported */
					err = sdph->accept_video_codecs (sdph,
									 ctx,
									 &payload);
					if (err == 0 && payload.localport > 0)
					{
						ncodec++;
						/* codec accepted */
						if (ncodec == 1)
						{
							/* first codec accepted, setup the line  */
							sdp_message_m_media_add
								(answer,
								 osip_strdup
								 (mtype),
								 osip_strdup
								 ("0"), NULL,
								 osip_strdup
								 (proto));
						}
						/* add the payload, rtpmap, fmtp */
						sprintf(tmp, "%i", payload.pt);
						sdp_message_m_payload_add (answer, i,
									   osip_strdup(tmp));
						if (payload.a_rtpmap != NULL)
						{
						  sprintf(tmp, "%i %s",
							  payload.pt,
							  payload.a_rtpmap);
							sdp_message_a_attribute_add
								(answer, i,
								 osip_strdup
								 ("rtpmap"),
								 osip_strdup
								 (tmp));
						}
						if (payload.a_fmtp != NULL)
						  {
						    sprintf(tmp, "%i %s",
							    payload.pt,
							    payload.a_fmtp);
						    sdp_message_a_attribute_add
						      (answer, i,
						       osip_strdup("fmtp"),
						       osip_strdup(tmp));
						  }
					}
				}
				if (ncodec == 0)
				{
					/* refuse the line */
					sdp_message_m_media_add (answer,
							 osip_strdup (mtype),
							 osip_strdup("0"),
								 NULL,
							 osip_strdup (proto));
				}
				else
					m_lines_accepted++;
			}
			else
			{
				/* refuse this line (leave port to 0) */
				sdp_message_m_media_add (answer, osip_strdup (mtype),
						 osip_strdup("0"), NULL,
						 osip_strdup (proto));
			}
		}
	}
	ctx->answer = answer;
	if (m_lines_accepted > 0)
		ctx->negoc_status = 200;
	else
		ctx->negoc_status = 415;
	return answer;
}

void
sdp_handler_read_remote_answer (SdpHandler * sdph, struct _SdpContext *ctx)
{
	sdp_message_t *remote = ctx->remote;
	char *mtype;
	char *proto, *port, *pt;
	int i, j;
	int err;
	SdpPayload payload;


	/* for each m= line */
	for (i = 0; !sdp_message_endof_media (remote, i); i++)
	{
		memset (&payload, 0, sizeof (SdpPayload));
		mtype = sdp_message_m_media_get (remote, i);
		proto = sdp_message_m_proto_get (remote, i);
		port = sdp_message_m_port_get (remote, i);
		payload.remoteport = osip_atoi (port);
		payload.localport = osip_atoi (sdp_message_m_port_get (ctx->offer, i));
		payload.proto = proto;
		payload.line = i;
		payload.c_addr = sdp_message_c_addr_get (remote, i, 0);
		if (payload.c_addr == NULL)
			payload.c_addr = sdp_message_c_addr_get (remote, -1, 0);
		if (osip_strcasecmp ("audio", mtype) == 0)
		{
			if (sdph->get_audio_codecs != NULL)
			{
				/* for each payload type */
				for (j = 0;
				     ((pt =
				       sdp_message_m_payload_get (remote, i,
							  j)) != NULL); j++)
				{
					payload.pt = osip_atoi (pt);
					/* get the rtpmap associated to this codec, if any */
					payload.a_rtpmap =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "rtpmap");
					/* get the fmtp, if any */
					payload.a_fmtp =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "fmtp");
					/* ask the application if this codec is supported */
					err = sdph->get_audio_codecs (sdph,
								      ctx,
								      &payload);
				}
			}
		}
		else if (osip_strcasecmp ("video", mtype) == 0)
		{
			if (sdph->accept_video_codecs != NULL)
			{
				/* for each payload type */
				for (j = 0;
				     ((pt =
				       sdp_message_m_payload_get (remote, i,
							  j)) != NULL); j++)
				{
					payload.pt = osip_atoi (pt);
					/* get the rtpmap associated to this codec, if any */
					payload.a_rtpmap =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "rtpmap");
					/* get the fmtp, if any */
					payload.a_fmtp =
						sdp_a_attr_value_get_with_pt
						(remote, i, payload.pt,
						 "fmtp");
					/* ask the application if this codec is supported */
					err = sdph->accept_video_codecs (sdph,
									 ctx,
									 &payload);
				}
			}
		}
	}
}
