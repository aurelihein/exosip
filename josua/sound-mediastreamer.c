/*
 * sh_phone - Shell Phone
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This software is protected by copyright. For more information
 * write to <jack@atosc.org>.
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

int os_sound_start(jcall_t *ca)
{
  ca->audio=audio_stream_start(&av_profile,
			       10500,
			       ca->remote_sdp_audio_ip,
			       ca->remote_sdp_audio_port,
			       ca->payload,
			       250);
  return 0;
}

void os_sound_close(jcall_t *ca)
{
  audio_stream_stop(ca->audio);
}

#endif
