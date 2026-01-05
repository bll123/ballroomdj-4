#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

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

  # echo "-- $(date +%T) creating ${locale} ${tmpl}"
  set -o noglob
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
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,' -e 's,\&,\\&,g' -e "s,',!!!,g")
    case $line in
      ..*)
        xl=$(echo "..$xl")
        ;;
    esac
    sedcmd+="-e '\~^${line}\$~ s~.*~${xl}~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    sedcmd+="-e \"s,!!!,',g\""

    eval sed ${sedcmd} "$tmpl" > "${TMPLDIR}/${locale}/$(basename ${tmpl})"
  fi
  set +o noglob
}

LOCALEDIR=../../locale
TMPLDIR=../../templates

TMP=temp.txt

pofile=$1
shift
locale=$1
shift
slocale=$1

uselocale=${slocale}

if [[ $pofile == "" || $locale == en_GB || $locale == en_US ]]; then
  exit 0
fi

if [[ -d "${TMPLDIR}/${locale}" ]]; then
  uselocale=${locale}
fi
test -d "${TMPLDIR}/${uselocale}" || mkdir "${TMPLDIR}/${uselocale}"

# note that the extraction is also done in poexttmpl.sh

fn=${TMPLDIR}/audiosrc.txt
echo "..Podcast" > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/bdjconfig.txt.p
sed -n -e '/^COMPLETEMSG/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

for fn in ${TMPLDIR}/bdjconfig.q?.txt; do
  sed -n -e '/^QUEUE_NAME/ {n;p;}' $fn > $TMP
  mksub $fn $TMP $uselocale $pofile
done

fn=${TMPLDIR}/dances.txt
sed -n -e '/^DANCE/ {n;p;}' $fn > $TMP
sort -u $TMP > $TMP.n
mv -f $TMP.n $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/dancetypes.txt
sed -e '/^#/d' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/genres.txt
sed -n -e '/^GENRE/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/itunes-stars.txt
sed -n -e '/^[0-9]/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/levels.txt
sed -n -e '/^LEVEL/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/ratings.txt
sed -n -e '/^RATING/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

fn=${TMPLDIR}/status.txt
sed -n -e '/^STATUS/ {n;p;}' $fn > $TMP
mksub $fn $TMP $uselocale $pofile

# playlists

for fn in ${TMPLDIR}/*.pldances; do
  sed -n -e '/^DANCE/ {n;p;}' $fn > $TMP
  sort -u $TMP > $TMP.n
  mv -f $TMP.n $TMP
  mksub $fn $TMP $uselocale $pofile
done

for fn in ${TMPLDIR}/*.pl; do
  sed -n -e '/^DANCERATING/ {n;p;}' $fn > $TMP
  sed -n -e '/^DANCELEVEL/ {n;p;}' $fn >> $TMP
  sort -u $TMP > $TMP.n
  mv -f $TMP.n $TMP
  mksub $fn $TMP $uselocale $pofile
done

for fn in ${TMPLDIR}/*.sequence; do
  sed -e '/^#/d' $fn > $TMP
  mksub $fn $TMP $uselocale $pofile
done

test -f $TMP && rm -f $TMP
exit 0
