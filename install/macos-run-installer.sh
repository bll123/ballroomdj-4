#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#
ver=3

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
    archtaglist=intel
    ;;
  arm64)
    # m1 is an old name and can be removed soon
    archtaglist="applesilicon m1"
    ;;
esac

latest=""
for archtag in $archtaglist; do
  patternold="bdj4-4.[0-9]*.[0-9]*-installer-macos-${archtag}*"
  pattern="bdj4-installer-macos-${archtag}-4.[0-9]*.[0-9]*"

  for f in $patternold $pattern; do
    if [[ -f $f ]]; then
      if [[ $latest == "" ]];then
        latest=$f
      fi
      if [[ $f -nt $latest ]]; then
        latest=$f
      fi
    fi
  done
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
