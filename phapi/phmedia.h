/*
 * phmedia -  Phone Api media streamer
 *
 * Copyright (C) 2004 Vadim Lebedev <vadim@mbdsys.com>
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
#ifndef __PHMEDIA_H__
#define __PHMEDIA_H__ 1

#define PH_MEDIA_DTMF_PAYLOAD 101
#define PH_MEDIA_ILBC_PAYLOAD 97

struct ph_media_payload_s
{
  int  number;
  char string[32];
  int  rate;
};

typedef struct ph_media_payload_s  ph_media_payload_t;

int ph_media_supported_payload(ph_media_payload_t *pt, const char *ptstring);

int ph_media_init(void);
int ph_media_cleanup(void);

int ph_media_start(phcall_t *ca, int port, void (*dtmfCallback)(phcall_t *ca, int event), const char * deviceId);
void ph_media_stop(phcall_t *ca);


int ph_media_send_dtmf(phcall_t *ca, int dtmf, int mode);

void ph_media_suspend(phcall_t *ca);
void ph_media_resume(phcall_t *ca);



#endif

