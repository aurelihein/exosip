
if COMPILE_JOSUA
lib_LTLIBRARIES = libjosua.la
bin_PROGRAMS = josua
endif

if GLIB_SUPPORT
MORE_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@ @JOSUA_FLAGS@ @EXOSIP_FLAGS@ `pkg-config --cflags glib-2.0`
else
MORE_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@ @JOSUA_FLAGS@ @EXOSIP_FLAGS@
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

josua_SOURCES = \
gui_new_identity.c       \
gui.c                       gui_online.c             \
gui_address_book_browse.c   gui_registrations_list.c main_ncurses.c         \
gui_address_book_newentry.c gui_sessions_list.c                 \
gui_icon.c                  gui_setup.c              \
gui_insubscriptions_list.c  gui_subscriptions_list.c \
gui_loglines.c              gui_topline.c            \
gui_menu.c                  \
gui_new_call.c              \
gui_menu.h               gui_subscriptions_list.h \
gui.h                       gui_new_call.h           gui_topline.h          \
gui_address_book_browse.h   gui_new_identity.h       \
gui_address_book_newentry.h gui_online.h             \
gui_icon.h                  gui_registrations_list.h \
gui_insubscriptions_list.h  gui_sessions_list.h      \
gui_loglines.h              gui_setup.h 

libjosua_la_SOURCES = \
commands.c	jinsubscriptions.c jsubscriptions.c       \
  jcalls.c                               jconfig.c commands.h      jcalls.h               \
  jconfig.h  jinsubscriptions.h        jsubscriptions.h ppl_getopt.h pplgetopt.c         \
sound-mediastreamer.c sound-ortp.c            sound-ucl.c        sound-win32.c  sound_conv.c  sound-mac.c


libjosua_la_includedir=$(includedir)/josua
libjosua_la_include_HEADERS=jmainconfig.h


if GLIB_SUPPORT
josua_LDFLAGS = -O @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@  `pkg-config --libs glib-2.0`
else
josua_LDFLAGS = -O @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ 
endif


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

josua_LDADD = $(top_builddir)/src/libeXosip.la libjosua.la $(libdir)/libosip2.la $(libdir)/libosipparser2.la -lrt $(MORE_MYADD)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(includedir) $(MORE_INCLUDES)

libjosua_la_LIBADD = $(top_builddir)/src/libeXosip.la $(libdir)/libosip2.la $(libdir)/libosipparser2.la  $(MORE_MYADD)

