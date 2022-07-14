#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "datafile.h"
#include "fileop.h"
#include "lock.h"
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
  { "COUNT",              VOLREG_COUNT,     VALUE_NUM, NULL, -1 },
  { "ORIGVOL",            VOLREG_ORIG_VOL,  VALUE_NUM, NULL, -1 },
  { "SINK",               VOLREG_SINK,      VALUE_STR, NULL, -1 },
};

static void volregLockWait (void);
static void volregUnlock (void);
static int  volregLockFilename (char *tfn, size_t sz);
static int  volregUpdate (const char *sink, int originalVolume, int inc);
static void volregDataFilename (char *tfn, size_t sz);
static int  volregGetFilename (char *tfn, size_t sz, char *fn);

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

  fileopMakeDir (sysvarsGetStr (SV_CONFIG_DIR));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ3_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_CONFIGDIR);
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

  fileopMakeDir (sysvarsGetStr (SV_CONFIG_DIR));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ4_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_CONFIGDIR);
  fh = fopen (fn, "w");
  if (fh != NULL) {
    fclose (fh);
  }
}

void
volregClearBDJ4Flag (void)
{
  char  fn [MAXPATHLEN];

  fileopMakeDir (sysvarsGetStr (SV_CONFIG_DIR));
  pathbldMakePath (fn, sizeof (fn),
      VOLREG_BDJ4_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_CONFIGDIR);
  fileopDelete (fn);
}

/* internal routines */

static void
volregLockWait (void)
{
  int     count;
  char    tfn [MAXPATHLEN];
  int     flags = PATHBLD_MP_NONE;

  flags = volregLockFilename (tfn, sizeof (tfn));
fprintf (stderr, "volreg-get-lock: %s\n", tfn);

  count = 0;
  while (lockAcquire (tfn, flags) < 0 &&
      count < 2000) {
    mssleep (10);
    ++count;
  }
}

static void
volregUnlock (void)
{
  char  tfn [MAXPATHLEN];
  int   flags;

  flags = volregLockFilename (tfn, sizeof (tfn));
fprintf (stderr, "volreg-clr-lock: %s\n", tfn);

  lockRelease (tfn, flags);
}

static int
volregLockFilename (char *tfn, size_t sz)
{
  int   flags = PATHBLD_MP_NONE;

  flags = volregGetFilename (tfn, sz, VOLREG_LOCK);
  return flags;
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
  char        *dsink;
  int         rval;


  volregLockWait ();
  volregDataFilename (fn, sizeof (fn));
fprintf (stderr, "volreg-fn: %s\n", fn);
  df = datafileAllocParse ("volreg", DFTYPE_INDIRECT, fn,
      volregdfkeys, VOLREG_KEY_MAX, DATAFILE_NO_LOOKUP);
  vlist = datafileGetList (df);
  if (vlist == NULL) {
    vlist = ilistAlloc ("volreg", LIST_ORDERED);
  }

  ilistStartIterator (vlist, &viteridx);
  vkey = -1;
  while ((key = ilistIterateKey (vlist, &viteridx)) >= 0) {
    dsink = ilistGetStr (vlist, key, VOLREG_SINK);
    if (strcmp (sink, dsink) == 0) {
      vkey = key;
      break;
    }
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
fprintf (stderr, "volreg-set: %d %s\n", 1, sink);
    } else {
      count = ilistGetNum (vlist, vkey, VOLREG_COUNT);
      ++count;
      ilistSetNum (vlist, vkey, VOLREG_COUNT, count);
fprintf (stderr, "volreg-inc: %d %s\n", count, sink);
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
        rval = ilistGetNum (vlist, vkey, VOLREG_ORIG_VOL);
      }
    }
  }

  datafileSaveIndirect ("volreg", fn, volregdfkeys, VOLREG_KEY_MAX, vlist);
  volregUnlock ();
  return rval;
}

static void
volregDataFilename (char *tfn, size_t sz)
{
  volregGetFilename (tfn, sz, VOLREG_FN);
  if (strcmp (tfn, VOLREG_FN) == 0) {
    pathbldMakePath (tfn, sz, VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  }
}

static int
volregGetFilename (char *tfn, size_t sz, char *fn)
{
  int   flags = PATHBLD_MP_NONE;
  char  tbuff [MAXPATHLEN];

  strlcpy (tfn, fn, sz);
  pathbldMakePath (tbuff, sizeof (tbuff), fn, BDJ4_LINK_EXT, PATHBLD_MP_DATA);
  if (fileopFileExists (tbuff)) {
    FILE    *fh;

    fh = fopen (tbuff, "r");
    fgets (tfn, sz, fh);
    stringTrim (tfn);
    fclose (fh);
    fn = tfn;
    flags |= PATHBLD_LOCK_FFN;
  }

  return flags;
}



