#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#

export olddata

for pofile in "$@"; do
  case $pofile in
    en*)
      continue
    ;;
  esac

  NPOFILE=$pofile.n
  TMPPOFILE=$pofile.tmp
  CURRPOFILE=$pofile.current
  DBGPOFILE=$pofile.debug

  test -f $NPOFILE && rm -f $NPOFILE
  test -f $DBGPOFILE && rm -f $DBGPOFILE

  for oldpo in $CURRPOFILE;  do
    if [[ ! -f $oldpo ]]; then
      continue
    fi

    # echo "   $(date +%T) processing $oldpo"
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
