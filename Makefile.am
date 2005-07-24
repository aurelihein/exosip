
EXTRA_DIST = README autogen.sh eXosip_addfriend.sh eXosip_addidentity.sh eXosip_addsubscriber.sh

SUBDIRS = scripts platform src include josua tools help

doxygen:
	doxygen help/doxygen/DoxyFile

install-exec-local:
	echo "Installing script file for Josua";
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@chmod 0755 $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addsubscriber.sh $(DESTDIR)$(bindir)/eXosip_addsubscriber.sh
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addidentity.sh $(DESTDIR)$(bindir)/eXosip_addidentity.sh
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addfriend.sh $(DESTDIR)$(bindir)/eXosip_addfriend.sh

indent:
	indent */*.h
	indent */*.c
	rm -rf */*.h~
	rm -rf */*.c~
