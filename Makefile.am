
EXTRA_DIST = README autogen.sh eX_script

bin_PROGRAMS = eXosip_addfriend.sh eXosip_addidentity.sh eXosip_addsubscriber.sh

eXosip_addfriend_sh_SOURCES = eXosip_addfriend.sh
eXosip_addidentity_sh_SOURCES = eXosip_addidentity.sh
eXosip_addsubscriber_sh_SOURCES = eXosip_addsubscriber.sh

SUBDIRS = eXosip josua scripts

josuarpm:
	rpm -ba josua.spec

eXosiprpm:
	rpm -ba eXosip.spec

