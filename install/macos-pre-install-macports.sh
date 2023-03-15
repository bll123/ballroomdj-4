#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
ver=9

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

function getresponse {
  echo -n "[Y/n]: " > /dev/tty
  read answer
  case $answer in
    Y|y|yes|Yes|YES|"")
      answer=Y
      ;;
    *)
      answer=N
      ;;
  esac
  echo $answer
}

function mkgtkpatch {
  outnm=$1

  cat > "$outnm" << '_HERE_'
diff --git a/gnome/gtk3/Portfile b/gnome/gtk3/Portfile
index d5fe438f929..0a0d781b7b7 100644
--- a/gnome/gtk3/Portfile
+++ b/gnome/gtk3/Portfile
@@ -6,12 +6,13 @@ PortGroup           xcodeversion 1.0
 PortGroup           active_variants 1.1
 PortGroup           compiler_blacklist_versions 1.0
 PortGroup           legacysupport 1.1
+PortGroup           meson 1.0

 name                gtk3
 conflicts           gtk3-devel
 set my_name         gtk3
-version             3.24.34
-revision            2
+version             3.24.37
+revision            0
 epoch               1

 set proj_name       gtk+
@@ -33,10 +34,9 @@ distname            ${proj_name}-${version}
 dist_subdir         ${my_name}
 use_xz              yes
 master_sites        gnome:sources/${proj_name}/${branch}/
-
-checksums           rmd160  2060a89575f9adf938bf91e4f06935ea619f7577 \
-                    sha256  dbc69f90ddc821b8d1441f00374dc1da4323a2eafa9078e61edbe5eeefa852ec \
-                    size    21587592
+checksums           rmd160  afab13f415e5923bb185d923f3a37734e0f346d7 \
+                    sha256  6745f0b4c053794151fd0f0e2474b077cccff5f83e9dd1bf3d39fe9fe5fb7f57 \
+                    size    12401196

 minimum_xcodeversions {9 3.1}

@@ -59,9 +59,7 @@ depends_run         port:shared-mime-info \
 # darwin 10 and earlier requires legacy support for O_CLOEXEC
 legacysupport.newest_darwin_requires_legacy 10

-# use autoreconf to deal with dependency tracking issues in configure
-use_autoreconf      yes
-autoreconf.args     -fvi
+patchfiles         patch-meson_build.diff

 # gtk3 +quartz uses instancetype which is not available
 # before approximately Xcode 4.6 (#49391)
@@ -84,10 +82,10 @@ if {${universal_possible} && [variant_isset universal]} {
         lappend merger_destroot_args(${arch})  CC_FOR_BUILD='${configure.cc} -arch ${arch}'
     }
 } else {
-    build.args-append       CC="${configure.cc} ${configure.cc_archflags}" \
-                            CC_FOR_BUILD="${configure.cc} ${configure.cc_archflags}"
-    destroot.args-append    CC="${configure.cc} ${configure.cc_archflags}" \
-                            CC_FOR_BUILD="${configure.cc} ${configure.cc_archflags}"
+#     build.args-append       CC="${configure.cc} ${configure.cc_archflags}" \
+#                             CC_FOR_BUILD="${configure.cc} ${configure.cc_archflags}"
+#     destroot.args-append    CC="${configure.cc} ${configure.cc_archflags}" \
+#                             CC_FOR_BUILD="${configure.cc} ${configure.cc_archflags}"
 }

 pre-configure {
@@ -104,23 +102,23 @@ configure.cppflags-append \
 configure.cflags-append \
                     -fstrict-aliasing

-configure.args      --enable-static \
-                    --disable-glibtest \
-                    --enable-introspection \
-                    --disable-wayland-backend \
-                    --disable-schemas-compile \
-                    gio_can_sniff=yes
-
-build.args-append   V=1 \
-                    CPP_FOR_BUILD="${configure.cpp}"
+configure.args      -Dgtk_doc=false \
+                    -Dman=true \
+                    -Dintrospection=true \
+                    -Ddemos=false \
+                    -Dexamples=false \
+                    -Dtests=false \
+                    -Dprofiler=false

+# almost all tests failing??
 test.run            yes
-test.target         check
+
+destroot.post_args-append -v

 post-destroot {
     set docdir ${prefix}/share/doc/${name}
     xinstall -d ${destroot}${docdir}
-    xinstall -m 644 -W ${worksrcpath} AUTHORS COPYING HACKING NEWS README \
+    xinstall -m 644 -W ${worksrcpath} CONTRIBUTING.md COPYING NEWS README.md \
         ${destroot}${docdir}

     # avoid conflict with the gtk-update-icon-cache installed by gtk2
@@ -146,7 +144,7 @@ platform darwin {
         }

         # https://trac.macports.org/ticket/63151
-        configure.args-append --disable-dependency-tracking
+#         configure.args-append --disable-dependency-tracking
     }

     if {${os.major} <= 10} {
@@ -157,7 +155,7 @@ platform darwin {
     }
     if {${os.major} <= 12} {
         # requires cups 1.7
-        configure.args-append --disable-cups
+        configure.args-append -Dprint_backends=file,lpr,test
     }
 }

@@ -240,7 +238,8 @@ variant quartz conflicts x11 {
     require_active_variants path:lib/pkgconfig/cairo.pc:cairo quartz
     require_active_variants path:lib/pkgconfig/pango.pc:pango quartz

-    configure.args-append   --enable-quartz-backend
+    configure.args-append  -Dx11_backend=false \
+                           -Dquartz_backend=true
 }

 variant x11 conflicts quartz {
@@ -256,10 +255,9 @@ variant x11 conflicts quartz {
                             port:xorg-libXfixes \
                             port:at-spi2-atk

-    configure.args-append   --enable-xinerama \
-                            --x-include=${prefix}/include \
-                            --x-lib=${prefix}/lib \
-                            --enable-x11-backend
+    configure.args-append  -Dx11_backend=true \
+                           -Dquartz_backend=false \
+                           -Dxinerama=yes
 }

 if {![variant_isset quartz]} {
_HERE_
}

function mkgtkmesonpatch {
  outnm=$1

  cat > $outnm << '_HERE_'
--- meson.build.orig	2023-01-09 13:44:54.000000000 -0500
+++ meson.build	2023-01-09 13:45:30.000000000 -0500
@@ -158,7 +158,7 @@

 if os_darwin
   wayland_enabled = false
-  x11_enabled = false
+#  x11_enabled = false
 else
   quartz_enabled = false
 endif
_HERE_
}



if [[ $(uname -s) != Darwin ]]; then
  echo "Not running on MacOS"
  exit 1
fi

echo ""
echo "This script uses the 'sudo' command to run various commands"
echo "in a privileged state.  You will be required to enter your"
echo "password."
echo ""
echo "For security reasons, this script should be reviewed to"
echo "determine that your password is not mis-used and no malware"
echo "is installed."
echo ""
echo "Continue? "
gr=$(getresponse)
if [[ $gr != Y ]]; then
  exit 1
fi

vers=$(sw_vers -productVersion)

xcode-select -p >/dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to locate MacOS Command Line Tools"
  echo "Attempting to bootstrap install of MacOS Command Line Tools"
  make > /dev/null 2>&1
  echo ""
  echo "After MacOS Command Lines Tools has finished installing,"
  echo "run this script again."
  echo ""
  exit 0
fi

# not sure if there's a way to determine the latest python version
# this needs to be fixed.

# 2023-3-10 m1-ventura still has a dependency on python310 for glib2
oldpyverlist="310"
pyver=311

skipmpinst=F
case $vers in
  1[456789]*)
    mp_os_nm=$vers
    mp_os_vers=$vers
    echo "This script has no knowledge of this version of MacOS (too new)."
    echo "If macports is already installed, the script can continue."
    echo "Continue? "
    gr=$(getresponse)
    if [[ $gr != Y ]]; then
      exit 1
    fi
    skipmpinst=T
    ;;
  13*)
    mp_os_nm=Ventura
    mp_os_vers=13
    ;;
  12*)
    mp_os_nm=Monterey
    mp_os_vers=12
    ;;
  11*)
    mp_os_nm=BigSur
    mp_os_vers=11
    ;;
  10.15*)
    mp_os_nm=Catalina
    mp_os_vers=10.15
    ;;
  10.14*)
    mp_os_nm=Mojave
    mp_os_vers=10.14
    ;;
  *)
    echo "BallroomDJ 4 cannot be installed on this version of MacOS."
    echo "This version of MacOS is too old."
    echo "MacOS Mojave and later are supported."
    exit 1
    ;;
