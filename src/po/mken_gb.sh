#!/bin/bash

sed \
    -e 's,== English .US.*,== English (GB),' \
    -e 's,-- english/us,-- english/gb,' \
    en_US.po > en_GB.po

set -o noglob
start=F
pofile=en_GB.po
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
        nline=$(echo $nline | sed \
            -e 's,\([Cc]\)olor,\1olour,g' \
            -e 's,\([Oo]\)rganiz,\1rganis,g' \
            -e 's,LICENSE,LICENCE,g' \
            )
        if [[ $nline == $nmid ]]; then
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

mv -f $pofile.n en_GB.po
exit 0
