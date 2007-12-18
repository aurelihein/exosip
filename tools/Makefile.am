
if COMPILE_TOOLS
bin_PROGRAMS = sip_reg
endif

AM_CFLAGS = @CFLAGS@ @PTHREAD_CFLAGS@ @EXOSIP_FLAGS@

sip_reg_SOURCES = sip_reg.c
sip_reg_LDADD = $(top_builddir)/src/libeXosip2.la @TOOLS_LIBS@ $(OSIP_LIBS) $(EXOSIP_LIB) $(PTHREAD_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include $(OSIP_CFLAGS)
