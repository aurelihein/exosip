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

#ifdef ORTP_SUPPORT

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#endif

extern char    *localip;
#endif

jcall_t jcalls[MAX_NUMBER_OF_CALLS];

#define AUDIO_DEVICE "/dev/dsp"

static int ___call_init = 0;

int fd = -1;

static int min_size = 0;

static int __call_init()
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      jcalls[k].state = NOT_USED;
    }

#ifdef ORTP_SUPPORT
  ortp_init();
  ortp_scheduler_init();

  fd=open(AUDIO_DEVICE, O_RDWR|O_NONBLOCK);
  if (fd<0) return -EWOULDBLOCK;

  /* open the device */
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

#ifdef HAVE_SYS_SOUNDCARD_H
  {
    int p,cond;
    int bits = 16;
    int stereo = 0;
    int rate = 8000;
    int blocksize = 512;

    ioctl(fd, SNDCTL_DSP_RESET, 0);
  
    p =  bits;  /* 16 bits */
    ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);
    
    p =  stereo;  /* number of channels */
    ioctl(fd, SNDCTL_DSP_CHANNELS, &p);
  
    p =  rate;  /* rate in khz*/
    ioctl(fd, SNDCTL_DSP_SPEED, &p);
    
    ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
    if (min_size>blocksize)
      {
	cond=1;
	p=min_size/blocksize;
	while(cond)
	  {
	    int i=ioctl(fd, SNDCTL_DSP_SUBDIVIDE, &p);
	    /* printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno); */
	    if ((i==0) || (p==1)) cond=0;
	    else p=p/2;
	  }
      }
    ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
    if (min_size>blocksize)
      {
	printf("dsp block size set to %i.",min_size);
	exit(0);
      }else{
	/* no need to access the card with less latency than needed*/
	min_size=blocksize;
      }
  }
  printf("blocksize = %i\n", min_size);
#endif
#endif
  return 0;
}

#ifdef ORTP_SUPPORT

void
rcv_telephone_event(RtpSession *rtp_session, jcall_t *ca)
{
  /* telephone-event received! */
  
}

void
*jcall_ortp_start_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_in[160];
  /*  char data_out[160]; */
  int have_more;
  int timestamp = 0;
  int stream_received = 0;
  int i;
  while (ca->enable_audio != -1)
    {
      memset(data_in, 0, 160);
      i = rtp_session_recv_with_ts(ca->rtp_session, data_in, 160, timestamp, &have_more);
      if (i>0) stream_received=1;
      /* this is to avoid to write to disk some silence before
	 the first RTP packet is returned*/     
      if ((stream_received) && (i>0)) {
	write(fd, data_in, i);
      }
      timestamp += 160;
    }
  return NULL;
}

void
*jcall_ortp_start_out_thread(void *_ca)
{
  jcall_t *ca = (jcall_t*)_ca;
  char data_out[10000];
  int timestamp = 0;
  int i;
  while (ca->enable_audio != -1)
    {
      memset(data_out, 0, min_size);
      i=read(fd, data_out, min_size);
      if (i>0)
        {
	  printf("data_out=%i\n",i);
	  rtp_session_send_with_ts(ca->rtp_session, data_out, i,timestamp);
	  timestamp+=i;
        }
    }
  return NULL;
}

#endif

int
jcall_get_number_of_pending_calls()
{
  int pos=0;
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED)
	{
	  pos++;
	}
    }
  return pos;
}

jcall_t *jcall_find_call(int pos)
{
  int k;
  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED)
	{
	  if (pos==0)
	    return &(jcalls[k]);
	  pos--;
	}
    }
  return NULL;
}

int jcall_new(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state == NOT_USED)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  memset(&(jcalls[k]), 0, sizeof(jcall_t));

  ca->cid = je->cid;
  ca->did = je->did;

  if (ca->did<1 && ca->did<1)
    {
      exit(0);
      return -1; /* not enough information for this event?? */
    }

  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      int p;
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);

