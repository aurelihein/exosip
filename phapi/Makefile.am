EXTRA_DIST = 	phmedia-win32.c mec3.h mec3-float.h

if COMPILE_PHAPI
lib_LTLIBRARIES = libphapi.la 
lib_LIBRARIES = libphapicl.a
bin_PROGRAMS = phapiserver

if COMPILE_GSM
if COMPILE_ILBC
SUBDIRS = ilbc gsm
else
SUBDIRS = gsm
endif
else
if COMPILE_ILBC
SUBDIRS = ilbc
endif
endif

endif

if GLIB_SUPPORT
MORE_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@  @EXOSIP_FLAGS@ `pkg-config --cflags glib-2.0`
else
MORE_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@  @EXOSIP_FLAGS@
endif

if UCL_SUPPORT
AM_CFLAGS = $(MORE_CFLAGS) -DUCL_SUPPORT
else
if ORTP_SUPPORT
AM_CFLAGS = $(MORE_CFLAGS) -DORTP_SUPPORT
else
if MEDIASTREAMER_SUPPORT
AM_CFLAGS = $(MORE_CFLAGS) -DMEDIASTREAMER_SUPPORT
endif
endif
endif

libphapi_la_SOURCES = 	phapi.c \
		 	phmedia.c \
			phcodec.c \
			aec.cpp	\
			aec.h	\
			phapi.h \
			phcall.h \
			phmedia.h \
			phcodec.h \
			tonegen.h \
			tonegen.c

libphapicl_a_SOURCES = phapi_client.c phapi.h phrpc.h

phapiserver_SOURCES = phapi_server.c


if GLIB_SUPPORT
GLIB_LIBS=`pkg-config --libs glib-2.0`
endif

libphapi_la_LDFLAGS = -O @EXOSIP_LIB@ @PTHREAD_LIBS@  @GLIB_LIBS@


if UCL_SUPPORT
MORE_MYADD = /usr/local/lib/libuclmmbase.a

MORE_INCLUDES = -I/ucl
else
if ORTP_SUPPORT
MORE_MYADD = -L$(libdir) -lortp -lm

MORE_INCLUDES = -I$(includedir)/ortp
else
if MEDIASTREAMER_SUPPORT
MORE_MYADD = -lortp -lm -lmediastreamer -lmsspeex

MORE_INCLUDES = -I$(includedir)/mediastreamer -I$(includedir)/ortp
endif
endif
endif

CFLAGS += -DPHAPI_VERSION="$(PHAPI_VERSION)"

phapi_includedir=$(includedir)/phapi

phapi_include_HEADERS= phapi.h phrpc.h

if COMPILE_GSM

if COMPILE_ILBC
CODECLIBS = -L$(top_builddir)/phapi/gsm -lgsm  -L$(top_builddir)/phapi/ilbc -lilbc 
else
CODECLIBS = -L$(top_builddir)/phapi/gsm -lgsm 
endif

else

if COMPILE_ILBC
CODECLIBS = -L$(top_builddir)/phapi/ilbc -lilbc 
else
CODECLIBS = 
endif

endif

libphapi_la_LIBADD = $(top_builddir)/src/libeXosip.la -L$(libdir) -losip2 -losipparser2  $(MORE_MYADD)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/josua  -I$(includedir) $(MORE_INCLUDES)

phapiserver_LDADD = $(top_builddir)/phapi/libphapi.la $(top_builddir)/src/libeXosip.la $(CODECLIBS)  -L$(libdir) -losip2 -losipparser2  $(MORE_MYADD) -lstdc++



