#!/bin/bash
#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

rm -f po/*~ po/po/*~ po/web/*~ po/old/*~ > /dev/null 2>&1
export POTFILE=bdj4.pot

. ../VERSION.txt
export VERSION
dt=$(date '+%F')
export dt

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
    po/potemplates.c */*.c */*.m */*.cpp \
    -p po -o ${POTFILE}

cd po

awk -f extract-helptext.awk ${POTFILE} > ${POTFILE}.n

sed -e '/^"POT-Creation-Date:/ d' ${POTFILE}.n > ${POTFILE}.b
diff ${POTFILE}.a ${POTFILE}.b > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
  mv -f ${POTFILE}.n ${POTFILE}
else
  echo "   No changes to extracted text"
  mv -f ${POTFILE}.k ${POTFILE}
fi

rm -f ${POTFILE}.a ${POTFILE}.b ${POTFILE}.k ${POTFILE}.n
exit 0
