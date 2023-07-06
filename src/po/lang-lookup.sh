#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

export olddata

for pofile in "$@"; do
  case $pofile in
    en*)
      continue
    ;;
  esac

  bdj3pofile=old/$pofile
  if [[ ! -f $bdj3pofile ]]; then
    pfx=$(echo $pofile | sed 's/\(..\).*/\1/')
    tmppo=$(echo old/$pfx*)
    if [[ -f $tmppo ]]; then
      bdj3pofile=$tmppo
    fi
  fi

  NPOFILE=$pofile.n
  TMPPOFILE=$pofile.tmp
  CURRPOFILE=$pofile.current
  DBGPOFILE=$pofile.debug

  test -f $NPOFILE && rm -f $NPOFILE
  test -f $DBGPOFILE && rm -f $DBGPOFILE

  for oldpo in $CURRPOFILE $bdj3pofile; do
    if [[ ! -f $oldpo ]]; then
      continue
    fi

    echo "   $(date +%T) processing $oldpo"
    if [[ -f $NPOFILE ]]; then
      gawk -f lang-lookup.awk $oldpo $NPOFILE > $TMPPOFILE 2>>$DBGPOFILE
    else
      gawk -f lang-lookup.awk $oldpo $pofile > $TMPPOFILE 2>>$DBGPOFILE
    fi
    mv -f $TMPPOFILE $NPOFILE
  done

  mv -f $NPOFILE $pofile
  test -f $CURRPOFILE && rm -f $CURRPOFILE
  if [[ ! -s $DBGPOFILE ]]; then
    rm -f $DBGPOFILE
  fi
done
