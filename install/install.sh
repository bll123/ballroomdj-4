#!/bin/bash
#
# console installation script
#

LOG=""

if [[ ! -d install ||
    ! -f install/install-helpers.sh ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

. install/install-helpers.sh
processcmdargs

unpackdir=$(pwd)

systype=$(uname -s)

if [[ $targetdir == "" && $guienabled == F ]]; then
  echo "Enter the directory where BallroomDJ 4 should be installed."
  echo "Press 'Enter' to use the default."
  case ${systype} in
    Darwin)
      targetdir="$HOME/Applications/BallroomDJ4.app"
      ;;
    *)
      targetdir="$HOME/BDJ4"
      ;;
  esac

  echo "Installation Directory [$targetdir]"
  echo -n ": "
  read answer
  case $answer in
    "")
      ;;
    *)
      targetdir="$answer"
      ;;
  esac
fi

if [[ -d $targetdir ]]; then
  if [[ -d $targetdir/bin && -f $targetdir/bin/bdj4 ]]; then
    newinstall=F
  fi

  if [[ $newinstall == T ]]; then
    echo "Error: $targetdir already exists."
    exit 1
  fi
fi

./install/install-mkdirs.sh \
    -gui $guienabled \
    -unpackdir "$unpackdir" \
    -targetdir "$targetdir" \
    -new $newinstall

# copy the installation to the target directory

tar -c -f - . | (cd "$targetdir"; tar -x -f -)

# at this point the target directory should exist, and
# it can be used.

. ./VERSION.txt
INSTVERSION=$VERSION
INSTBUILD=$BUILD

cd "$targetdir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to change directories to $targetdir."
  exit 1
fi

if [[ ! -f VERSION.txt ]]; then
  echo "Invalid install directory.  Something went wrong."
  exit 1
fi

. ./VERSION.txt

if [[ $INSTVERSION != $VERSION ]]; then
  echo "Mismatched versions. Something went wrong."
  exit 1
fi

# clean up old files that are no longer in use

if [[ -f install/install-cleanup.sh ]]; then
  ./install/install-cleanup.sh \
      -gui $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -new $newinstall
fi

# copy template files over to data directory if needed.

if [[ -f install/install-templates.sh ]]; then
  ./install/install-templates.sh \
      -gui $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -new $newinstall
fi

# copy template files to http directory

if [[ -f install/install-http.sh ]]; then
  ./install/install-http.sh \
      -gui $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -new $newinstall
fi

# if the conversion script has not been run,
# run it now.  Do this step last after the
# template files have been copied.

if [[ -f install/install-http.sh ]]; then
  ./install/install-convert.sh \
      -gui $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -new $newinstall
fi

exit 0
