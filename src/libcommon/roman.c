/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "roman.h"

typedef struct {
  int         value;
  const char  *rep;
  const char  *lcrep;
  bool        subflag;
} romanidx_t;

romanidx_t romtable [] = {
  { 1000, "M", "m", false },
  {  500, "D", "d", false },
  {  100, "C", "c", true },
  {   50, "L", "l", false },
  {   10, "X", "x", true },
  {    5, "V", "v", false },
  {    1, "I", "i", true }
};

enum {
  romtabsz = sizeof (romtable) / sizeof (romanidx_t),
};

/* this is used for classical music numbers which are not likely */
/* to go over 10-15, but write the correct code anyways */
int
convertToRoman (int value, char * buff, size_t sz)
{
  size_t    dispidx = 0;
  int       rc = 0;

  *buff = '\0';

  if (value <= 0) {
    return 0;
  }
  if (value > 3999) {
    return 0;
  }

  for (int idx = 0; idx < romtabsz; ++idx) {
    while (value >= romtable [idx].value) {
      buff [dispidx++] = *romtable [idx].rep;
      value -= romtable [idx].value;
    }
    /* subtractive notation */
    for (int tidx = idx + 2; tidx > idx; --tidx) {
      int   trom;

      if (tidx >= romtabsz) {
        continue;
      }
      if (romtable [tidx].subflag == false) {
        continue;
      }

      trom = romtable [idx].value - romtable [tidx].value;
      if (value >= trom) {
        buff [dispidx++] = *romtable [tidx].rep;
        buff [dispidx++] = *romtable [idx].rep;
        value -= trom;
      }
    }
  }

  buff [dispidx++] = '\0';

  if (value > 0) {
    rc = -1;
  }
  return rc;
}
