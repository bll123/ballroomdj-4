#!/bin/bash

LOCALEDIR=../../locale
TMPLDIR=../../templates
INSTDIR=../../install

cwd=$(pwd)
case ${cwd} in
  */src)
    cd po
    ;;
  */bdj4)
    cd src/po
    ;;
esac

function mksub {
  tmpl=$1
  tempf=$2
  locale=$3
  pofile=$4

  set -o noglob
  echo "-- Processing $tmpl"
  sedcmd=""
  while read -r line; do
    nl=$(echo $line |
      sed -e 's,^\.\.,,' -e 's,^,msgid ",' -e 's,$,",')
    xl=$(sed -n "\~^${nl}$~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    case $line in
      ..*)
        xl=$(echo "..$xl")
        ;;
    esac
    sedcmd+="-e '\~^${line}\$~ s~.*~${xl}~' "
  done < $tempf

  eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  set +o noglob
}

function mkhtmlsub {
  tmpl=$1
  tempf=$2
  locale=$3
  pofile=$4

  set -o noglob
  echo "-- Processing $tmpl"
  sedcmd=""
  while read -r line; do
    nl=$line
    case $nl in
      "")
        continue
        ;;
    esac
    xl=$(sed -n "\~msgid .${nl}.~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    sedcmd+="-e '\~value=\"${nl}\"~ s~value=\"${nl}\"~value=\"${xl}\"~' "
  done < $tempf

  eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  set +o noglob
}

function mkimgsub {
  tmpl=$1
  tempf=$2
  locale=$3
  pofile=$4

  set -o noglob
  echo "-- Processing $tmpl"
  sedcmd=""
  while read -r line; do
    nl=$line
    case $nl in
      "")
        continue
        ;;
    esac
    xl=$(sed -n "\~msgid .${nl}.~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    sedcmd+="-e '\~aria-label=\"${nl}\"~ s~aria-label=\"${nl}\"~aria-label=\"${xl}\"~' "
  done < $tempf

  eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  set +o noglob
}

TMP=temp.txt
CTMP=tempcomp.txt

echo "-- Creating .mo files"
for i in *.po; do
  j=$(echo $i | sed 's,\.po$,,')
  sj=$(echo $j | sed 's,\(..\).*,\1,')
  if [[ -d ${LOCALEDIR}/$sj ]]; then
    rm -rf ${LOCALEDIR}/$sj
  fi
  if [[ -h ${LOCALEDIR}/$sj ]]; then
    rm -f ${LOCALEDIR}/$sj
  fi
done

for i in *.po; do
  j=$(echo $i | sed 's,\.po$,,')
  sj=$(echo $j | sed 's,\(..\).*,\1,')
  mkdir -p ${LOCALEDIR}/$j/LC_MESSAGES
  msgfmt -c -o ${LOCALEDIR}/$j/LC_MESSAGES/bdj4.mo $i
  if [[ ! -d ${LOCALEDIR}/$sj && ! -h ${LOCALEDIR}/$sj ]]; then
    ln -s $j ${LOCALEDIR}/$sj
  fi
done

> $CTMP

