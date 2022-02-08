#!/bin/bash

function logmsg {
  if [[ $LOG == "" ]]; then
    return
  fi
}

function processcmdargs {
  while test $# -gt 0; do
    case $1 in
      -newinstall|--newinstall)
        newinstall=T
        ;;
      -gui|--gui)
        guienabled=T
        ;;
      -reinstall|--reinstall)
        reinstall=T
        ;;
      -targetdir|--targetdir)
        shift
        targetdir=$1
        ;;
      -unpackdir|--unpackdir)
        shift
        unpackdir=$1
        ;;
    esac
    shift
  done
}
