
if COMPILE_TOOLS
bin_PROGRAMS = sip_reg
endif

sip_reg_SOURCES = sip_reg.c

sip_reg_LDFLAGS = -O @EXOSIP_LIB@ @PTHREAD_LIBS@ @JOSUA_LIB@ 

sip_reg_LDADD = -L$(prefix)/lib $(top_builddir)/src/libeXosip.la -losip2 -losipparser2

INCLUDES = -I$(prefix)/include -I$(top_srcdir) -I$(top_srcdir)/include -I$(includedir)


