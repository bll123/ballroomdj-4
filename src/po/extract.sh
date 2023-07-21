#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

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

  if [[ $CHANGED == F ]]; then
    hg diff ${out} > ${POTFILE}.t
    if [[ ! -s ${POTFILE}.t ]]; then
      echo "-- $(date +%T) no changes to $out"
      rm -f ${POTFILE}.t
      return
    fi
    rm -f ${POTFILE}.t
  fi

  echo "-- $(date +%T) creating $out"
  if [[ -f ${out} && ${out} != en_GB.po ]]; then
    # re-use data from existing file
    mv -f ${out} ${out}.current
    > ${out}

    sed -n '1,/^$/ p' ${out}.current >> ${out}
    # "POT-Creation-Date: 2022-05-26"
    cdt=$(grep -E '^"POT-Creation-Date:' bdj4.pot)
    # "PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE"
    # "PO-Revision-Date: 2022-05-27 14:09"
    sed -i \
      -e "s,PO-Revision-Date:.*,PO-Revision-Date: ${dt}\\\\n\"," \
      -e "s/: [0-9][0-9-]* [0-9][0-9:-]*/: ${dt}/" \
      ${out}
    sed -n '/^$/,$ p' bdj4.pot >> ${out}

    echo "-- $(date +%T) updating translations from old .po files"
    ./lang-lookup.sh ${out}
  else
    # new en_GB file
    > ${out}
    echo "# == $dlang" >> ${out}
    echo "# -- $elang" >> ${out}
    sed -e '1,6 d' \
        -e "s/YEAR-MO-DA.*ZONE/${dt}/" \
        -e "s/: [0-9][0-9-]* [0-9][0-9:-]*/: ${dt}/" \
        -e "s/PACKAGE/ballroomdj-4/" \
        -e "s/VERSION/${VERSION}/" \
        -e "s/FULL NAME.*ADDRESS./${xlator}/" \
        -e "s/Bugs-To: /Bugs-To: brad.lanam.di@gmail.com/" \
        -e "s/Language: /Language: ${lang}/" \
        -e "s,Language-Team:.*>,Language-Team: $elang," \
        -e "s/CHARSET/UTF-8/" \
        bdj4.pot >> ${out}
  fi
}

FORCE=F
while test $# -gt 0; do
  case $1 in
    --force)
      shift
      FORCE=T
      ;;
  esac
done

echo "-- $(date +%T) extracting additional strings"
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
echo "// CONTEXT: configuration file: The completion message displayed on the marquee when the playlist is finished." >> $TMP
sed -n -e '/^COMPLETEMSG/ {n;p}' $fn >> $TMP

for qn in 0 1 2 3; do
  fn=../templates/bdjconfig.q${qn}.txt
  ctxt="// CONTEXT: (noun) configuration file: name of a music queue"
  sed -n -e "/^QUEUE_NAME/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMP
done

ctxt="// CONTEXT: text from the HTML templates (buttons/labels/alt-text)"
grep -E 'value=' ../templates/*.html |
  sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMP
grep -E 'alt=' ../templates/*.html |
  sed -e 's,.*alt=",,' -e 's,".*,,' -e '/^BDJ4$/ d' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMP
grep -E '<p[^>]*>[A-Za-z][A-Za-z]*</p>' ../templates/*.html |
  sed -e 's,.*: *<,<,' -e 's,<[^>]*>,,g' -e 's,^ *,,' -e 's, *$,,' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMP

# names of playlist files
echo "// CONTEXT: The name of the 'automatic' playlist file" >> $TMP
echo "..automatic" >> $TMP
echo "// CONTEXT: The name of the 'standardrounds' playlist file" >> $TMP
echo "..standardrounds" >> $TMP
echo "// CONTEXT: The name of the 'queuedance' playlist file" >> $TMP
echo "..QueueDance" >> $TMP

# linux desktop shortcut
echo "// CONTEXT: tooltip for desktop icon" >> $TMP
grep -E '^Comment=' ../install/bdj4.desktop |
  sed -e 's,Comment=,,' -e 's,^,..,' >> $TMP

sed -e '/^\.\./ {s,^\.\.,, ; s,^,_(", ; s,$,"),}' $TMP > $TMP.n
mv -f $TMP.n $TMP

# preserve the original pot file w/o the creation date for later diff
(
  cd po
  sed -e '/^"POT-Creation-Date:/ d' ${POTFILE} > ${POTFILE}.a
  mv ${POTFILE} ${POTFILE}.k
)

echo "-- $(date +%T) extracting"
# */*/*.c is not needed, as those are the check modules.
xgettext -s -d bdj4 \
    --from-code=UTF-8 \
    --language=C \
    --add-comments=CONTEXT: \
    --no-location \
    --keyword=_ \
    --flag=_:1:pass-c-format \
    *.c */*.c */*.m */*.cpp \
    -p po -o ${POTFILE}

rm -f $TMP

cd po

awk -f extract-helptext.awk ${POTFILE} > ${POTFILE}.n

CHANGED=F
sed -e '/^"POT-Creation-Date:/ d' ${POTFILE}.n > ${POTFILE}.b
diff ${POTFILE}.a ${POTFILE}.b > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  mv -f ${POTFILE}.n ${POTFILE}
  CHANGED=T
else
  echo "   No changes to extracted text"
  mv -f ${POTFILE}.k ${POTFILE}
  rm -f ${POTFILE}.n
fi
rm -f ${POTFILE}.a ${POTFILE}.b ${POTFILE}.k
if [[ $FORCE == T ]]; then
  CHANGED=T
fi

if [[ $CHANGED == T ]]; then
  fn=en_GB.po
  mkpo en ${fn} 'Automatically generated' 'English (GB)' english/gb
  # The Queue_noun tag must be replaced with the correct noun.
  sed -i -e '/"Queue_noun"/ { n ; s,"","Queue", ; }' ${fn}

  echo "-- $(date +%T) creating en_US.po"
  awk -f mken_us.awk en_GB.po > en_US.po
fi

# If a translation is removed, remember to also remove
# the locale/xx files and directories.

mkpo nl nl_BE.po 'marimo' Nederlands dutch
# the turkish translation is not done well.
# mkpo tr tr_TR.po 'Turkish' Türkçe turkish

#mkpo fr fr_FR.po 'unassigned' Français french
#mkpo fi fi_FI.po 'unassigned' Suomi finnish
#mkpo de de_DE.po 'unassigned' Deutsch german
#mkpo es es_ES.po 'unassigned' Español spanish
#mkpo zh zh_TW.po 'unassigned' "繁體中文" "chinese (TW)"
#mkpo zh zh_CN.po 'unassigned' "简体中文" "chinese (CN)"

echo "-- $(date +%T) finished"

exit 0
