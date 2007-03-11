
if COMPILE_TOOLS
bin_PROGRAMS = sip_reg
endif

AM_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@ @EXOSIP_FLAGS@

sip_reg_SOURCES = sip_reg.c

sip_reg_LDFLAGS = -O @EXOSIP_LIB@ @PTHREAD_LIBS@

sip_reg_LDADD = -L$(prefix)/lib $(top_builddir)/src/libeXosip2.la -losip2 -losipparser2 @TOOLS_LIBS@

INCLUDES = -I$(prefix)/include -I$(top_srcdir) -I$(top_srcdir)/include -I$(includedir)


