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

#include "jcalls.h"

#ifdef MEDIASTREAMER_SUPPORT

int os_sound_init()
{
  ortp_init();
  ortp_set_debug_file("oRTP",NULL);
  rtp_profile_set_payload(&av_profile,115,&lpc1015);
  rtp_profile_set_payload(&av_profile,110,&speex_nb);
  rtp_profile_set_payload(&av_profile,111,&speex_wb);

  ms_init();
  ms_speex_codec_init();
  return 0;
}

int os_sound_start(jcall_t *ca, int port)
{
  /* creates the couple of encoder/decoder */
  PayloadType *pt;
  pt=rtp_profile_get_payload(&av_profile,ca->payload);
  if (pt==NULL){
    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			    "undefined payload type\n"));
    return -1;
  }
  if (ms_encoder_new_with_string_id(pt->mime_type)==NULL
      || ms_decoder_new_with_string_id(pt->mime_type)==NULL)
    {
      /* No registered codec for this payload...*/
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "No decoder availlable for payload %i\n", ca->payload));
      return -1;
    }

  ca->audio=audio_stream_start(&av_profile,
			       port,
			       ca->remote_sdp_audio_ip,
			       ca->remote_sdp_audio_port,
			       ca->payload,
			       250);
  return 0;
}

void os_sound_close(jcall_t *ca)
{
  if (ca->audio!=NULL)
    {
      audio_stream_stop(ca->audio);
    }
}

#endif
