#!/bin/bash

LOG=""

if [[ ! -d install ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

guienabled=T

while test $# -gt 0; do
  case $1 in
    --guienabled)
      shift
      guienabled=$1
      shift
      ;;
    *)
      shift
      ;;
  esac
done


if [[ $guienabled == T ]]; then
  ./bin/bdj4 --converter
else
  if [[ -f bin/bdj4locatebdj3 ]]; then
    if [[ ! -f install/bdj3location.txt ]]; then
      bdj3path=$(./bin/bdj4locatebdj3)
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
      echo "bdj3path=\"${bdj3path}\"" > install/bdj3location.txt
    fi

    . ./install/bdj3location.txt
    case ${bdj3path} in
      none|"'none'"|None|NONE)
        ;;
      *)
        ./conv/cvtall.sh "$bdj3path" "$datatopdir"
        ;;
    esac
  fi
fi
