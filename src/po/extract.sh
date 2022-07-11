#!/bin/bash

cwd=$(pwd)
case ${cwd} in
  */po)
    cd ..
    ;;
esac

rm -f po/*~ po/old/*~ > /dev/null 2>&1
declare -A helptext
export helptext
export helpkeys=""
export POTFILE=bdj4.pot

. ../VERSION.txt
export VERSION
dt=$(date '+%F')
export dt

function mkpo {
  lang=$1
  out=$2
  xlator=$3
  dlang=$4
  elang=$5

  echo "-- $(date +%T) creating $out"
  if [[ -f ${out} && ${out} != en_GB.po ]]; then
    # re-use data from existing file
    mv -f ${out} ${out}.old
    > ${out}

    sed -n '1,/^$/ p' ${out}.old >> ${out}
    # "POT-Creation-Date: 2022-05-26"
    cdt=$(egrep '^"POT-Creation-Date:' bdj4.pot)
    # "PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE"
    # "PO-Revision-Date: 2022-05-27 14:09"
    sed -i \
      -e "s,PO-Revision-Date:.*,PO-Revision-Date: ${dt}\\\\n\"," \
      -e "s/: [0-9][0-9-]* [0-9][0-9:-]*/: ${dt}/" \
      ${out}
    sed -n '/^$/,$ p' bdj4.pot >> ${out}
  else
    # new file
    > ${out}
    echo "# == $dlang" >> ${out}
    echo "# -- $elang" >> ${out}
    sed -e '1,6 d' \
        -e "s/YEAR-MO-DA.*ZONE/${dt}/" \
        -e "s/: [0-9][0-9-]* [0-9][0-9:-]*/: ${dt}/" \
        -e "s/PACKAGE/ballroomdj-4/" \
        -e "s/VERSION/${VERSION}/" \
        -e "s/FULL NAME.*ADDRESS./${xlator}/" \
        -e "s/Bugs-To: /Bugs-To: brad.lanam.comp@gmail.com/" \
        -e "s/Language: /Language: ${lang}/" \
        -e "s,Language-Team:.*>,Language-Team: $elang," \
        -e "s/CHARSET/UTF-8/" \
        bdj4.pot >> ${out}
  fi
}

TMP=potemplates.c
> $TMP

ctxt="// CONTEXT: configuration file: dance type"
fn=../templates/dancetypes.txt
sed -e '/^#/d' -e 's,^,..,' -e "s,^,${ctxt}\n," $fn >> $TMP

ctxt="// CONTEXT: configuration file: dance"
fn=../templates/dances.txt
sed -n -e "/^DANCE/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP

ctxt="// CONTEXT: configuration file: rating"
fn=../templates/ratings.txt
sed -n -e "/^RATING/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP

ctxt="// CONTEXT: configuration file: genre"
fn=../templates/genres.txt
sed -n -e "/^GENRE/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP

ctxt="// CONTEXT: configuration file: dance level"
fn=../templates/levels.txt
sed -n -e "/^LEVEL/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP

ctxt="// CONTEXT: configuration file: status"
fn=../templates/status.txt
sed -n -e "/^STATUS/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP

fn=../templates/bdjconfig.txt.p
ctxt="// CONTEXT: configuration file: name of a music queue"
sed -n -e "/^QUEUE_NAME_[AB]/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP
echo "// CONTEXT: configuration file: The completion message displayed on the marquee when the playlist is finished." >> $TMP
sed -n -e '/^COMPLETEMSG/ {n;p}' $fn >> $TMP

ctxt="// CONTEXT: text from the HTML templates (buttons and labels)"
egrep 'value=' ../templates/*.html |
  sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMP
egrep '<p[^>]*>[A-Za-z][A-Za-z]*</p>' ../templates/*.html |
  sed -e 's,.*: *<,<,' -e 's,<[^>]*>,,g' -e 's,^ *,,' -e 's, *$,,' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMP

# names of playlist files
echo "// CONTEXT: The name of the 'automatic' playlist file" >> $TMP
echo "..automatic" >> $TMP
echo "// CONTEXT: The name of the 'standardrounds' playlist file" >> $TMP
echo "..standardrounds" >> $TMP
echo "// CONTEXT: The name of the 'queuedance' playlist file" >> $TMP
echo "..QueueDance" >> $TMP
echo "// CONTEXT: The name of the 'raffle songs' playlist file" >> $TMP
echo "..Raffle Songs" >> $TMP

# linux desktop shortcut
echo "// CONTEXT: tooltip for desktop icon" >> $TMP
egrep '^Comment=' ../install/bdj4.desktop |
  sed -e 's,Comment=,,' -e 's,^,..,' >> $TMP

sed -e '/^\.\./ {s,^\.\.,, ; s,^,_(", ; s,$,"),}' $TMP > $TMP.n
mv -f $TMP.n $TMP

echo "-- $(date +%T) extracting"
xgettext -s -d bdj4 \
    --language=C \
    --add-comments=CONTEXT: \
    --no-location \
    --keyword=_ \
    --flag=_:1:pass-c-format \
    */*.c \
    -p po -o ${POTFILE}

rm -f $TMP

cd po

awk -f extract-helptext.awk ${POTFILE} > ${POTFILE}.n
mv -f ${POTFILE}.n ${POTFILE}

mkpo en en_GB.po 'Automatically generated' 'English (GB)' english/gb
rm -f en_GB.po.old

mkpo nl nl_BE.po 'marimo' Nederlands dutch

#mkpo fr fr_FR.po 'unassigned' Français french
#mkpo fi fi_FI.po 'unassigned' Suomi finnish
#mkpo de de_DE.po 'unassigned' Deutsch german
#mkpo es es_ES.po 'unassigned' Español spanish
#mkpo zh zh_TW.po 'unassigned' "繁體中文" "chinese (TW)"
#mkpo zh zh_CN.po 'unassigned' "简体中文" "chinese (CN)"

echo "-- $(date +%T) updating translations from old .po files"
./lang-lookup.sh
echo "-- $(date +%T) creating english/us .po files"
awk -f mken_us.awk en_GB.po > en_US.po
echo "-- $(date +%T) finished"

exit 0
