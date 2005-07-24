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

#ifndef __SDPTOOLS_H__
#define __SDPTOOLS_H__

#include <eXosip2/eXosip.h>

int sdp_complete_message (int did, sdp_message_t * remote_sdp,
                          osip_message_t * ack_or_183);
int sdp_complete_200ok (int did, osip_message_t * answer);

int _sdp_hold_call (sdp_message_t * local_sdp);
int _sdp_off_hold_call (sdp_message_t * local_sdp);
int _sdp_analyse_attribute (sdp_message_t * sdp, sdp_media_t * med);

#endif
