

lib_LTLIBRARIES = libeXosip.la

#
# Other flags to add someday:
#	-Wcast-qual
#	-Wwrite-strings
#	-Wstrict-prototypes
#	-Werror
#
AM_CFLAGS = @EXOSIP_FLAGS@ @CFLAGS@  @PTHREAD_CFLAGS@

libeXosip_la_SOURCES =       eXosip.c  \
jrequest.c jresponse.c    jcallback.c  jdialog.c   udp.c  \
jcall.c jreg.c jfreinds.c jidentity.c  eXutils.c          \
jnotify.c      jsubscribers.c          jsubscribe.c    jevents.c \
sdp_offans.c   misc.c                  eXosip2.h         \
jpipe.c        jpipe.h                 jauth.c     jpublish.c

libeXosip_la_LDFLAGS = -L$(prefix)/lib -version-info $(LIBEXOSIP_SO_VERSION)
libeXosip_la_LIBADD = -losip2  @EXOSIP_LIB@ @PTHREAD_LIBS@

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(includedir)
