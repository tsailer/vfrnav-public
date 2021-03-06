Name:           vfrnav
Version:        20160429
Release:        1%{?dist}
Summary:        VFR/IFR Navigation

Group:          Applications/Productivity
License:        GPLv2+
URL:            http://www.baycom.org/~tom/vfrnav
Source0:        http://download.gna.org/vfrnav/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%bcond_without webservice

BuildRequires:  gtkmm30-devel
BuildRequires:  libxml++-devel >= 2.14.0
BuildRequires:  sqlite-devel >= 3.0
BuildRequires:  gpsd-devel
BuildRequires:  libsqlite3x-devel
BuildRequires:  gdal-devel
BuildRequires:  bluez-libs-devel
BuildRequires:  eigen3-devel
BuildRequires:  boost-devel
BuildRequires:  cairomm-devel
BuildRequires:  cairo-devel
BuildRequires:  zfstream-devel
BuildRequires:  gettext
BuildRequires:  antlr-C++
%ifnarch s390 s390x
BuildRequires:  pilot-link-devel
%endif
BuildRequires:  gnome-bluetooth-libs-devel
BuildRequires:  desktop-file-utils
BuildRequires:  ImageMagick
BuildRequires:  geoclue2-devel
BuildRequires:  gsl-devel
BuildRequires:  libftdi-devel
BuildRequires:  libftdi-c++-devel
BuildRequires:  libudev-devel
BuildRequires:  libXScrnSaver-devel
BuildRequires:  evince-devel
BuildRequires:  libusb1-devel
BuildRequires:  libcurl-devel
BuildRequires:  webkitgtk4-devel
BuildRequires:  openjpeg-devel
BuildRequires:  libsoup-devel
BuildRequires:  librsvg2-devel
BuildRequires:  podofo-devel
BuildRequires:  systemd-devel
BuildRequires:  transfig
BuildRequires:  texlive
BuildRequires:  texlive-texconfig
BuildRequires:  texlive-latex
BuildRequires:  texlive-latex-bin-bin
BuildRequires:  texlive-epstopdf
BuildRequires:  texlive-epstopdf-bin
BuildRequires:  texlive-umlaute
BuildRequires:  texlive-babel-german
BuildRequires:  texlive-multirow
BuildRequires:  texlive-oberdiek
BuildRequires:  texlive-tools
BuildRequires:  poppler-utils
BuildRequires:  readline-devel
BuildRequires:  geos-devel
BuildRequires:  polyclipping-devel
BuildRequires:  libpqxx-devel
BuildRequires:  octave
BuildRequires:  octave-devel
BuildRequires:  inkscape
BuildRequires:  selinux-policy-devel
Requires:       libreoffice-core
Requires:       libreoffice-calc
Requires:       texlive
Requires:       texlive-latex
Requires:       texlive-latex-bin-bin
Requires:       texlive-droid                            
Requires:       texlive-contour
Requires:       texlive-truncate
Requires:       texlive-changebar
Requires:       texlive-eurosym
Requires:       texlive-xstring
Requires:       texlive-hyphenat
Requires:       tex(t2aenc.def)
Requires:       texlive-lh
Requires:       texlive-xetex-def
Requires:       texlive-framed
Requires:       texlive-polyglossia
Requires:       texlive-multirow

%if %{with webservice}
BuildRequires:  jsoncpp-devel
BuildRequires:  sqlite
%endif

# Auto BR:
# BuildRequires: atk-devel
# BuildRequires: bash
# BuildRequires: binutils
# BuildRequires: boost-devel
# BuildRequires: cairo-devel
# BuildRequires: cairomm-devel
# BuildRequires: coreutils
# BuildRequires: cpio
# BuildRequires: diffutils
# BuildRequires: elfutils
# BuildRequires: file
# BuildRequires: findutils
# BuildRequires: gawk
# BuildRequires: gcc-c++
# BuildRequires: gcc-gfortran
# BuildRequires: gcc
# BuildRequires: gdal-devel
# BuildRequires: gettext
# BuildRequires: glib2-devel
# BuildRequires: glibc-common
# BuildRequires: glibc-devel
# BuildRequires: glibc-headers
# BuildRequires: glibmm24-devel
# BuildRequires: gnome-bluetooth-devel
# BuildRequires: gnome-libs-devel
# BuildRequires: gnome-vfs2-devel
# BuildRequires: gpsd-devel
# BuildRequires: grep
# BuildRequires: gtk2-devel
# BuildRequires: gtkmm24-devel
# BuildRequires: gzip
# BuildRequires: kernel-headers
# BuildRequires: libart_lgpl-devel
# BuildRequires: libbonobo-devel
# BuildRequires: libbonoboui-devel
# BuildRequires: libbtctl-devel
# BuildRequires: libglademm24-devel
# BuildRequires: libgnomecanvas-devel
# BuildRequires: libgnome-devel
# BuildRequires: libgnomeui-devel
# BuildRequires: libsigc++20-devel
# BuildRequires: libsqlite3x-devel
# BuildRequires: libstdc++-devel
# BuildRequires: libxml2-devel
# BuildRequires: libxml++-devel
# BuildRequires: make
# BuildRequires: net-tools
# BuildRequires: pango-devel
# BuildRequires: pangomm-devel
# BuildRequires: pilot-link-devel
# BuildRequires: pkgconfig
# BuildRequires: popt-devel
# BuildRequires: sed
# BuildRequires: sqlite36-devel
# BuildRequires: tar
# BuildRequires: zfstream-devel
# BuildRequires: zlib-devel

