
bin_PROGRAMS = josua

AM_CFLAGS = -Wall  @EXOSIP_FLAGS@ @CFLAGS@  @PTHREAD_CFLAGS@ @JOSUA_FLAGS@

josua_SOURCES = jcalls.c \
commands.c      gui_menu.c         jconfig.c          \
gui.c           gui_new_call.c     main_ncurses.c     \
gui_icon.c      gui_topline.c      pplgetopt.c        \
gui_loglines.c  gui_sessions_list.c   \
gui_address_book_newentry.c \
gui_address_book_newentry.h        gui_address_book_browse.h \
gui_address_book_browse.c \
commands.h      gui_loglines.h     gui_topline.h     \
gui.h           gui_menu.h         jcalls.h          \
gui_icon.h      gui_new_call.h     jconfig.h         \
ppl_getopt.h    gui_sessions_list.h \
gui_registrations_list.c gui_registrations_list.h


josua_LDFLAGS = -O  -Wall
josua_LDADD = -losip2 @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ \
$(top_builddir)/eXosip/libeXosip.la

INCLUDES = -I$(top_srcdir)

