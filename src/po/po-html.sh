#!/bin/bash
#
# Copyright 2021-2026 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

LOCALEDIR=../../locale
TMPLDIR=../../templates

TMP=temp.txt

pofile=$1
shift
locale=$1
shift
slocale=$1
shift
langdesc="$*"

uselocale=${slocale}

if [[ $pofile == "" || $locale == en_GB || $locale == en_US ]]; then
  exit 0
fi

if [[ -d "${TMPLDIR}/${locale}" ]]; then
  uselocale=${locale}
fi
test -d "${TMPLDIR}/${uselocale}" || mkdir "${TMPLDIR}/${uselocale}"

> $TMP
for fn in ${TMPLDIR}/*.html; do
  case $fn in
    *qrcode.html)
      continue
      ;;
    *mobilemq.html)
      # 2023-11-28: mobilemq changed to not have any title
      continue
      ;;
  esac

  grep -E 'value=' $fn |
      sed -e 's,.*value=",,' \
          -e 's,".*,,' -e '/^100$/ d' \
          >> $TMP
  grep -E 'alt=' $fn |
      sed -e 's,.*alt=",,' \
          -e 's,".*,,' \
          -e '/^BDJ4$/ d' \
          >> $TMP
  grep -E '<p[^>]*>[A-Za-z][A-Za-z]*</p>' $fn |
      sed -e 's, *<p[^>]*>,,' \
          -e 's,</p>,,' \
          >> $TMP
done

sort -u $TMP > $TMP.n
mv -f $TMP.n $TMP

set -o noglob
sedcmd=""
while read -r nl; do
  case $nl in
    "")
      continue
      ;;
  esac
  tnl=$(echo ${nl} | sed -e 's,&amp;,\&,g')
  ctxt=""
  if [[ $tnl == Next ]]; then
    ctxt="-c Page"
  elif [[ $tnl == Queue ]]; then
    ctxt="-c Verb"
  fi
  xl=$(LC_MESSAGES=${locale}.UTF-8 TEXTDOMAINDIR=${LOCALEDIR} \
      gettext -s -d bdj4 ${ctxt} "$tnl")
  # any &amp; removed must go back in
  xl=$(echo ${xl} | sed -e 's,&,\\&amp;,g' -e "s,',!!!,g")
  sedcmd+="-e '\~value=\"${nl}\"~ s~value=\"${nl}\"~value=\"${xl}\"~' "
  sedcmd+="-e '\~alt=\"${nl}\"~ s~alt=\"${nl}\"~alt=\"${xl}\"~' "
  sedcmd+="-e '\~>${nl}</p>~ s~${nl}~${xl}~' "
done < $TMP

sedcmd+="-e \"s,!!!,',g\""

for fn in ${TMPLDIR}/*.html; do
  case $fn in
    *qrcode.html)
      continue
      ;;
    *mobilemq.html)
      # 2023-11-28: mobilemq changed to not have any title
      continue
      ;;
  esac

  outfn="${TMPLDIR}/${uselocale}/$(basename ${fn})"

  eval sed ${sedcmd} "$fn" > "$outfn"
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "== po-html.sh failed"
    echo "   sedcmd: $sedcmd"
    exit 1
  fi

  sed -e "s/English/${langdesc}/" "$outfn" > "$outfn.n"
  mv -f "$outfn.n" "$outfn"
done
set +o noglob

test -f $TMP && rm -f $TMP
exit 0
