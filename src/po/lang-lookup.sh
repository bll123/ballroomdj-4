#!/bin/bash

export olddata

for pofile in *.po; do
  case $pofile in
    en*)
      continue
    ;;
  esac

  opofile=old/$pofile
  if [[ ! -f $opofile ]]; then
    pfx=$(echo $pofile | sed 's/\(..\).*/\1/')
    tmppo=$(echo old/$pfx*)
    if [[ -f $tmppo ]]; then
      opofile=$tmppo
    fi
  fi

  test -f $pofile.na && rm -f $pofile.na

  for oldpo in $pofile.old $opofile; do
    if [[ ! -f $oldpo ]]; then
      continue
    fi

    echo "   $(date +%T) processing $oldpo"
    if [[ -f $pofile.na ]]; then
      gawk -f lang-lookup.awk $oldpo $pofile.na > $pofile.tmp
    else
      gawk -f lang-lookup.awk $oldpo $pofile > $pofile.tmp
    fi
    mv -f $pofile.tmp $pofile.na
  done

  mv -f $pofile.na $pofile
  test -f $pofile.old && rm -f $pofile.old
done