#ifdef ORTP_SUPPORT
      if (ca->payload==0)
	p =  AFMT_MU_LAW;  /* rate in khz*/
      else if (ca->payload==8)
	p = AFMT_A_LAW;
      ioctl(fd, SNDCTL_DSP_SETFMT, &p);

      ca->rtp_session = rtp_session_new(RTP_SESSION_SENDRECV);
      rtp_session_set_scheduling_mode(ca->rtp_session, 1); /* yes */
      rtp_session_set_blocking_mode(ca->rtp_session, 1);

      rtp_session_set_profile(ca->rtp_session, &av_profile);
      rtp_session_set_jitter_compensation(ca->rtp_session, 60);
      rtp_session_set_local_addr(ca->rtp_session, localip, 10500);
      rtp_session_set_remote_addr(ca->rtp_session,
				  ca->remote_sdp_audio_ip,
				  ca->remote_sdp_audio_port);
      rtp_session_set_payload_type(ca->rtp_session, je->payload);
      rtp_session_signal_connect(ca->rtp_session, "telephone-event",
				 (RtpCallback)rcv_telephone_event, ca);

      /* enter a loop (thread?) to send AUDIO data with
	 rtp_session_send_with_ts(ca->rtp_session, data, data_length, timestamp);
      */
      ca->audio_thread = osip_thread_create(20000,
					    jcall_ortp_start_thread, ca);
      ca->out_audio_thread = osip_thread_create(20000,
					    jcall_ortp_start_out_thread, ca);
      ca->enable_audio=1; /* audio is started */

#endif
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }

  ca->state = je->type;
  return 0;
}

int jcall_remove(jcall_t *ca)
{
  if (ca==NULL)
    return -1;
  ca->state = NOT_USED;
  return 0;
}

int jcall_proceeding(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;

      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;

}

int jcall_ringing(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;
      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;;
  return 0;
}

int jcall_answered(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    {
      for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
	{
	  if (jcalls[k].state == NOT_USED)
	    break;
	}
      if (k==MAX_NUMBER_OF_CALLS)
	return -1;
      ca = &(jcalls[k]);
      memset(&(jcalls[k]), 0, sizeof(jcall_t));
      
      ca->cid = je->cid;
      ca->did = je->did;
      
      if (ca->did<1 && ca->did<1)
	{
	  exit(0);
	  return -1; /* not enough information for this event?? */
	}
    }

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);
  osip_strncpy(ca->req_uri,    je->req_uri, 255);
  osip_strncpy(ca->local_uri,  je->local_uri, 255);
  osip_strncpy(ca->remote_uri, je->remote_uri, 255);
  osip_strncpy(ca->subject,    je->subject, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      int p;
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
      ca->payload = je->payload;
      osip_strncpy(ca->payload_name, je->payload_name, 49);

#ifdef ORTP_SUPPORT
      if (ca->payload==0)
	p =  AFMT_MU_LAW;  /* rate in khz*/
      else if (ca->payload==8)
	p = AFMT_A_LAW;
      ioctl(fd, SNDCTL_DSP_SETFMT, &p);

      ca->rtp_session = rtp_session_new(RTP_SESSION_RECVONLY);
      rtp_session_set_scheduling_mode(ca->rtp_session, 1); /* yes */
      rtp_session_set_blocking_mode(ca->rtp_session, 1);

      rtp_session_set_profile(ca->rtp_session, &av_profile);
      rtp_session_set_jitter_compensation(ca->rtp_session, 60);
      rtp_session_set_local_addr(ca->rtp_session, localip, 10500);
      p = rtp_session_set_remote_addr(ca->rtp_session,
				      ca->remote_sdp_audio_ip,
				      ca->remote_sdp_audio_port);
      rtp_session_set_payload_type(ca->rtp_session, je->payload);
      rtp_session_signal_connect(ca->rtp_session, "telephone-event",
				 (RtpCallback)rcv_telephone_event, ca);

      /* enter a loop (thread?) to send AUDIO data with
	 rtp_session_send_with_ts(ca->rtp_session, data, data_length, timestamp);
      */
      ca->audio_thread = osip_thread_create(20000,
					    jcall_ortp_start_thread, ca);
      ca->out_audio_thread = osip_thread_create(20000,
					    jcall_ortp_start_out_thread, ca);
      ca->enable_audio=1; /* audio is started */
#endif
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jcall_redirected(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

#ifdef ORTP_SUPPORT
  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      osip_thread_join(ca->audio_thread);
      osip_free(ca->audio_thread);
      osip_thread_join(ca->out_audio_thread);
      osip_free(ca->out_audio_thread);
      rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
						(RtpCallback)rcv_telephone_event);
      rtp_session_destroy(ca->rtp_session);
    }
