/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "datafile.h"
#include "dirop.h"
#include "fileop.h"
#include "lock.h"
#include "mdebug.h"
#include "ilist.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"
#include "volreg.h"

enum {
  VOLREG_SINK,
  VOLREG_COUNT,
  VOLREG_ORIG_VOL,
  VOLREG_KEY_MAX,
};

static datafilekey_t volregdfkeys [VOLREG_KEY_MAX] = {
  { "COUNT",              VOLREG_COUNT,     VALUE_NUM, NULL, DF_NORM },
  { "ORIGVOL",            VOLREG_ORIG_VOL,  VALUE_NUM, NULL, DF_NORM },
  { "SINK",               VOLREG_SINK,      VALUE_STR, NULL, DF_NORM },
};

static void volregLockWait (void);
static void volregUnlock (void);
static int  volregUpdate (const char *sink, int originalVolume, int inc);

/* returns the number of processes registered to this sink */
int
volregSave (const char *sink, int originalVolume)
{
  int   count;

  count = volregUpdate (sink, originalVolume, 1);
  return count;
}

/* returns -1 if there are other processes registered to this sink */
/* otherwise returns the original system volume */
int
volregClear (const char *sink)
{
  int origvol;

  origvol = volregUpdate (sink, 0, -1);
  return origvol;
}

bool
volregCheckBDJ3Flag (void)
{
  char  fn [MAXPATHLEN];
  int   rc = false;

  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ3_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);
  if (fileopFileExists (fn)) {
    rc = true;
  }

  return rc;
}

void
volregCreateBDJ4Flag (void)
{
  char  fn [MAXPATHLEN];
  FILE  *fh;

  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ4_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);
  fh = fileopOpen (fn, "w");
  if (fh != NULL) {
    mdextfclose (fh);
    fclose (fh);
  }
}

void
volregClearBDJ4Flag (void)
{
  char  fn [MAXPATHLEN];

  diropMakeDir (sysvarsGetStr (SV_DIR_CONFIG));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ4_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);
  fileopDelete (fn);
}

void
volregClean (void)
{
  char  fn [MAXPATHLEN];

  volregUnlock ();
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (fn);
}

/* internal routines */

static void
volregLockWait (void)
{
  int     count;
  char    tfn [MAXPATHLEN];

  pathbldMakePath (tfn, sizeof (tfn),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);

  count = 0;
  while (lockAcquire (tfn, PATHBLD_LOCK_FFN) < 0 &&
      count < 2000) {
    mssleep (10);
    ++count;
  }
}

static void
volregUnlock (void)
{
  char  tfn [MAXPATHLEN];

  diropMakeDir (sysvarsGetStr (SV_DIR_CACHE));
  pathbldMakePath (tfn, sizeof (tfn),
      VOLREG_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_CACHE);
  lockRelease (tfn, PATHBLD_LOCK_FFN);
}

static int
volregUpdate (const char *sink, int originalVolume, int inc)
{
  datafile_t  *df;
  ilist_t     *vlist;
  ilistidx_t  viteridx;
  char        fn [MAXPATHLEN];
  ilistidx_t  key;
  ilistidx_t  vkey;
  const char  *dsink;
  int         rval;
  bool        newvlist = false;


  diropMakeDir (sysvarsGetStr (SV_DIR_CACHE));
  volregLockWait ();
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  df = datafileAllocParse ("volreg", DFTYPE_INDIRECT, fn,
      volregdfkeys, VOLREG_KEY_MAX, DF_NO_OFFSET, NULL);
  vlist = datafileGetList (df);

  vkey = -1;
  if (vlist != NULL && ilistGetCount (vlist) > 0) {
    ilistStartIterator (vlist, &viteridx);
    while ((key = ilistIterateKey (vlist, &viteridx)) >= 0) {
      dsink = ilistGetStr (vlist, key, VOLREG_SINK);
      if (strcmp (sink, dsink) == 0) {
        vkey = key;
        break;
      }
    }
  }

  if (vlist == NULL) {
    vlist = ilistAlloc ("volreg", LIST_ORDERED);
    newvlist = true;
  }

  rval = -1;

  if (inc == 1) {
    int   count;

    count = 0;
    if (vkey >= 0) {
      count = ilistGetNum (vlist, vkey, VOLREG_COUNT);
    } else {
      vkey = ilistGetCount (vlist);
    }

    if (count == 0) {
      ilistSetStr (vlist, vkey, VOLREG_SINK, sink);
      ilistSetNum (vlist, vkey, VOLREG_COUNT, 1);
      ilistSetNum (vlist, vkey, VOLREG_ORIG_VOL, originalVolume);
    } else {
      count = ilistGetNum (vlist, vkey, VOLREG_COUNT);
      ++count;
      ilistSetNum (vlist, vkey, VOLREG_COUNT, count);
    }
    count = ilistGetNum (vlist, vkey, VOLREG_COUNT);
    rval = count;
  }

  if (inc == -1) {
    if (vkey == -1) {
      fprintf (stderr, "ERR: Unable to locate %s in volreg\n", sink);
    } else {
      int   count;

      count = ilistGetNum (vlist, vkey, VOLREG_COUNT);
      --count;
      ilistSetNum (vlist, vkey, VOLREG_COUNT, count);
      if (count > 0) {
        rval = -1;
      } else {
        if (count < 0) {
          count = 0;
        }
        rval = ilistGetNum (vlist, vkey, VOLREG_ORIG_VOL);
      }
    }
  }

  datafileSave (df, NULL, vlist, DF_NO_OFFSET, datafileDistVersion (df));
  datafileFree (df);
  if (newvlist) {
    ilistFree (vlist);
  }
  volregUnlock ();
  return rval;
}
