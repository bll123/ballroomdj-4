#!/bin/bash
#
# console installation script
#

export newinstall=T
# the graphical installer will run this script with -guienabled
export guienabled=F
export targetdir=""
export unpackdir=""
export reinstall=F

LOG=""

if [[ ! -d install ||
    ! -f install/install-helpers.sh ]]; then
  echo "Unable to locate install directory."
  exit 1
fi

. install/install-helpers.sh
processcmdargs $@

targetsavedir="$HOME/.config/BDJ4"
targetsavefn="${targetsavedir}/installdir.txt"
test -d $targetsavedir || mkdir $targetsavedir
if [[ $targetdir == "" && -f $targetsavefn ]]; then
  targetdir=$(cat $targetsavefn)
fi

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

echo "-- Checking $targetdir."
if [[ -d $targetdir ]]; then
  if [[ -d $targetdir/bin && -f $targetdir/bin/bdj4 ]]; then
    newinstall=F
  fi

  if [[ $newinstall == T ]]; then
    echo "Error: $targetdir already exists."
    test -d "$unpackdir" && rm -rf "$unpackdir"
    exit 1
  fi
fi

if [[ $reinstall == T ]]; then
  newinstall=T
fi

echo "$targetdir" > "$targetsavefn"

echo "-- Creating directory structure."
./install/install-mkdirs.sh \
    -guienabled $guienabled \
    -unpackdir "$unpackdir" \
    -targetdir "$targetdir" \
    -newinstall $newinstall \
    -reinstall $reinstall

# copy the installation to the target directory

echo "-- Copying files."
tar -c -f - . | (cd "$targetdir"; tar -x -f -)

# at this point the target directory should exist, and
# it can be used.

echo "-- Checking target."
. ./VERSION.txt
INSTVERSION=$VERSION
INSTBUILD=$BUILD

cd "$targetdir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to change directories to $targetdir."
  test -d "$unpackdir" && rm -rf "$unpackdir"
  exit 1
fi

if [[ ! -f VERSION.txt ]]; then
  echo "Invalid install directory.  Something went wrong."
  test -d "$unpackdir" && rm -rf "$unpackdir"
  exit 1
fi

. ./VERSION.txt

if [[ $INSTVERSION != $VERSION ]]; then
  echo "Mismatched versions. Something went wrong."
  test -d "$unpackdir" && rm -rf "$unpackdir"
  exit 1
fi

# clean up old files that are no longer in use

if [[ -f install/install-cleanup.sh ]]; then
  echo "-- Cleaning up old files."
  ./install/install-cleanup.sh \
      -guienabled $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -newinstall $newinstall \
      -reinstall $reinstall
fi

# copy template files over to data directory if needed.

if [[ $newinstall == T && -f install/install-templates.sh ]]; then
  echo "-- Copying templates."
  ./install/install-templates.sh \
      -guienabled $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -newinstall $newinstall \
      -reinstall $reinstall
fi

# if the conversion script has not been run,
# run it now.  Do this step last after the
# template files have been copied.

if [[ $reinstall == T ]]; then
  test -f "install/convrun.txt" && rm -f "install/convrun.txt"
fi

if [[ $newinstall == T && \
    -f install/install-convert.sh && \
    ! -f "install/convrun.txt" ]]; then
  echo "-- Running conversion script"
  ./install/install-convert.sh \
      -guienabled $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -newinstall $newinstall \
      -reinstall $reinstall
fi

test -d "$unpackdir" && rm -rf "$unpackdir"
exit 0
