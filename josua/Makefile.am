
bin_PROGRAMS = josua

AM_CFLAGS = -Wall  @EXOSIP_FLAGS@ @CFLAGS@  @PTHREAD_CFLAGS@ @JOSUA_FLAGS@

josua_SOURCES = \
main_ncurses.c 	main_shell.c \
jconfig.c 	jconfig.h \
pplgetopt.c 	ppl_getopt.h

josua_LDFLAGS = -O  -Wall
josua_LDADD = -lfsmtl2 -losip2  \
@EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ \
$(top_builddir)/eXosip/libeXosip.la

INCLUDES = -I$(top_srcdir)

