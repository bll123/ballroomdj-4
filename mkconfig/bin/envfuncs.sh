#!/bin/sh
#
# Copyright 2010-2018 Brad Lanam Walnut Creek, CA, USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

setcflags () {
  CFLAGS=""
  if [ "$CFLAGS_ALL" != "" ]; then
    CFLAGS="$CFLAGS_ALL"
  else
    doappend CFLAGS " ${CFLAGS_OPTIMIZE}"
    doappend CFLAGS " ${CFLAGS_DEBUG}"
    doappend CFLAGS " ${CFLAGS_INCLUDE}"
    doappend CFLAGS " ${CFLAGS_USER}"
    doappend CFLAGS " ${CFLAGS_APPLICATION}"
    doappend CFLAGS " ${CFLAGS_COMPILER}"
    doappend CFLAGS " ${CFLAGS_SYSTEM}"
  fi
  export CFLAGS
}

setldflags () {
  LDFLAGS=""
  if [ "$LDFLAGS_ALL" != "" ]; then
    LDFLAGS="$LDFLAGS_ALL"
  else
    doappend LDFLAGS " ${LDFLAGS_OPTIMIZE}"
    doappend LDFLAGS " ${LDFLAGS_DEBUG}"
    doappend LDFLAGS " ${LDFLAGS_USER}"
    doappend LDFLAGS " ${LDFLAGS_APPLICATION}"
    doappend LDFLAGS " ${LDFLAGS_COMPILER}"
    doappend LDFLAGS " ${LDFLAGS_SYSTEM}"
  fi
  export LDFLAGS
}

setlibs () {
  LIBS=""
  if [ "$LDFLAGS_LIBS_ALL" != "" ]; then
    LIBS="$LDFLAGS_LIBS_ALL"
  else
    doappend LIBS " ${LDFLAGS_LIBS_USER}"
    doappend LIBS " ${LDFLAGS_LIBS_APPLICATION}"
    doappend LIBS " ${LDFLAGS_LIBS_SYSTEM}"
  fi
  export LIBS
}

