#!/bin/bash

sed \
    -e 's,== English .US.*,== English (GB),' \
    -e 's,-- english/us,-- english/gb,' \
    en_US.po > en_GB.po

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
          # some of these no longer exist within BDJ4
          echo $nline | egrep -q '([Cc]olor|[Oo]rganiz|LICENSE)'
          found=$?
          if [[ $pofile == en_GB.po ]]; then
            nline=$(echo $nline | sed \
                -e 's,\([Cc]\)olor,\1olour,g' \
                -e 's,\([Oo]\)rganiz,\1rganis,g' \
                -e 's,LICENSE,LICENCE,g' \
                )
          fi
          if [[ $found -ne 0 ]]; then
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
modpo en_GB.po
# since en_GB will be the default, modify the US file to
# have the specific strings changed.
modpo en_US.po

exit 0
