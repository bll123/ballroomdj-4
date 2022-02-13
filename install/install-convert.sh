#!/bin/bash

LOG=""

set -x

if [[ ! -d install ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

guienabled=T
newinstall=T
reinstall=F
datatopdir=""

while test $# -gt 0; do
  case $1 in
    --guienabled)
      shift
      guienabled=$1
      shift
      ;;
    --datatopdir)
      shift
      datatopdir=$1
      shift
      ;;
    --reinstall)
      shift
      reinstall=$1
      shift
      ;;
    --newinstall)
      shift
      newinstall=$1
      shift
      ;;
    *)
      shift
      ;;
  esac
done

if [[ $datatopdir == "" ]]; then
  echo "ERROR: Something went wrong (no datatopdir)."
  exit 1
fi

bdj3locfn=install/bdj3location.txt

if [[ $guienabled == T ]]; then
  ./bin/bdj4 --converter
else
  if [[ -f bin/bdj4locatebdj3 ]]; then
    if [[ $reinstall == T || ! -f ${bdj3locfn} ]]; then
      if [[ -f ${bdj3locfn} ]]; then
        . ${bdj3locfn}
      else
        bdj3path=$(./bin/bdj4locatebdj3)
      fi
      echo "Enter the directory where BallroomDJ 3 is installed."
      echo "Press 'Enter' to use the default."
      echo "If this is a new BDJ4 installation, enter 'none'."
      echo "BallroomDJ 3 Directory [$bdj3path]"
      echo -n ": "
      read answer
      case $answer in
        "")
          ;;
        *)
          bdj3path=$answer
          ;;
      esac
      echo "bdj3path=\"${bdj3path}\"" > ${bdj3locfn}
    fi

    . ${bdj3locfn}
    case ${bdj3path} in
      none|"'none'"|None|NONE)
        ;;
      *)
        ./conv/cvtall.sh "$bdj3path" "$datatopdir"
        ;;
    esac
  fi
fi
