#ifndef __PHCALL_H__
#define __PHCALL_H__ 1

#define PH_MAX_CALLS  32

struct phcall
{
  int cid;
  int did;
  char remote_sdp_audio_ip[64];
  int  remote_sdp_audio_port;
  char payload_name[32];
  int  payload;
  int  hasaudio;
  int  remotehold;
  int  localhold;
  int  localresume;
  void *phstream;
};


typedef struct phcall phcall_t;



#endif