while read -r line; do
  set $line
  pofile=$1
  shift
  langdesc=$*

  if [[ $pofile == "" ]]; then
    continue
  fi

  llocale=$(echo $pofile | sed 's,\.po$,,')
  # want the short locale
  locale=$(echo $llocale | sed 's,\(..\).*,\1,')

  desc=$(sed -n -e '1,1p' $pofile)
  desc=$(echo $desc | sed -e 's/^# == //')
  echo $desc >> $CTMP
  echo "..$llocale" >> $CTMP

  case $pofile in
    en*)
      continue
      ;;
  esac

  fn=${TMPLDIR}/dancetypes.txt
  sed -e '/^#/d' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/bdjconfig.txt.p
  # may also need pause msg and done msg in the future
  sed -n -e '/^QUEUE_NAME_[AB]/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/dances.txt
  sed -n -e '/^DANCE/ {n;p;}' $fn > $TMP
  sed -n -e '/^TYPE/ {n;p;}' $fn >> $TMP
  sed -n -e '/^SPEED/ {n;p;}' $fn >> $TMP
  sort -u $TMP > $TMP.n
  mv -f $TMP.n $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/ratings.txt
  sed -n -e '/^RATING/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/genres.txt
  sed -n -e '/^GENRE/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/levels.txt
  sed -n -e '/^LEVEL/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/status.txt
  sed -n -e '/^STATUS/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  for fn in ${TMPLDIR}/*.pldances; do
    sed -n -e '/^DANCE/ {n;p;}' $fn > $TMP
    sort -u $TMP > $TMP.n
    mv -f $TMP.n $TMP
    mksub $fn $TMP $locale $pofile
  done

  for fn in ${TMPLDIR}/*.pl; do
    sed -n -e '/^DANCERATING/ {n;p;}' $fn > $TMP
    sed -n -e '/^DANCELEVEL/ {n;p;}' $fn >> $TMP
    sort -u $TMP > $TMP.n
    mv -f $TMP.n $TMP
    mksub $fn $TMP $locale $pofile
  done

  for fn in ${TMPLDIR}/*.sequence; do
    sed -e '/^#/d' $fn > $TMP
    mksub $fn $TMP $locale $pofile
  done

  for fn in ${TMPLDIR}/*.html; do
    case $fn in
      *qrcode.html)
        continue
        ;;
      *mobilemq.html)
        # mobilemq has only the title string 'marquee'.
        # handle this later; nl doesn't change it
        continue
        ;;
    esac
    egrep 'value=' $fn |
        sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' > $TMP
    sort -u $TMP > $TMP.n
    mv -f $TMP.n $TMP
    mkhtmlsub $fn $TMP $locale $pofile
  done

  fn=${TMPLDIR}/fades.svg
  egrep 'aria-label=' $fn |
      sed -e 's,.*aria-label=",,' -e 's,".*,,' > $TMP
  sort -u $TMP > $TMP.n
  mv -f $TMP.n $TMP
  mkimgsub $fn $TMP $locale $pofile

  # these are used by the installer to locate the localized name
  # of the playlists.
  ATMP=${INSTDIR}/localized-auto.txt
  SRTMP=${INSTDIR}/localized-sr.txt
  QDTMP=${INSTDIR}/localized-qd.txt
  > $ATMP
  > $SRTMP
  > $QDTMP
  if [[ $pofile != en_US.po && $pofile != en_GB.po ]]; then
    for txt in automatic standardrounds queuedance; do
      ttxt=$txt
      if [[ $ttxt == queuedance ]]; then ttxt="QueueDance"; fi
      xl=$(sed -n "\~msgid .${ttxt}.~ {n;p;}" $pofile)
      case $xl in
        ""|msgstr\ \"\")
          continue
          ;;
      esac
      xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
      if [[ $xl == "" ]]; then
        xl=$txt
      fi
      eval $txt="\"$xl\""
    done

    cwd=$(pwd)

    echo $locale >> $ATMP
    echo "..${automatic}" >> $ATMP
    echo $locale >> $SRTMP
    echo "..${standardrounds}" >> $SRTMP
    echo $locale >> $QDTMP
    echo "..${queuedance}" >> $QDTMP

    cd ${TMPLDIR}/${locale}

    if [[ -f automatic.pl ]]; then
      mv -f automatic.pl "${automatic}.pl"
      mv -f automatic.pldances "${automatic}.pldances"
    fi

    if [[ -f standardrounds.pl ]]; then
      mv -f standardrounds.pl "${standardrounds}.pl"
      mv -f standardrounds.pldances "${standardrounds}.pldances"
      mv -f standardrounds.sequence "${standardrounds}.sequence"
    fi

    if [[ -f QueueDance.pl ]]; then
      mv -f QueueDance.pl "${queuedance}.pl"
      mv -f QueueDance.pldances "${queuedance}.pldances"
    fi

    for fn in *.html; do
      case $fn in
        mobilemq.html|qrcode.html)
          continue
          ;;
      esac
      sed -e "s/English/${langdesc}/" "$fn" > "$fn.n"
      mv -f "$fn.n" "$fn"
    done

    cd $cwd
  fi
done < complete.txt

mv -f $CTMP ${LOCALEDIR}/locales.txt

test -f $TMP && rm -f $TMP
test -f $CTMP && rm -f $CTMP
exit 0
