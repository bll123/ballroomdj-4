#!/bin/bash

function getinfo () {
  desc=$1
  default=$2

  rval=$default
  echo -n "$desc [$default]: "
  read answer
  if [[ $answer != "" ]]; then
    rval=$answer
  fi
}

cwd=$(pwd)
case $cwd in
  *bdj4/src/utils)
    cd ../..
    ;;
  *bdj4/src)
    cd ..
    ;;
esac

if [[ ! -d templates || ! -d web ]]; then
  echo "Unknown directory."
  exit 1
fi

sed -e 's,^//WEB,,' \
  templates/mobilemq.html.dist \
  > web/marquee4.html

getinfo Server ballroomdj.org
server=$rval

case $server in
  web.sourceforge.net)
    wwwpath=/home/project-web/ballroomdj4/htdocs
    ;;
  ballroomdj.org)
    wwwpath=/var/www/ballroomdj.org
    ;;
esac

getinfo 'Remote User' bll123
remuser=$rval
getinfo Port 22
port=$rval
export ssh="ssh -p $port"

echo -n "Remote Password: "
read -s SSHPASS
echo ""
export SSHPASS

sshpass -e rsync -e "$ssh" -aSv \
  web/marquee4.* \
  ${remuser}@${server}:${wwwpath}
