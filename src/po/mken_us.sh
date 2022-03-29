#!/bin/bash

sed \
    -e 's,== English .GB.*,== English (US),' \
    -e 's,-- english/gb,-- english/us,' \
    en_GB.po > en_US.po

function modpo {
  pofile=$1

  start=F
  while read -r line; do
      case $line in
        "")
          start=T
          ;;
      esac

      if [[ $start == F ]]; then
        echo $line
        continue
      fi

      case $line in
        msgid*)
          nmid=$line
          echo $line
          ;;
        'msgstr ""')
          nline=$nmid
          nline=$(echo $nline | sed \
              -e 's,\([Cc]\)olour,\1olor,g' \
              -e 's,\([Oo]\)rganis,\1rganiz,g' \
              -e 's,LICENCE,LICENSE,g' \
              )
          if [[ "$nline" == "$nmid" ]]; then
            echo 'msgstr ""'
          else
            nline=$(echo $nline | sed -e 's/msgid/msgstr/')
            echo $nline
          fi
          ;;
        *)
          echo $line
          ;;
      esac
  done < $pofile > $pofile.n
  mv -f $pofile.n $pofile
}

set -o noglob
# en_GB will be the default
modpo en_US.po

exit 0
