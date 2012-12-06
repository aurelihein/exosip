%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Librairie eXosip2
Name            : libexosip2GIP
Version         : 3.5.0
Release         : 0
License         : GPL
Vendor          : CASTEL
Packager        : Aurelien BOUIN
Group           : System Environment/Libraries
URL             : www.linphone.org
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build

export OSIP_CFLAGS='-I$(DEV_IMAGE)/usr/include'
export OSIP_LIBS='$(DEV_IMAGE)/usr/lib/libosip2.so $(DEV_IMAGE)/usr/lib/libosipparser2.so'
export CFLAGS='-I$(DEV_IMAGE)/usr/include/ -DOSIP_MT'

./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --disable-static --with-gnu-ld OSIP_CFLAGS=$OSIP_CFLAGS

make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
