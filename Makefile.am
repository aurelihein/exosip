
EXTRA_DIST = README autogen.sh eX_script eXosip_addfriend.sh eXosip_addidentity.sh eXosip_addsubscriber.sh

SUBDIRS = scripts platform src include josua tools phapi minims miniua

josuarpm:
	rpm -ba josua.spec

eXosiprpm:
	rpm -ba eXosip.spec

install-exec-local:
	echo "Installing script file for Josua";
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@chmod 0755 $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addsubscriber.sh $(DESTDIR)$(bindir)/eXosip_addsubscriber.sh
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addidentity.sh $(DESTDIR)$(bindir)/eXosip_addidentity.sh
	$(INSTALL_SCRIPT) $(top_srcdir)/eXosip_addfriend.sh $(DESTDIR)$(bindir)/eXosip_addfriend.sh