%description
This is a navigation application for VFR and IFR flying.

%package utils
Summary:        VFR Navigation Utilities
Group:          Applications/Productivity
Requires:       %{name} = %{version}
Requires:       php
Requires:       php-mbstring

%description utils
This package contains utilities for database creation and manipulation
for the VFR navigation application.

%package wetterdl
Summary:        VFR Navigation Weather Downloader
Group:          Applications/Productivity
Requires:       %{name} = %{version}

%description wetterdl
This package contains a downloader application for weather
pictures from flugwetter.de and other sources.

%package validatorservice
Summary:        VFR Navigation CFMU Validator Service
Group:          Applications/Productivity
Requires:       %{name} = %{version}
Requires:       %{name}-selinux = %{version}
Requires:       xorg-x11-server-Xvfb

%description validatorservice
Opening the connection to the CFMU validator takes some time (in the order
of seconds to minutes). In order to amortize this time over multiple tasks,
this package contains a local socket validator server.

%package selinux
Summary:        VFR Navigation selinux module
Group:          Applications/Productivity
Requires:       %{name} = %{version}
BuildArch:      noarch
Requires:       selinux-policy
Requires(post):         policycoreutils
Requires(postun):       policycoreutils

%description selinux
This package contains the selinux module required for vfrnav

%if %{with webservice}
%package webservice
Summary:        VFR Navigation CFMU Autorouter Webservice
Group:          Applications/Productivity
Requires:       %{name} = %{version}
Requires:       %{name}-validatorservice = %{version}
Requires:       %{name}-selinux = %{version}
Requires:       httpd

%description webservice
This package contains a webservice for the CFMU Autorouter.
%endif

%prep
%setup -q

%build
#configure --disable-gtk3
CXXFLAGS=`echo %optflags -std=c++11 | sed -e 's/-O2//'`
export CXXFLAGS
CFLAGS=`echo %optflags | sed -e 's/-O2//'`
export CFLAGS
%configure
make %{?_smp_mflags}
# build selinux module
make -C data/selinux -f %{_datadir}/selinux/devel/Makefile

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

