#!/bin/bash

export olddata

function readolddata {
  pof=$1

  start=F
  while read -r line; do
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
        omid="$line"
        ;;
      msgstr*)
        olddata[$omid]="$line"
        ;;
      *)
        ;;
    esac

  done < $pof
}

for pofile in *.po; do
  set +o noglob
  case $pofile in
    en*)
      continue
    ;;
  esac

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

  set -o noglob
  start=F
  while read -r line; do
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
        else
          tnmid=$(echo $nmid | sed 's,"$,:",')
          if [[ ${olddata[${tnmid}]} != "" ]]; then
            line=${olddata[${tnmid}]}
            line=$(echo $line | sed -e 's,:"$,",')
          fi
        fi
        echo $line
        ;;
      *)
        echo $line
        ;;
    esac
  done < $pofile > $pofile.n

  mv -f $pofile.n $pofile
done
