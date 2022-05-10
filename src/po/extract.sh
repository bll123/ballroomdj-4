#!/bin/bash

cwd=$(pwd)
case ${cwd} in
  */po)
    cd ..
    ;;
esac

rm -f po/*~ po/old/*~ > /dev/null 2>&1

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
  if [[ -f ${out} ]]; then
    mv ${out} ${out}.old
  fi
  > ${out}
  echo "# == $dlang" >> ${out}
  echo "# -- $elang" >> ${out}
  sed -e '1,6 d' \
      -e "s/YEAR-MO-DA.*ZONE/${dt}/" \
      -e "s/: [0-9][0-9][0-9 :-]*/: ${dt}/" \
      -e "s/PACKAGE/BDJ4/" \
      -e "s/VERSION/${VERSION}/" \
      -e "s/FULL NAME.*ADDRESS./${xlator}/" \
      -e "s/Bugs-To: /Bugs-To: brad.lanam.comp@gmail.com/" \
      -e "s/Language: /Language: ${lang}/" \
      -e "s,Language-Team:.*>,Language-Team: $elang," \
      -e "s/CHARSET/utf-8/" \
      bdj4.pot >> ${out}
}

TMP=potemplates.c

> $TMP
fn=../templates/dancetypes.txt
sed -e '/^#/d' -e 's,^,..,' $fn >> $TMP
fn=../templates/dances.txt
sed -n -e '/^DANCE/ {n;p}' -e 's,^,..,' $fn >> $TMP
fn=../templates/ratings.txt
sed -n -e '/^RATING/ {n;p}' -e 's,^,..,' $fn >> $TMP
fn=../templates/genres.txt
sed -n -e '/^GENRE/ {n;p}' -e 's,^,..,' $fn >> $TMP
fn=../templates/levels.txt
sed -n -e '/^LEVEL/ {n;p}' -e 's,^,..,' $fn >> $TMP
fn=../templates/status.txt
sed -n -e '/^STATUS/ {n;p}' -e 's,^,..,' $fn >> $TMP
fn=../templates/bdjconfig.txt.p
echo "// CONTEXT: The names of the music queues" >> $TMP
sed -n -e '/^QUEUE_NAME_[AB]/ {n;p}' -e 's,^,..,' $fn >> $TMP
echo "// CONTEXT: The completion message displayed on the marquee when the playlist is finished." >> $TMP
sed -n -e '/^COMPLETEMSG/ {n;p}' -e 's,^,..,' $fn >> $TMP

egrep 'value=' ../templates/*.html |
  sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' -e 's,^,..,' >> $TMP

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
    *.c */*.c \
    -p po -o bdj4.pot

rm -f $TMP

cd po

mkpo en en_GB.po "Automatically generated" "English (GB)" english/gb
rm -f en_GB.po.old
mkpo nl nl_BE.po "marimo" Nederlands dutch
# mkpo de de_DE.po "unassigned" Deutsch german
./lang-lookup.sh
echo "-- $(date +%T) creating english/us .po files"
./mken_us.sh
echo "-- $(date +%T) finished"

exit 0
