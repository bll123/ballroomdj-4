#!/bin/bash

function logmsg (
  if [[ $LOG == "" ]]; then
    return
  fi
)

function processcmdargs (
  newinstall=T
  guienabled=F
  targetdir=""
  unpackdir=""
  for arg in $@; do
    case $arg in
      -newinstall)
        newinstall=T
        ;;
      -gui)
        guienabled=T
        ;;
      -targetdir)
        targetdir=$arg
        ;;
      -unpackdir)
        unpackdir=$arg
        ;;
    esac
  done
)
