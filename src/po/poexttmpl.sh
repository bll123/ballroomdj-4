#!/bin/bash
#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

rm -f po/*~ po/po/*~ po/web/*~ > /dev/null 2>&1
export POTFILE=bdj4.pot

. ../VERSION.txt
export VERSION
dt=$(date '+%F')
export dt

echo "-- $(date +%T) extracting additional strings"
TMPLOUT=po/potemplates.c
> $TMPLOUT

fn=../templates/audiosrc.txt
echo " // CONTEXT: configuration file: podcast name" >> $TMPLOUT
echo "..Podcast" >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: dance"
fn=../templates/dances.txt
sed -n -e "/^DANCE/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: dance type"
fn=../templates/dancetypes.txt
sed -e '/^#/d' -e 's,^,..,' -e "s,^,${ctxt}\n," $fn >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: rating"
fn=../templates/ratings.txt
sed -n -e "/^RATING/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: genre"
fn=../templates/genres.txt
sed -n -e "/^GENRE/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: dance level"
fn=../templates/levels.txt
sed -n -e "/^LEVEL/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT

ctxt=" // CONTEXT: configuration file: status"
fn=../templates/status.txt
sed -n -e "/^STATUS/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT

fn=../templates/bdjconfig.txt.p
echo " // CONTEXT: configuration file: The completion message displayed on the marquee when the playlist is finished." >> $TMPLOUT
sed -n -e '/^COMPLETEMSG/ {n;p}' $fn >> $TMPLOUT

for fn in ../templates/bdjconfig.q?.txt; do
  ctxt=" // CONTEXT: (noun) configuration file: name of a music queue"
  sed -n -e "/^QUEUE_NAME/ {n;s,^,${ctxt}\n,;p}" $fn >> $TMPLOUT
done

ctxt=" // CONTEXT: text from the HTML templates (buttons/labels/alt-text)"
grep -E 'value=' ../templates/*.html |
  sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMPLOUT
grep -E 'alt=' ../templates/*.html |
  sed -e 's,.*alt=",,' -e 's,".*,,' -e '/^BDJ4$/ d' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMPLOUT
grep -E '<p[^>]*>[A-Za-z][A-Za-z]*</p>' ../templates/*.html |
  sed -e 's,.*: *<,<,' -e 's,<[^>]*>,,g' -e 's,^ *,,' -e 's, *$,,' \
      -e 's,^,..,' -e "s,^,${ctxt}\n," >> $TMPLOUT

# names of playlist files
echo " // CONTEXT: The name of the 'standardrounds' playlist file" >> $TMPLOUT
echo "..standardrounds" >> $TMPLOUT
echo " // CONTEXT: The name of the 'queuedance' playlist file" >> $TMPLOUT
echo "..QueueDance" >> $TMPLOUT

# linux desktop shortcut
echo " // CONTEXT: tooltip for desktop icon" >> $TMPLOUT
grep -E '^Comment=' ../install/bdj4.desktop |
  sed -e 's,Comment=,,' -e 's,^,..,' >> $TMPLOUT

sed -e '/^\.\./ {s,^\.\.,, ; s,^,_(", ; s,$,"),}' $TMPLOUT > $TMPLOUT.n
mv -f $TMPLOUT.n $TMPLOUT

exit 0