esac

if [[ $skipmpinst == F ]]; then
  mp_installed=F
  if [[ -d /opt/local && \
      -d /opt/local/share/macports && \
      -f /opt/local/bin/port ]]; then
    mp_installed=T
  fi

  if [[ $mp_installed == F ]]; then
    echo "-- MacPorts is not installed"

    url=https://github.com/macports/macports-base/releases
    # find the current version
    mp_tag=$(curl --include --head --silent \
      ${url}/latest |
      grep '^.ocation:' |
      sed -e 's,.*/,,' -e 's,\r$,,')
    mp_vers=$(echo ${mp_tag} | sed -e 's,^v,,')

    url=https://github.com/macports/macports-base/releases/download
    pkgnm=MacPorts-${mp_vers}-${mp_os_vers}-${mp_os_nm}.pkg
    echo "-- Downloading MacPorts"
    curl -JOL ${url}/${mp_tag}/${pkgnm}
    echo "-- Installing MacPorts using sudo"
    sudo installer -pkg ${pkgnm} -target /Applications
    rm -f ${pkgnm}
  fi
fi

PATH=$PATH:/opt/local/bin

sudo -v

echo "-- Running MacPorts 'port selfupdate' with sudo"
sudo port selfupdate

sudo -v

TLOC=${TMPDIR:-/var/tmp}
TFN="$TLOC/bdj4-pre-inst.tmp"
VARIANTSCONF=/opt/local/etc/macports/variants.conf

