/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "log.h"
#include "mdebug.h"
#include "uinbutil.h"

uinbtabid_t *
uinbutilIDInit (void)
{
  uinbtabid_t *nbtabid;

  nbtabid = mdmalloc (sizeof (uinbtabid_t));
  nbtabid->tabcount = 0;
  nbtabid->tabids = NULL;
  return nbtabid;
}

void
uinbutilIDFree (uinbtabid_t *nbtabid)
{
  if (nbtabid != NULL) {
    dataFree (nbtabid->tabids);
    mdfree (nbtabid);
  }
}

void
uinbutilIDAdd (uinbtabid_t *nbtabid, int id)
{
  nbtabid->tabids = mdrealloc (nbtabid->tabids,
      sizeof (int) * (nbtabid->tabcount + 1));
  nbtabid->tabids [nbtabid->tabcount] = id;
  ++nbtabid->tabcount;
}

int
uinbutilIDGet (uinbtabid_t *nbtabid, int idx)
{
  if (nbtabid == NULL) {
    return 0;
  }
  if (idx >= nbtabid->tabcount) {
    return 0;
  }
  return nbtabid->tabids [idx];
}

int
uinbutilIDGetPage (uinbtabid_t *nbtabid, int id)
{
  if (nbtabid == NULL) {
    return 0;
  }

  /* just brute forced.  the list is very small. */
  /* used by manageui */
  for (int i = 0; i < nbtabid->tabcount; ++i) {
    if (id == nbtabid->tabids [i]) {
      return i;
    }
  }

  return 0;
}

void
uinbutilIDStartIterator (uinbtabid_t *nbtabid, int *iteridx)
{
  *iteridx = -1;
}

int
uinbutilIDIterate (uinbtabid_t *nbtabid, int *iteridx)
{
  ++(*iteridx);
  if (*iteridx >= nbtabid->tabcount) {
    return -1;
  }
  return nbtabid->tabids [*iteridx];
}

