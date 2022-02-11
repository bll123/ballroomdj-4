#!/bin/bash

LOG=""

export newinstall
export guienabled
export targetdir
export unpackdir
export topdir
export reinstall

if [[ ! -d install ||
    ! -f install/install-helpers.sh ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

. install/install-helpers.sh
processcmdargs $@

if [[ $targetdir == "" ]]; then
  echo "No target directory specified."
  exit 1
fi

cwd=$(pwd)

if [[ $topdir != $cwd ]]; then
  echo "Working directory is not $topdir."
  exit 1
fi

if [[ ! -f "install/convrun.txt" ]]; then
  if [[ $guienabled == T ]]; then
    ./bin/bdj4 --converter
  else
    if [[ -f bin/bdj4locatebdj3 ]]; then
      if [[ ! -f install/bdj3location.txt ]]; then
        bdj3path=$(./bin/bdj4locatebdj3)
        echo "Enter the directory where BallroomDJ 3 is installed."
        echo "Press 'Enter' to use the default."
        echo "If this is a new BallroomDJ 4 installation, enter 'none'."
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
        echo "bdj3path=\"${bdj3path}\"" > install/bdj3location.txt
      fi

      . ./install/bdj3location.txt
      case ${bdj3path} in
        none|"'none'"|None|NONE)
          ;;
        *)
          ./conv/cvtall.sh "$bdj3path"
          ;;
      esac
    fi
  fi
fi
