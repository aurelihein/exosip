
if COMPILE_JOSUA
bin_PROGRAMS = josua
endif

if GLIB_SUPPORT
MORE_CFLAGS = -Wall @CFLAGS@ @PTHREAD_CFLAGS@ @JOSUA_FLAGS@ @EXOSIP_FLAGS@ `pkg-config --cflags glib-2.0`
else
MORE_CFLAGS = -Wall @CFLAGS@ @PTHREAD_CFLAGS@ @JOSUA_FLAGS@ @EXOSIP_FLAGS@
endif

if MEDIASTREAMER_SUPPORT
AM_CFLAGS = $(MORE_CFLAGS) -DMEDIASTREAMER_SUPPORT
endif
if ORTP_SUPPORT
AM_CFLAGS = $(MORE_CFLAGS) -DORTP_SUPPORT
endif

josua_SOURCES = \
commands.c                  gui_new_identity.c       jinsubscriptions.c     \
gui.c                       gui_online.c             jsubscriptions.c       \
gui_address_book_browse.c   gui_registrations_list.c main_ncurses.c         \
gui_address_book_newentry.c gui_sessions_list.c      main_shell.c           \
gui_icon.c                  gui_setup.c              pplgetopt.c            \
gui_insubscriptions_list.c  gui_subscriptions_list.c sound-mediastreamer.c  \
gui_loglines.c              gui_topline.c            sound-ortp.c           \
gui_menu.c                  jcalls.c                 sound-win32.c          \
gui_new_call.c              jconfig.c                sound_conv.c           \
commands.h                  gui_menu.h               gui_subscriptions_list.h \
gui.h                       gui_new_call.h           gui_topline.h          \
gui_address_book_browse.h   gui_new_identity.h       jcalls.h               \
gui_address_book_newentry.h gui_online.h             jconfig.h              \
gui_icon.h                  gui_registrations_list.h jinsubscriptions.h     \
gui_insubscriptions_list.h  gui_sessions_list.h      jsubscriptions.h       \
gui_loglines.h              gui_setup.h              ppl_getopt.h

if GLIB_SUPPORT
josua_LDFLAGS = -O -Wall -leXosip -losip2 @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@  `pkg-config --libs glib-2.0`
else
josua_LDFLAGS = -O  -Wall -leXosip -losip2 @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ 
endif

if ORTP_SUPPORT
MORE_MYADD = -lortp -lm

MORE_INCLUDES = -I$(includedir)/ortp
else
if MEDIASTREAMER_SUPPORT
MORE_MYADD = -lortp -lm -lmediastreamer -lmsspeex

MORE_INCLUDES = -I$(includedir)/mediastreamer -I$(includedir)/ortp
endif
endif

josua_LDADD = $(top_builddir)/src/libeXosip.la -losip2 $(MORE_MYADD)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include -I$(includedir) $(MORE_INCLUDES)


