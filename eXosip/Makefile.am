

lib_LTLIBRARIES = libeXosip.la

AM_CFLAGS = -Wall  @EXOSIP_FLAGS@ @CFLAGS@  @PTHREAD_CFLAGS@

libeXosip_la_SOURCES = eXosip.h  eXosip2.h      eXosip.c  \
jrequest.c jresponse.c    jcallback.c  jdialog.c   udp.c  \
jcall.c jreg.c jfreinds.c jidentity.c  eXutils.c          \
jnotify.c      jsubscribers.c          jsubscribe.c    jevents.c \
sdp_offans.c   misc.c
#sdpcontext.c sdphandler.c    \
#bodyhandler.c  bodycontext.c

libeXosip_la_LDFLAGS = -version-info $(LIBEXOSIP_SO_VERSION)
libeXosip_la_LIBADD = -losip2  @EXOSIP_LIB@ @PTHREAD_LIBS@

INCLUDES = -I$(top_srcdir)
