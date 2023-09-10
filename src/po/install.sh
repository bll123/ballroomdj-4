#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

LOCALEDIR=../../locale
TMPLDIR=../../templates
INSTDIR=../../install

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

function mksub {
  tmpl=$1
  tempf=$2
  locale=$3
  pofile=$4

  set -o noglob
  echo "-- Processing $tmpl"
  sedcmd=""
  ok=F
  while read -r line; do
    nl=$(echo $line |
      sed -e 's,^\.\.,,' -e 's,^,msgid ",' -e 's,$,",')
    xl=$(sed -n "\~^${nl}$~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,' -e 's,\&,\\&,g')
    case $line in
      ..*)
        xl=$(echo "..$xl")
        ;;
    esac
    sedcmd+="-e '\~^${line}\$~ s~.*~${xl}~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  fi
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
  ok=F
  while read -r line; do
    nl=$line
    case $nl in
      "")
        continue
        ;;
    esac
    xl=$(sed -n "\~msgid \"${nl}\"$~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,' -e 's,\&,\\&amp;,g')
    sedcmd+="-e '\~value=\"${nl}\"~ s~value=\"${nl}\"~value=\"${xl}\"~' "
    sedcmd+="-e '\~alt=\"${nl}\"~ s~alt=\"${nl}\"~alt=\"${xl}\"~' "
    sedcmd+="-e '\~>${nl}</p>~ s~${nl}~${xl}~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  fi
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
  ok=F
  while read -r line; do
    nl=$line
    case $nl in
      "")
        continue
        ;;
    esac
    xl=$(sed -n "\~msgid \"${nl}\"$~ {n;p;}" $pofile)
    case $xl in
      ""|msgstr\ \"\")
        continue
        ;;
    esac
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    sedcmd+="-e '\~aria-label=\"${nl}\"~ s~aria-label=\"${nl}\"~aria-label=\"${xl}\"~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  fi
  set +o noglob
}

TMP=temp.txt

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

# used by the installer to locate the localized name
# of the playlists.
# also include other information that may useful for the locale
LOCALEDATA=${TMPLDIR}/localization.txt
> $LOCALEDATA
keycount=0
echo "# localization" >> $LOCALEDATA

while read -r line; do

  case $line in
    '#'*)
      continue
      ;;
  esac

  set $line
  pofile=$1
  shift
  iso6393code=$1
  shift
  langdesc=$*

  if [[ $pofile == "" ]]; then
    continue
  fi

  llocale=$(echo $pofile | sed 's,\.po$,,')
  # want the short locale
  locale=$(echo $llocale | sed 's,\(..\).*,\1,')

  for txt in automatic standardrounds queuedance; do
    ttxt=$txt
    if [[ $ttxt == queuedance ]]; then ttxt="QueueDance"; fi
    xl=$(sed -n "\~msgid \"${ttxt}\"$~ {n;p;}" $pofile)
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,')
    if [[ $xl == "" ]]; then
      xl=$ttxt
    fi
    eval $txt="\"$xl\""
  done

  cwd=$(pwd)

  echo KEY >> $LOCALEDATA
  echo "..$keycount" >> $LOCALEDATA
  echo LONG >> $LOCALEDATA
  echo "..$llocale" >> $LOCALEDATA
  echo SHORT >> $LOCALEDATA
  echo "..$locale" >> $LOCALEDATA
  echo ISO639-3 >> $LOCALEDATA
  echo "..$iso6393code" >> $LOCALEDATA
  echo DISPLAY >> $LOCALEDATA
  echo "..$langdesc" >> $LOCALEDATA
  echo AUTO >> $LOCALEDATA
  echo "..${automatic}" >> $LOCALEDATA
  echo STDROUNDS >> $LOCALEDATA
  echo "..${standardrounds}" >> $LOCALEDATA
  echo QDANCE >> $LOCALEDATA
  echo "..${queuedance}" >> $LOCALEDATA

  keycount=$(($keycount+1))

  # The english locales do not need any substitutions
  # none of the en_GB -> en_US substitutions are present
  # in the configuration files.
  # The en_US configuration files are different than
  # the base en_GB, and are pre-built.
  case $pofile in
    en*)
      continue
      ;;
  esac

  fn=${TMPLDIR}/dancetypes.txt
  sed -e '/^#/d' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  fn=${TMPLDIR}/bdjconfig.txt.p
  sed -n -e '/^COMPLETEMSG/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $locale $pofile

  for qn in 0 1 2 3; do
    fn=${TMPLDIR}/bdjconfig.q${qn}.txt
    sed -n -e '/^QUEUE_NAME/ {n;p;}' $fn > $TMP
    mksub $fn $TMP $locale $pofile
  done

  fn=${TMPLDIR}/dances.txt
  sed -n -e '/^DANCE/ {n;p;}' $fn > $TMP
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

  fn=${TMPLDIR}/itunes-stars.txt
  sed -n -e '/^[0-9]/ {n;p;}' $fn > $TMP
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
    grep -E 'value=' $fn |
        sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' > $TMP
    grep -E 'alt=' $fn |
        sed -e 's,.*alt=",,' -e 's,".*,,' -e '/^BDJ4$/ d' >> $TMP
    grep -E '<p[^>]*>[A-Za-z][A-Za-z]*</p>' $fn |
        sed -e 's, *<p[^>]*>,,' -e 's,</p>,,' >> $TMP
    sort -u $TMP > $TMP.n
    mv -f $TMP.n $TMP
    mkhtmlsub $fn $TMP $locale $pofile
  done

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
done < complete.txt

test -f $TMP && rm -f $TMP
exit 0