for i in $RPM_BUILD_ROOT/%{_datadir}/applications/*.desktop; do
  grep -v '^\(\(X-\)\|\(Version\)\|\(Encoding\)\)' $i > $i.tmp
  sed -e s,Exec=/usr/bin/,Exec=, < $i.tmp > $i
  rm -f $i.tmp
done

for i in $RPM_BUILD_ROOT/%{_datadir}/applications/*.desktop; do
  desktop-file-validate $i
done

# convert icons to sane dimensions
install -d $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/32x32/apps
install -d $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/48x48/apps
convert -size 32x32 $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/26x26/apps/vfrnav.png $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/32x32/apps/vfrnav.png
convert -size 48x48 $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/40x40/apps/vfrnav.png $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/48x48/apps/vfrnav.png
convert -size 32x32 $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/26x26/apps/wetterdl.png $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/32x32/apps/wetterdl.png
convert -size 48x48 $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/40x40/apps/wetterdl.png $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/48x48/apps/wetterdl.png
rm -rf $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/26x26/
rm -rf $RPM_BUILD_ROOT/%{_datadir}/icons/hicolor/40x40/

install -d $RPM_BUILD_ROOT/%{_sysconfdir}/vfrnav

rm -f $RPM_BUILD_ROOT/%{_sbindir}/cfmuws
rm -f $RPM_BUILD_ROOT/lib/systemd/system/cfmuws.service
rm -f $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/cfmuws
rm -f $RPM_BUILD_ROOT/%{_sysconfdir}/vfrnav/cfmuwsusers.json

#rm -f $RPM_BUILD_ROOT/%{_bindir}/cfmusidstar

rm -f $RPM_BUILD_ROOT/%{_libdir}/libvfrnav.so
rm -f $RPM_BUILD_ROOT/%{_libdir}/libvfrnav.la
rm -f $RPM_BUILD_ROOT/%{_libdir}/libvfrnav.a

rm -f $RPM_BUILD_ROOT/%{_libexecdir}/%{name}/web-extensions/libcfmuwebextension.a
rm -f $RPM_BUILD_ROOT/%{_libexecdir}/%{name}/web-extensions/libcfmuwebextension.la

install -d $RPM_BUILD_ROOT/run/vfrnav/validator
install -d $RPM_BUILD_ROOT/var/lib/vfrnav

# selinux
install -p -m 644 -D data/selinux/vfrnav.pp $RPM_BUILD_ROOT%{_datadir}/selinux/packages/%{name}/%{name}.pp

%if %{with webservice}
install -d $RPM_BUILD_ROOT/%{_libdir}/vfrnav
echo "CREATE TABLE IF NOT EXISTS credentials (username TEXT UNIQUE NOT NULL, passwdclear TEXT, passwdmd5 TEXT, salt INTEGER);" | sqlite3 $RPM_BUILD_ROOT/%{_sysconfdir}/vfrnav/autoroute.db
install -d $RPM_BUILD_ROOT/run/vfrnav/autoroute
%else
rm -f $RPM_BUILD_ROOT/lib/systemd/system/cfmuautoroute.service
rm -f $RPM_BUILD_ROOT/lib/systemd/system/cfmuautoroute.socket
rm -f $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/cfmuautoroute
%endif

%pre validatorservice
getent group vfrnav &>/dev/null || groupadd -r vfrnav
getent passwd vfrnav &>/dev/null || \
useradd -g vfrnav -d /var/lib/vfrnav -M -r -s /sbin/nologin \
    -c "Special user account to be used by vfrnav cfmuautoroute/cfmuvalidate services" vfrnav

%post
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%post selinux
/usr/sbin/semodule -i %{_datadir}/selinux/packages/%{name}/%{name}.pp >/dev/null 2>&1 || :

%postun selinux
if [ $1 -eq 0 ] ; then
    /usr/sbin/semodule -r %{name} >/dev/null 2>&1 || :
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%license COPYING
%doc AUTHORS ChangeLog INSTALL NEWS README TODO doc/flightdeck.pdf
%{_datadir}/applications/vfrnav.desktop
%{_datadir}/applications/flightdeck.desktop
%{_datadir}/applications/vfrairporteditor.desktop
%{_datadir}/applications/vfrairspaceeditor.desktop
%{_datadir}/applications/vfrnavaideditor.desktop
%{_datadir}/applications/vfrtrackeditor.desktop
%{_datadir}/applications/vfrwaypointeditor.desktop
%{_datadir}/applications/acftperf.desktop
%{_datadir}/icons/hicolor/32x32/apps/vfrnav.png
%{_datadir}/icons/hicolor/48x48/apps/vfrnav.png
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/vfrnav.png
%{_datadir}/%{name}/bluetooth.png
%{_datadir}/%{name}/BlankMap-World_gray.svg
%{_datadir}/%{name}/dbeditor.ui
%{_datadir}/%{name}/navigate.ui
%{_datadir}/%{name}/routeedit.ui
%{_datadir}/%{name}/prefs.ui
%{_datadir}/%{name}/acftperformance.ui
%{_datadir}/%{name}/cfmuvalidate.ui
%{_libdir}/libreoffice/share/registry/vfrnav.xcd
%{_libdir}/libvfrnav.so.0
%{_libdir}/libvfrnav.so.0.0.0
%{_bindir}/vfrnav
%{_bindir}/vfrnavaideditor
%{_bindir}/vfrwaypointeditor
%{_bindir}/vfrairwayeditor
%{_bindir}/vfrairporteditor
%{_bindir}/vfrairspaceeditor
%{_bindir}/vfrtrackeditor
%{_bindir}/vfrnavdb2xml
%{_bindir}/vfrnavfplan
%{_bindir}/vfrnavxml2db
%{_bindir}/vfrpdfmanip
%{_bindir}/acftperf
%{_bindir}/flightdeck
%{_bindir}/cfmuvalidate
%{_bindir}/cfmuautoroute
%{_bindir}/checkfplan
%dir %{_libexecdir}/%{name}
%{_libexecdir}/%{name}/cfmuvalidateserver
%{_libexecdir}/%{name}/web-extensions/libcfmuwebextension.so
%{_datadir}/%{name}/themes/gtk-3.0/flightdeck.css
%{_datadir}/%{name}/flightdeck.ui
%{_datadir}/%{name}/flightdeck/hbdhg.cfg
%{_datadir}/%{name}/flightdeck/hbpbx.cfg
%{_datadir}/%{name}/flightdeck/hbpho.cfg
%{_datadir}/%{name}/flightdeck/hbtda.cfg
%{_datadir}/%{name}/flightdeck/hbtdb.cfg
%{_datadir}/%{name}/flightdeck/hbtdc.cfg
%{_datadir}/%{name}/flightdeck/hbtdd.cfg
%{_datadir}/%{name}/flightdeck/sim.cfg
%{_datadir}/%{name}/aircraft/hbdhg.xml
%{_datadir}/%{name}/aircraft/hbpbx.xml
%{_datadir}/%{name}/aircraft/hbpho.xml
%{_datadir}/%{name}/aircraft/hbtda.xml
%{_datadir}/%{name}/aircraft/hbtdb.xml
%{_datadir}/%{name}/aircraft/hbtdc.xml
%{_datadir}/%{name}/aircraft/hbtdd.xml
%{_datadir}/%{name}/navlogtemplates/navlog.ods
%{_datadir}/%{name}/navlog.xml

%files utils
%defattr(-,root,root,-)
%{_bindir}/vfrdbdafif
%{_bindir}/vfrdbmapelementsdb
%{_bindir}/vfrdboptimizelabelplacement
%{_bindir}/vfrdbrebuildspatialindex
%{_bindir}/vfrdbsrtm30db
%{_bindir}/vfrdbsrtmwatermask
%{_bindir}/vfrgshhsimport
%{_bindir}/vfrdbsettopo30
%{_bindir}/vfrdbtopo30zerotiles
%{_bindir}/vfrdbtopo30bin
%{_bindir}/vfrdbupdategndelev
%{_bindir}/vfrdbcamelcase
%{_bindir}/vfrnavwmmconv
%{_bindir}/vfrnavwmmtest
%{_bindir}/vfrdbxplane
%{_bindir}/vfrdbcsv
%{_bindir}/vfrdbxplaneexport
%{_bindir}/flightdeckftdieeprog
%{_bindir}/vfrdbairwaydump
%{_bindir}/vfrdbeadimport
%{_bindir}/vfrdbaixmimport
%{_bindir}/vfrdbopenair
%{_bindir}/vfrdbfasimport
%{_bindir}/vfrradconvert
%{_bindir}/vfrdbukpostcodeimport
%{_bindir}/vfrnavfixdb
%{_bindir}/cfmusidstar
%{_bindir}/adrimport
%{_bindir}/adraupimport
%{_bindir}/adrquery
%{_bindir}/adrdbsync
%{_bindir}/aircraftopsperf
%{_bindir}/weatherfplan

%files wetterdl
%defattr(-,root,root,-)
%{_bindir}/wetterdl.py
%{_datadir}/applications/wetterdl.desktop
%{_datadir}/icons/hicolor/32x32/apps/wetterdl.png
%{_datadir}/icons/hicolor/48x48/apps/wetterdl.png

%files validatorservice
%defattr(-,root,root,-)
/lib/systemd/system/cfmuvalidate.service
/lib/systemd/system/cfmuvalidate.socket
%config(noreplace) %{_sysconfdir}/sysconfig/cfmuvalidate
%dir %attr(0755,vfrnav,vfrnav) /run/vfrnav
%dir %attr(0750,vfrnav,vfrnav) /run/vfrnav/validator
%dir %attr(0750,vfrnav,vfrnav) /var/lib/vfrnav

%files selinux
%{_datadir}/selinux/packages/%{name}/vfrnav.pp

%if %{with webservice}
%files webservice
%defattr(-,root,root,-)
/lib/systemd/system/cfmuautoroute@.service
/lib/systemd/system/cfmuautoroute@.socket
%config(noreplace) %{_sysconfdir}/sysconfig/cfmuautoroute
%dir %{_sysconfdir}/vfrnav
%config(noreplace) %attr(0660,vfrnav,apache) %{_sysconfdir}/vfrnav/autoroute.db
%dir %attr(0750,vfrnav,apache) /run/vfrnav/autoroute
%endif

%changelog
* Sat Jul 27 2013 Thomas Sailer <t.sailer@alumni.ethz.ch> - 20130727-1
- convert webservice from websockets to php and long poll

* Sun Jul 14 2013 Thomas Sailer <t.sailer@alumni.ethz.ch> - 20130627-1
- add webservice

* Wed Jun 13 2012 Thomas Sailer <t.sailer@alumni.ethz.ch> - 0.8-1
- update to 0.8: add flightdeck

* Wed Mar 25 2009 Thomas Sailer <t.sailer@alumni.ethz.ch> - 0.3-1
- move to gypsy, add airways

* Sat Feb  2 2008 Thomas Sailer <t.sailer@alumni.ethz.ch> - 0.1-2
- move utilities into their own subpackage

* Sat Aug 25 2007 Thomas Sailer <t.sailer@alumni.ethz.ch> - 0.1-1
- initial spec file

