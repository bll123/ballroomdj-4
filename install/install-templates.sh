#!/bin/bash

LOG=""

if [[ ! -d install ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

function docopy {
  from=$1
  to=$2
  todir=$3

  if [[ -f $f.${slocale} ]]; then
    from=$f.${slocale}
    to=$(echo $to | sed "s,\.${slocale},,")
  fi
  if [[ $newinstall == T || ! -f ${todir}/${to} ]]; then
    # do not overwrite existing files
    cp -f ${from} ${todir}/${to}
  fi
}

hostname=$(hostname)
if [[ $hostname == "" ]]; then
  hostname=$(uname -n)
fi

set $(./bin/bdj4locale)
slocale=$(echo $1 | tr 'A-Z' 'a-z')

for f in templates/*; do
  to=$(basename $f)
  case $f in
    *qrcode|qrcode.html)
      # these just stay where they are
      ;;
    *html-list.txt)
      ;;
    *.crt)
      ;;
    *bdj-flex-dark.html)
      to=bdj4remote.html
      docopy $f $to http
      ;;
    *mobilemq.html)
      docopy $f $to http
      ;;
    *.html)
      # the other .html files are alternates and don't need any handling
      ;;
    *.svg)
      docopy $f $to img
      ;;
    *bdjconfig.txt.[gpm]|*bdjconfig.txt.mp)
      # special handling
      case $f in
        *.g)
          todir=data
          to=$(echo $to | sed 's,\.g$,,')
          ;;
        *.p)
          todir=data/profiles
          to=$(echo $to | sed 's,\.p$,,')
          ;;
        *.m)
          todir=data/$hostname
          to=$(echo $to | sed 's,\.m$,,')
          ;;
        *.mp)
          todir=data/$hostname/profiles
          to=$(echo $to | sed 's,\.mp$,,')
          ;;
      esac
      docopy $f $to $todir
      ;;
    *.txt|*.sequence|*.pl|*.pldances)
      # everything else is a template for the data dir
      docopy $f $to data
      ;;
  esac
done

# other http files

for f in img/favicon.ico img/led_on.svg img/led_off.svg img/ballroomdj.svg \
    ; do
  cp -f $f http
done
for d in img/mrc; do
  cp -r $d http
done

exit 0
