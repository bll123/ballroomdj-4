#!/bin/bash
#
# Copyright 2023-2026 Brad Lanam Pleasant Hill CA
#

function mkhtmlsub {
  outdir=$1
  tmpl=$2
  tempf=$3
  locale=$4
  pofile=$5
  outfile=$6

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
    sedcmd+="-e '\~content=\"${nl}\"~ s~content=\"${nl}\"~content=\"${xl}\"~' "
    sedcmd+="-e '\~value=\"${nl}\"~ s~value=\"${nl}\"~value=\"${xl}\"~' "
    sedcmd+="-e '\~alt=\"${nl}\"~ s~alt=\"${nl}\"~alt=\"${xl}\"~' "
    sedcmd+="-e '\~>${nl}</p>~ s~${nl}~${xl}~' "
    sedcmd+="-e '\~>${nl}</a>~ s~${nl}~${xl}~' "
    sedcmd+="-e '\~>${nl}</span>~ s~${nl}~${xl}~' "
    sedcmd+="-e '\~>${nl}</li>~ s~${nl}~${xl}~' "
    sedcmd+="-e '\~>${nl}</title>~ s~${nl}~${xl}~' "
    ok=T
  done < $tempf

  if [[ $ok == T ]]; then
    eval sed ${sedcmd} "$tmpl" > ${outfile}
  fi
  set +o noglob
}

