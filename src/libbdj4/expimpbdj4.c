#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "bdj4.h"
#include "dirop.h"
#include "expimpbdj4.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathbld.h"
#include "song.h"

typedef struct eibdj4 {
  musicdb_t   *musicdb;
  const char  *dirname;
  nlist_t     *dbidxlist;
  nlistidx_t  dbidxiter;
  char        *plName;
  const char  *newName;
  int         eiflag;
  int         counter;
  int         totcount;
  int         state;
} eibdj4_t;

static bool eibdj4ProcessExport (eibdj4_t *eibdj4);
static bool eibdj4ProcessImport (eibdj4_t *eibdj4);

eibdj4_t *
eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag)
{
  eibdj4_t  *eibdj4;

  eibdj4 = mdmalloc (sizeof (eibdj4_t));
  eibdj4->musicdb = musicdb;
  eibdj4->dirname = dirname;
  eibdj4->dbidxlist = NULL;
  eibdj4->plName = NULL;
  eibdj4->newName = NULL;
  eibdj4->eiflag = eiflag;
  eibdj4->counter = 0;
  eibdj4->totcount = 0;
  eibdj4->state = BDJ4_STATE_START;

  return eibdj4;
}

void
eibdj4Free (eibdj4_t *eibdj4)
{
  if (eibdj4 != NULL) {
    dataFree (eibdj4->plName);
    mdfree (eibdj4);
  }
}

void
eibdj4SetDBIdxList (eibdj4_t *eibdj4, nlist_t *dbidxlist)
{
  if (eibdj4 == NULL) {
    return;
  }

  eibdj4->dbidxlist = dbidxlist;
}

void
eibdj4SetName (eibdj4_t *eibdj4, const char *name)
{
  if (eibdj4 == NULL) {
    return;
  }

  eibdj4->plName = mdstrdup (name);
}

void
eibdj4GetCount (eibdj4_t *eibdj4, int *count, int *tot)
{
  if (eibdj4 == NULL) {
    *count = 0;
    *tot = 0;
    return;
  }

  *count = eibdj4->counter;
  *tot = eibdj4->totcount;
}

bool
eibdj4Process (eibdj4_t *eibdj4)
{
  bool    rc = true;

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state != BDJ4_STATE_START) {
    return rc;
  }

  if (eibdj4->eiflag == EIBDJ4_EXPORT) {
    if (eibdj4->dbidxlist == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
    if (eibdj4->plName == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
    rc = eibdj4ProcessExport (eibdj4);
  }
  if (eibdj4->eiflag == EIBDJ4_IMPORT) {
    rc = eibdj4ProcessImport (eibdj4);
  }

  return rc;
}

/* internal routines */

static bool
eibdj4ProcessExport (eibdj4_t *eibdj4)
{
  bool      rc = true;
  dbidx_t   dbidx;
  char      tbuff [MAXPATHLEN];
  char      from [MAXPATHLEN];

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state == BDJ4_STATE_START) {
    nlistStartIterator (eibdj4->dbidxlist, &eibdj4->dbidxiter);
fprintf (stderr, "export: dir: %s\n", eibdj4->dirname);

    snprintf (tbuff, sizeof (tbuff), "%s/data", eibdj4->dirname);
    diropMakeDir (tbuff);
    snprintf (tbuff, sizeof (tbuff), "%s/music", eibdj4->dirname);
    diropMakeDir (tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_PLAYLIST_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_PL_DANCE_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_SONGLIST_EXT);
fprintf (stderr, "from: %s\n  to: %s\n", from, tbuff);
    filemanipCopy (from, tbuff);

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t    *song;

    if ((dbidx = nlistIterateKey (eibdj4->dbidxlist, &eibdj4->dbidxiter)) >= 0) {
fprintf (stderr, "export %d\n", dbidx);
      song = dbGetByIdx (eibdj4->musicdb, dbidx);
      rc = false;
    } else {
      eibdj4->state = BDJ4_STATE_FINISH;
    }
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    eibdj4->dbidxlist = NULL;
    rc = true;
  }

  return rc;
}

static bool
eibdj4ProcessImport (eibdj4_t *eibdj4)
{
  if (eibdj4 == NULL || eibdj4->state == BDJ4_STATE_OFF) {
    return true;
  }
  return true;
}