grep -- "^-x11 +no_x11 +quartz" $VARIANTSCONF > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "-- Updating variants.conf"
  cp $VARIANTSCONF "$TFN"
  echo "-x11 +no_x11 +quartz" >> "$TFN"
  sudo cp "$TFN" $VARIANTSCONF
  rm -f "$TFN"
fi

GTKPORTDIR=/opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports/gnome/gtk3
GTKPORTFILE=${GTKPORTDIR}/Portfile
GTKFILEDIR=${GTKPORTDIR}/files
# version             3.24.37
gtkvers=$(grep '^version' $GTKPORTFILE | sed 's/^version *//')
if [[ $gtkvers == "3.24.34" ]]; then
  echo "-- Patching gtk Portfile"
  mkgtkpatch "$TFN"
  sudo patch -p0 $GTKPORTFILE < "$TFN"
  mkgtkmesonpatch "$TFN"
  sudo cp "$TFN" $GTKFILEDIR/patch-meson_build.diff
  rm -f "$TFN"
fi

echo "-- Running MacPorts 'port upgrade outdated' with sudo"
sudo port upgrade outdated

sudo -v

echo "-- Installing packages needed by BDJ4"
sudo port -N install \
    python${pyver} \
    py${pyver}-pip \
    py${pyver}-wheel \
    curl \
    curl-ca-bundle \
    librsvg \
    taglib \
    glib2 +quartz \
    gtk3 +quartz \
    adwaita-icon-theme \
    ffmpeg +nonfree -x11
sudo -v
sudo port select --set python python${pyver}
sudo port select --set python3 python${pyver}
sudo port select --set pip pip${pyver}
sudo port select --set pip3 pip${pyver}

echo "-- Cleaning up old MacPorts files"
for pyver in $oldpyverlist; do
  sudo port uninstall -N --follow-dependents python${pyver}
done

if [[ -z "$(port -q list inactive)" ]]; then
  sudo port reclaim --disable-reminders
  sudo port -N reclaim
fi

sudo -k

# look for the macos-run-installer script

pattern="macos-run-installer-v[0-9]*.sh"

latest=""
for f in $pattern; do
  if [[ -f $f ]]; then
    chmod a+rx $f
    if [[ $latest == "" ]];then
      latest=$f
    fi
    if [[ $f -nt $latest ]]; then
      latest=$f
    fi
  fi
done

if [[ -f $latest ]]; then
  bash ${latest}
fi

exit 0
