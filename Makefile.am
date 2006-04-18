
EXTRA_DIST = README autogen.sh eXosip_addidentity.sh

SUBDIRS = scripts platform src include josua tools help

doxygen:
	doxygen help/doxygen/DoxyFile

upload: dist
	@echo "######## Maintainer only ##########";
	@echo "building files for upload in ftp://savannah.gnu.org/incoming/savannah/exosip";
	@echo "building: $(PACKAGE)-$(VERSION).tar.gz.sig";
	gpg -b $(PACKAGE)-$(VERSION).tar.gz

install-exec-local:
	@echo "*** You can use eXosip_addidentity.sh to generate";
	@echo "*** an identity file which will be used for SIP";
	@echo "*** registrations.";
	@echo "*** ~/.eXosip/jm_identity  should look like:";
	@echo "*** ";
	@echo "*** 0 ENTRY: sip:jack@antisip.com|sip:sip.antisip.com|*empty*|realm|login|password";
	@echo "*** 1 ENTRY: sip:jack@antisip.com|sip:192.168.2.2|*empty*|realm|login|password";
	@echo "*** ";


indent:
	indent */*.h
	indent */*.c
	rm -rf */*.h~
	rm -rf */*.c~
