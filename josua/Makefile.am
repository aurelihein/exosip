
bin_PROGRAMS = josua

AM_CFLAGS = -Wall  @EXOSIP_FLAGS@ @CFLAGS@  @PTHREAD_CFLAGS@ @JOSUA_FLAGS@

josua_SOURCES = \
commands.c      gui_menu.c      jconfig.c         \
gui.c           gui_new_call.c  main_ncurses.c    \
gui_icon.c      gui_topline.c   pplgetopt.c       \
gui_loglines.c  jcalls.c        gui_manage_call.c \
commands.h      gui_loglines.h  gui_topline.h     \
gui.h           gui_menu.h      jcalls.h          \
gui_icon.h      gui_new_call.h  jconfig.h         \
ppl_getopt.h    gui_manage_call.h


josua_LDFLAGS = -O  -Wall
josua_LDADD = -losip2 @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ \
$(top_builddir)/eXosip/libeXosip.la

INCLUDES = -I$(top_srcdir)

