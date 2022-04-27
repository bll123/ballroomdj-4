#!/bin/bash
#
# requirements: sshpass
#

echo "* Remember to install on both sourceforge and ballroomdj.org"

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

case $server in
  web.sourceforge.net)
    port=22
    project=ballroomdj4
    # ${remuser}@web.sourceforge.net:/home/project-web/${project}/htdocs
    wwwpath=/home/project-web/${project}/htdocs
    ;;
  ballroomdj.org|gentoo.com)
    wwwpath=/var/www/ballroomdj.org
    ;;
esac
ssh="ssh -p $port"
export ssh

echo -n "Remote Password: "
read -s SSHPASS
echo ""
export SSHPASS

echo "## copying files"
if [[ $server == ballroomdj.org ]]; then
  for f in bdj4register.php bdj4support.php marquee4.html marquee4.php; do
    sshpass -e rsync -e "$ssh" -aS \
        $f ${remuser}@${server}:${wwwpath}
  done
fi

exit 0
