#!/bin/bash
ver=1

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
    archtag=m1
    ;;
esac

pattern="bdj4-4.[0-9]*.[0-9]*-installer-macos-${archtag}*"

latest=""
for f in $pattern; do
  case $f in
    *\*)
      ;;
    *)
      if [[ $latest == "" ]];then
        latest=$f
      fi
      if [[ $f -nt $latest ]]; then
        latest=$f
      fi
      chmod a+rx $f
      xattr -d com.apple.quarantine $f > /dev/null 2>&1
      ;;
  esac
done

if [[ $latest != "" ]]; then
  echo ""
  echo "Do you want to start the installation package? "
  gr=$(getresponse)
  if [[ $gr == Y ]]; then
    ./${latest}
  fi
fi

exit 0
