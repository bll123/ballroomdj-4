#!/bin/bash


export olddata

function readolddata {
  pof=$1

  start=F
  while read line; do
    case $line in
      "")
        start=T
        ;;
    esac

    if [[ $start != T ]]; then
      continue
    fi

    case $line in
      msgid*)
        omid=$line
        ;;
      msgstr*)
        olddata[$omid]=$line
        ;;
      *)
        ;;
    esac

  done < $pof
}

for pofile in *.po; do
  if [[ $pofile == "en.po" ]]; then
    continue
  fi

  olpo=""
  if [[ -f old/$pofile ]]; then
    oldpo=old/$pofile
  else
    pfx=$(echo $pofile | sed 's/\(..\).*/\1/')
    oldpo=$(echo old/$pfx*)
  fi

  unset olddata
  declare -A olddata
  readolddata $oldpo

  echo "processing $pofile : $oldpo"

  start=F
  while read line; do
    case $line in
      "")
        start=T
        ;;
    esac

    if [[ $start == F ]]; then
      echo $line
      continue
    fi

    case $line in
      msgid*)
        nmid=$line
        echo $line
        ;;
      'msgstr ""')
        if [[ ${olddata[$nmid]} != "" ]]; then
          line=${olddata[$nmid]}
        fi
        echo $line
        ;;
      *)
        echo $line
        ;;
    esac

  done < $pofile > $pofile.n
  cp -f $pofile $pofile.bak
  mv -f $pofile.n $pofile

done
