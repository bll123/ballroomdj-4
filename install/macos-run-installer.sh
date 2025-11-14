#!/bin/bash
#
# Copyright 2021-2025 Brad Lanam Pleasant Hill CA
#
ver=5

if [[ $1 == --version ]]; then
  echo ${ver}
  exit 0
fi

getresponse () {
  echo -n "[Y/n]: " > /dev/tty
  read answer
  case $answer in
    Y|y|yes|Yes|YES|"")
      answer=Y
      ;;
    *)
      answer=N
      ;;
  esac
  echo $answer
}

if [[ $(uname -s) != Darwin ]]; then
  echo "Not running on MacOS"
  exit 1
fi

arch=$(uname -m)
case $arch in
  x86_64)
    archtag=intel
    ;;
  arm64)
    # m1 is an old name and can be removed soon
    archtag="applesilicon"
    ;;
esac

if [[ -d /opt/local/lib && ! -f /opt/local/lib/libicui18n.dylib ]]; then
  echo "The latest macos-pre-install.sh script must be run."
  exit 1
fi

latest=""
pattern="bdj4-installer-macos-${archtag}-[a-z]*-4.[0-9]*.[0-9]*"

for f in $pattern; do
  if [[ -f $f ]]; then
    if [[ $latest == "" ]];then
      latest=$f
    fi
    if [[ $f -nt $latest ]]; then
      latest=$f
    fi
  fi
done

if [[ $latest != "" ]]; then
  chmod a+rx $latest
  # BDJ4 has no malware in it.
  xattr -d com.apple.quarantine $latest > /dev/null 2>&1
  echo ""
  echo " ( $latest )"
  echo "Do you want to start the installation package? "
  gr=$(getresponse)
  if [[ $gr == Y ]]; then
    ./${latest}
  fi
fi

exit 0
