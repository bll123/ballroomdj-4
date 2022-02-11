#!/bin/bash
#
# console installation script
#

function cleanInstall {
  cd "$unpackdir"
  while [[ ! -d bdj4-install ]]; do
    cd ..
  done
  test -d bdj4-install && rm -rf bdj4-install
}

export newinstall=T
# the graphical installer will run this script with -guienabled
export guienabled=F
export targetdir=""
export unpackdir=""
export topdir=""
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
  if [[ ! -d $targetdir ]]; then
    rm -f $targetsavefn
    targetdir=""
  fi
fi

currdir=$(pwd)
systype=$(uname -s)

if [[ $targetdir == "" && $guienabled == F ]]; then
  echo "Enter the directory where BallroomDJ 4 should be installed."
  echo "Press 'Enter' to use the default."
  case ${systype} in
    Darwin)
      targetdir="$HOME/Applications/BDJ4.app"
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

topdir=$targetdir
if [[ $systype == Darwin ]]; then
  topdir=$targetdir/Contents/MacOS
fi

if [[ -d "$topdir" ]]; then
  if [[ -d "$topdir/bin" && -f "$topdir/bin/bdj4" ]]; then
    newinstall=F
  fi

  if [[ $newinstall == T ]]; then
    echo "Error: $topdir already exists."
    cleanInstall
    exit 1
  fi
fi

if [[ $reinstall == T ]]; then
  newinstall=T
fi

echo "$targetdir" > "$targetsavefn"

# copy the installation to the target directory

echo "-- Copying files."

test -d "$targetdir" || mkdir -p "$targetdir"

cd "$unpackdir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to change directories to $unpackdir."
  cleanInstall
  exit 1
fi

tar -c -f - . | (cd "$targetdir"; tar -x -f -)

cd "$currdir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to change directories to $currdir."
  cleanInstall
  exit 1
fi

# at this point the target directory should exist, and
# it can be used.

echo "-- Checking target."
. ./VERSION.txt
INSTVERSION=$VERSION
INSTBUILD=$BUILD

cd "$topdir"
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to change directories to $topdir."
  cleanInstall
  exit 1
fi

if [[ ! -f VERSION.txt ]]; then
  echo "Invalid install directory.  Something went wrong."
  cleanInstall
  exit 1
fi

. ./VERSION.txt

if [[ $INSTVERSION != $VERSION ]]; then
  echo "Mismatched versions. Something went wrong."
  cleanInstall
  exit 1
fi

echo "-- Creating directory structure."
./install/install-mkdirs.sh \
    -guienabled $guienabled \
    -unpackdir "$unpackdir" \
    -targetdir "$targetdir" \
    -topdir "$topdir" \
    -newinstall $newinstall \
    -reinstall $reinstall

if [[ $systype == Darwin ]]; then
  ln -sf bin/bdj4 "$topdir/BDJ4"
fi

# clean up old files that are no longer in use

if [[ -f install/install-cleanup.sh ]]; then
  echo "-- Cleaning up old files."
  ./install/install-cleanup.sh \
      -guienabled $guienabled \
      -unpackdir "$unpackdir" \
      -targetdir "$targetdir" \
      -topdir "$topdir" \
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
      -topdir "$topdir" \
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
      -topdir "$topdir" \
      -newinstall $newinstall \
      -reinstall $reinstall
fi

cleanInstall
exit 0