#endif

  ca->state = NOT_USED;
  return 0;
}

int jcall_requestfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

#ifdef ORTP_SUPPORT
  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      osip_thread_join(ca->audio_thread);
      osip_free(ca->audio_thread);
      osip_thread_join(ca->out_audio_thread);
      osip_free(ca->out_audio_thread);
      rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
						(RtpCallback)rcv_telephone_event);
      rtp_session_destroy(ca->rtp_session);
    }
#endif

  ca->state = NOT_USED;
  return 0;
}

int jcall_serverfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);


#ifdef ORTP_SUPPORT
  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      osip_thread_join(ca->audio_thread);
      osip_free(ca->audio_thread);
      osip_thread_join(ca->out_audio_thread);
      osip_free(ca->out_audio_thread);
      rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
						(RtpCallback)rcv_telephone_event);
      rtp_session_destroy(ca->rtp_session);
    }
#endif

  ca->state = NOT_USED;
  return 0;
}

int jcall_globalfailure(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

#ifdef ORTP_SUPPORT
  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      osip_thread_join(ca->audio_thread);
      osip_free(ca->audio_thread);
      osip_thread_join(ca->out_audio_thread);
      osip_free(ca->out_audio_thread);
      rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
						(RtpCallback)rcv_telephone_event);
      rtp_session_destroy(ca->rtp_session);
    }
#endif

  ca->state = NOT_USED;
  return 0;
}

int jcall_closed(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);

#ifdef ORTP_SUPPORT
  if (ca->enable_audio>0)
    {
      ca->enable_audio = -1;
      osip_thread_join(ca->audio_thread);
      osip_free(ca->audio_thread);
      osip_thread_join(ca->out_audio_thread);
      osip_free(ca->out_audio_thread);
      rtp_session_signal_disconnect_by_callback(ca->rtp_session, "telephone-event",
						(RtpCallback)rcv_telephone_event);
      rtp_session_destroy(ca->rtp_session);
    }
#endif

  ca->state = NOT_USED;
  return 0;
}

int jcall_onhold(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

int jcall_offhold(eXosip_event_t *je)
{
  jcall_t *ca;
  int k;

  if (___call_init==0)
    {
      ___call_init = -1;
      __call_init();
    }

  for (k=0;k<MAX_NUMBER_OF_CALLS;k++)
    {
      if (jcalls[k].state != NOT_USED
	  && jcalls[k].cid==je->cid
	  && jcalls[k].did==je->did)
	break;
    }
  if (k==MAX_NUMBER_OF_CALLS)
    return -1;

  ca = &(jcalls[k]);
  osip_strncpy(ca->textinfo,   je->textinfo, 255);

  if (ca->remote_sdp_audio_ip[0]=='\0')
    {
      osip_strncpy(ca->remote_sdp_audio_ip, je->remote_sdp_audio_ip, 49);
      ca->remote_sdp_audio_port = je->remote_sdp_audio_port;
    }

  if (je->reason_phrase[0]!='\0')
    {
      osip_strncpy(ca->reason_phrase, je->reason_phrase, 49);
      ca->status_code = je->status_code;
    }
  ca->state = je->type;
  return 0;
}

