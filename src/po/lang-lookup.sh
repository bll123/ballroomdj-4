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

    if [[ -f $pofile.na ]]; then
      echo "   $(date +%T) processing $oldpo"
      gawk -f lang-lookup.awk $oldpo $pofile.na > $pofile.tb
      mv -f $pofile.tb $pofile.na
    else
      echo "   $(date +%T) processing $oldpo"
      gawk -f lang-lookup.awk $oldpo $pofile > $pofile.ta
      mv -f $pofile.ta $pofile.na
    fi

    mv -f $pofile.na $pofile
    test -f $pofile.old && rm -f $pofile.old
  done
done
