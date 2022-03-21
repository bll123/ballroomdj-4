#!/bin/bash

echo "Will overwrite existing .po files."
echo "Press enter to continue."
echo -n ": "
read answer

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

  > ${out}
  echo "# == $dlang" >> ${out}
  echo "# -- $elang" >> ${out}
  sed -e '1,6 d' \
      -e "s/YEAR-MO-DA.*ZONE/${dt}/" \
      -e "s/: [0-9][0-9 :-]*/: ${dt}/" \
      -e "s/PACKAGE/BDJ4/" \
      -e "s/VERSION/${VERSION}/" \
      -e "s/FULL NAME.*ADDRESS./${xlator}/" \
      -e "s/Bugs-To: /Bugs-To: brad.lanam.comp@gmail.com/" \
      -e "s/Language: /Language: ${lang}/" \
      -e "s/Language-Team:.*>/Language-Team: $elang/" \
      -e "s/CHARSET/utf-8/" \
      bdj4.pot >> ${out}
}

xgettext -s -d bdj4 \
    -C --add-comments=CONTEXT: \
    --no-location \
    --keyword=_ \
    --flag=_:1:pass-c-format \
    *.c *.cpp *.m \
    -p po -o bdj4.pot
cd po

TMP=templates.c
> $TMP
fn=../../templates/dancetypes.txt
sed -e '/^#/d' $fn >> $TMP
fn=../../templates/dances.txt
sed -n -e '/^DANCE/ {n;p}' $fn >> $TMP
fn=../../templates/ratings.txt
sed -n -e '/^RATING/ {n;p}' $fn >> $TMP
fn=../../templates/genres.txt
sed -n -e '/^GENRE/ {n;p}' $fn >> $TMP
fn=../../templates/levels.txt
sed -n -e '/^LABEL/ {n;p}' $fn >> $TMP
fn=../../templates/status.txt
sed -n -e '/^STATUS/ {n;p}' $fn >> $TMP

egrep 'value=' ../../templates/*.html |
  sed -e 's,.*value=",,' -e 's,".*,,' -e '/^100$/ d' >> $TMP

sed -e 's,^\.\.,,' -e 's,^,_(",' -e 's,$,"),' $TMP > $TMP.n
mv -f $TMP.n $TMP

xgettext -s -j -d bdj4 \
    --no-location \
    --keyword=_ \
    --flag=_:1:pass-c-format \
    $TMP \
    -o bdj4.pot
rm -f $TMP

mkpo en en_US.po "Automatically generated" "English (US)" english
mkpo nl nl.po "marimo" Nederlands dutch
#mkpo de de_DE.po "various" Deutsch german
./lang-lookup.sh
