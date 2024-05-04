#!/bin/bash
#
# Copyright 2021-2024 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src/po
cwd=$(pwd)

function mkhtmlsub {
  tmpl=$1
  outfn=$2
  tempf=$3
  pofile=$5

  set -o noglob
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
    xl=$(echo $xl | sed -e 's,^msgstr ",,' -e 's,"$,,' -e 's,\&,\\&amp;,g' -e "s,',!!!,g")
    sedcmd+="-e '\~value=\"${nl}\"~ s~value=\"${nl}\"~value=\"${xl}\"~' "
    sedcmd+="-e '\~alt=\"${nl}\"~ s~alt=\"${nl}\"~alt=\"${xl}\"~' "
    sedcmd+="-e '\~>${nl}</p>~ s~${nl}~${xl}~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    sedcmd+="-e \"s,!!!,',g\""
    eval sed ${sedcmd} "$tmpl" > "$outfn"
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
shift
langdesc="$*"

if [[ $pofile == "" || $locale == en_GB || $locale == en_US ]]; then
  exit 0
fi

test -d "${TMPLDIR}/${slocale}" || mkdir "${TMPLDIR}/${slocale}"

for fn in ${TMPLDIR}/*.html; do
  case $fn in
    *qrcode.html)
      continue
      ;;
    *mobilemq.html)
      # 2023-11-28: mobilemq change to not have any title
      continue
      ;;
  esac

  > $TMP
  grep -E 'value=' $fn |
      sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' >> $TMP
  grep -E 'alt=' $fn |
      sed -e 's,.*alt=",,' -e 's,".*,,' -e '/^BDJ4$/ d' >> $TMP
  grep -E '<p[^>]*>[A-Za-z][A-Za-z]*</p>' $fn |
      sed -e 's, *<p[^>]*>,,' -e 's,</p>,,' >> $TMP
  sort -u $TMP > $TMP.n
  mv -f $TMP.n $TMP

  outfn="${TMPLDIR}/${slocale}/$(basename ${fn})"

  mkhtmlsub $fn $outfn $TMP $pofile

  sed -e "s/English/${langdesc}/" "$outfn" > "$outfn.n"
  mv -f "$outfn.n" "$outfn"
done

test -f $TMP && rm -f $TMP
exit 0
