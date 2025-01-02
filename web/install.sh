#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
# requirements: sshpass
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done

# pkgnm.sh expects to be in the top level dir
. ./src/utils/pkgnm.sh
vers=$(pkgcurrvers)

cd web
cwd=$(pwd)

echo "* Remember to install on both sourceforge and ballroomdj.org"

TMPMAIN=bdj4tmp.html
TMP=tmpweb
TMPIMG=tmpweb/img

test -d $TMP && rm -rf $TMP
mkdir -p $TMPIMG

tserver=web.sourceforge.net
echo -n "Server [$tserver]: "
read server
if [[ $server == "" ]]; then
  server=$tserver
fi

tremuser=bll123
echo -n "User [$tremuser]: "
read remuser
if [[ $remuser == "" ]]; then
  remuser=$tremuser
fi

tport=22
echo -n "Port [$tport]: "
read port
if [[ $port == "" ]]; then
  port=$tport
fi

ttestweb=N
testpath=""
echo -n "Test [$ttestweb]: "
read testweb
if [[ $testweb == "" ]]; then
  testweb=$ttestweb
fi
if [[ $testweb == Y || $testweb == y ]]; then
  testpath="/test"
fi

projectb=""
wwwpathb=""
case $server in
  web.sourceforge.net)
    port=22
    project=ballroomdj4
    projectb=ballroomdj
    # ${remuser}@web.sourceforge.net:/home/project-web/${project}/htdocs
    wwwpath=/home/project-web/${project}/htdocs
    wwwpathb=/home/project-web/${projectb}/htdocs
    ;;
  ballroomdj.org|gentoo.com)
    wwwpath=/var/www/ballroomdj.org
    ;;
esac
ssh="ssh -p $port"
export ssh

echo -n "Version [$vers]: "
read tvers
if [[ $tvers != "" ]]; then
  vers=$tvers
fi

echo -n "Remote Password: "
read -s SSHPASS
echo ""
export SSHPASS

echo "## copying files"
if [[ $server == ballroomdj.org ]]; then
  cp -pf bdj4marquee.php bdj4register.php bdj4report.php \
      bdj4support.php bdj4tester.php $TMP
  cp -pf ../templates/mobilemq.html $TMP/bdj4marquee.html
fi

mkdir -p $TMP${testpath}
cp -pf bdj4.css $TMP${testpath}
for fn in bdj4.html.*; do
  case $fn in
    *~)
      continue
      ;;
    *.orig)
      continue
      ;;
  esac
  lang=$(echo ${fn} | sed -e 's,.*\.,,')
  sed "s/#VERSION#/${vers}/g" ${fn} > $TMPMAIN
  cp -pf $TMPMAIN $TMP${testpath}/index.html.${lang}
done
cp -pf ../http/ballroomdj4.svg $TMPIMG/ballroomdj4.svg
cp -pf ../img/menu-base.svg $TMPIMG/menu.svg
cp -pf ../img/bdj4_icon.png $TMPIMG/bdj4_icon.png

ifn=bdj4_icon
inkscape ../img/${ifn}.svg -w 16 -h 16 -o ${ifn}-16.png > /dev/null 2>&1
icotool -c -o favicon.ico ${ifn}-16.png
mv -f favicon.ico $TMP
rm -f ${ifn}-16.png > /dev/null 2>&1


if [[ $server == web.sourceforge.net ]]; then
  cp -pf htaccess.sf $TMP/.htaccess
fi

cd $TMP
# never use --delete here
# the wikiimg/ directory is also underneath htdocs
sshpass -e rsync -v -e "$ssh" -aS \
    . \
    ${remuser}@${server}:${wwwpath}
if [[ ${wwwpathb} != "" ]]; then
  sshpass -e rsync -v -e "$ssh" -aS \
      . \
      ${remuser}@${server}:${wwwpathb}
fi
cd ..

test -d $TMP && rm -rf $TMP
test -f $TMPMAIN && rm -f $TMPMAIN

exit 0
