#ifndef __PHCODEC_H__
#define  __PHCODEC_H__

struct phcodec
{
  const char *mime;
  int   encoded_framesize;
  int   decoded_framesize;
  void  *(*encoder_init)();
  void  *(*decoder_init)();
  void  (*encoder_cleanup)(void *ctx);
  void  (*decoder_cleanup)(void *ctx);
  int   (*encode)(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
  int   (*decode)(void *ctx, const void *src, int srcsize, void *dst, int dstsize);
  struct phcodec *next;
 
};

typedef struct phcodec phcodec_t;


extern phcodec_t *ph_codec_list;

void ph_media_codecs_init(void);


#endif
