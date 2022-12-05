#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

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

  NPOFILE=$pofile.n
  TMPPOFILE=$pofile.tmp
  OLDPOFILE=$pofile.old

  test -f $NPOFILE && rm -f $NPOFILE

  for oldpo in $OLDPOFILE $opofile; do
    if [[ ! -f $oldpo ]]; then
      continue
    fi

    echo "   $(date +%T) processing $oldpo"
    if [[ -f $NPOFILE ]]; then
      gawk -f lang-lookup.awk $oldpo $NPOFILE > $TMPPOFILE
    else
      gawk -f lang-lookup.awk $oldpo $pofile > $TMPPOFILE
    fi
    mv -f $TMPPOFILE $NPOFILE
  done

  mv -f $NPOFILE $pofile
  test -f $OLDPOFILE && rm -f $OLDPOFILE
done
