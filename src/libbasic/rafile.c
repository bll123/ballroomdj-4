/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "bdjstring.h"
#include "fileop.h"
#include "fileshared.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "rafile.h"
#include "tmutil.h"

typedef struct rafile {
  fileshared_t  *fsh;
  char          *fname;
  int           version;
  rafileidx_t   size;
  rafileidx_t   count;
  unsigned int  inbatch;
  unsigned int  locked;
  int           openmode;   // fileshared openmode, not rafile
} rafile_t;

static char ranulls [RAFILE_REC_SIZE];

static int  raReadHeader (rafile_t *);
static void raWriteHeader (rafile_t *, int);
static void raLock (rafile_t *);
static void raUnlock (rafile_t *);
static size_t rrnToOffset (rafileidx_t rrn);

rafile_t *
raOpen (const char *fname, int version, int openmode)
{
  rafile_t        *rafile;
  bool            fexists;
  int             rc;

  logProcBegin ();
  rafile = mdmalloc (sizeof (rafile_t));
  rafile->fsh = NULL;
  rafile->openmode = FILESH_OPEN_READ;
  if (openmode == RAFILE_RW) {
    rafile->openmode = FILESH_OPEN_READ_WRITE;
  }

  fexists = fileopFileExists (fname);

  rafile->fsh = fileSharedOpen (fname, rafile->openmode, FILESH_NO_FLUSH);
  if (rafile->fsh == NULL) {
    mdfree (rafile);
    return NULL;
  }
  rafile->inbatch = 0;
  rafile->locked = 0;
  rafile->version = version;
  rafile->size = RAFILE_REC_SIZE;

  if (fileopSize (fname) == 0) {
    fexists = false;
  }
  if (fexists) {
    raLock (rafile);
    rc = raReadHeader (rafile);
    raUnlock (rafile);

    if (rc != 0) {
      /* probably an incorrect filename   */
      /* don't try to do anything with it */
      mdfree (rafile);
      logProcEnd ("bad-header");
      return NULL;
    }
  }
  if (! fexists) {
    rafile->count = 0L;
    raLock (rafile);
    raWriteHeader (rafile, version);
    raUnlock (rafile);
  }
  rafile->fname = mdstrdup (fname);
  memset (ranulls, 0, RAFILE_REC_SIZE);
  logProcEnd ("");
  return rafile;
}

void
raClose (rafile_t *rafile)
{
  if (rafile == NULL) {
    return;
  }

  logProcBegin ();
  fileSharedClose (rafile->fsh);
  raUnlock (rafile);
  rafile->fsh = NULL;
  dataFree (rafile->fname);
  mdfree (rafile);
  logProcEnd ("");
}

rafileidx_t
raGetCount (rafile_t *rafile)
{
  if (rafile == NULL) {
    return 0;
  }

  return rafile->count;
}

void
raStartBatch (rafile_t *rafile)
{
  if (rafile == NULL) {
    return;
  }

  logProcBegin ();
  raLock (rafile);
  rafile->inbatch = 1;
  fileSharedFlush (rafile->fsh, FILESH_NO_SYNC);
  logProcEnd ("");
}

void
raEndBatch (rafile_t *rafile)
{
  if (rafile == NULL) {
    return;
  }

  logProcBegin ();
  rafile->inbatch = 0;
  fileSharedFlush (rafile->fsh, FILESH_SYNC);
  raUnlock (rafile);
  logProcEnd ("");
}

rafileidx_t
raWrite (rafile_t *rafile, rafileidx_t rrn, char *data, ssize_t len)
{
  bool    isnew = false;

  if (rafile == NULL) {
    return RAFILE_UNKNOWN;
  }

  if (rafile->openmode == FILESH_OPEN_READ) {
    return RAFILE_UNKNOWN;
  }

  if (len == -1) {
    len = strlen (data);
  }
  logProcBegin ();
  /* leave one byte for the terminating null */
  if (len > (RAFILE_REC_SIZE - 1)) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad data len %" PRId64, (int64_t) len);
    return RAFILE_UNKNOWN;
  }

  raLock (rafile);
  if (rrn == RAFILE_NEW) {
    isnew = true;
    ++rafile->count;
    rrn = rafile->count;
    raWriteHeader (rafile, rafile->version);
  } else {
    if (rrn > rafile->count) {
      rafile->count = rrn;
      raWriteHeader (rafile, rafile->version);
    }
  }
  if (! isnew) {
    fileSharedSeek (rafile->fsh, rrnToOffset (rrn), SEEK_SET);
    fileSharedWrite (rafile->fsh, ranulls, RAFILE_REC_SIZE);
  }
  fileSharedSeek (rafile->fsh, rrnToOffset (rrn), SEEK_SET);
  fileSharedWrite (rafile->fsh, data, len);
  if (isnew) {
    /* write one null byte to the next record so */
    /* that the last record has a size and is readable */
    fileSharedSeek (rafile->fsh, rrnToOffset (rrn + 1), SEEK_SET);
    fileSharedWrite (rafile->fsh, ranulls, 1);
  }
  if (! rafile->inbatch) {
    fileSharedFlush (rafile->fsh, FILESH_SYNC);
  }
  raUnlock (rafile);
  logProcEnd ("");

  return rrn;
}

int
raRead (rafile_t *rafile, rafileidx_t rrn, char *data)
{
  rafileidx_t   rc;

  if (rafile == NULL) {
    return 0;
  }

  logProcBegin ();
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad rrn %" PRId32 " racount %" PRId32, rrn, rafile->count);
    return 0;
  }

  /* as there are multiple processes, */
  /* the reader's buffers must also be flushed */
  if (! rafile->inbatch) {
    fileSharedFlush (rafile->fsh, FILESH_NO_SYNC);
  }
  raLock (rafile);
  fileSharedSeek (rafile->fsh, rrnToOffset (rrn), SEEK_SET);
  rc = (rafileidx_t) fileSharedRead (rafile->fsh, data, RAFILE_REC_SIZE);
  raUnlock (rafile);
  logProcEnd ("");
  return rc;
}

/* local routines */

static int
raReadHeader (rafile_t *rafile)
{
  char        buff [90];
  char        *rv;
  int         rrc;
  int         version;
  int         rasize;
  rafileidx_t count;

  if (rafile == NULL) {
    return 0;
  }

  logProcBegin ();
  /* as there are multiple processes, */
  /* the reader's buffers must also be flushed */
  if (! rafile->inbatch) {
    fileSharedFlush (rafile->fsh, FILESH_NO_SYNC);
  }
  raLock (rafile);
  rrc = 1;
  fileSharedSeek (rafile->fsh, 0L, SEEK_SET);
  rv = fileSharedGet (rafile->fsh, buff, sizeof (buff) - 1);
  if (rv != NULL && sscanf (buff, "#VERSION=%d\n", &version) == 1) {
    rafile->version = version;
    rv = fileSharedGet (rafile->fsh, buff, sizeof (buff) - 1);
    if (rv != NULL) {
      rv = fileSharedGet (rafile->fsh, buff, sizeof (buff) - 1);
      if (rv != NULL && sscanf (buff, "#RASIZE=%d\n", &rasize) == 1) {
        rafile->size = rasize;
        rv = fileSharedGet (rafile->fsh, buff, sizeof (buff) - 1);
        if (rv != NULL && sscanf (buff, "#RACOUNT=%" PRId32 "\n", &count) == 1) {
          rafile->count = count;
          rrc = 0;
        }
      }
    }
  }
  raUnlock (rafile);
  logProcEnd ("");
  return rrc;
}

static void
raWriteHeader (rafile_t *rafile, int version)
{
  char    tbuff [80];
  int     rc;

  if (rafile == NULL) {
    return;
  }

  if (rafile->openmode == FILESH_OPEN_READ) {
    return;
  }

  logProcBegin ();
  /* locks are handled by the caller */
  fileSharedSeek (rafile->fsh, 0L, SEEK_SET);
  rc = snprintf (tbuff, sizeof (tbuff), "#VERSION=%d\n", version);
  fileSharedWrite (rafile->fsh, tbuff, rc);
  rc = snprintf (tbuff, sizeof (tbuff), "# Do not edit this file.\n");
  fileSharedWrite (rafile->fsh, tbuff, rc);
  rc = snprintf (tbuff, sizeof (tbuff), "#RASIZE=%d\n", rafile->size);
  fileSharedWrite (rafile->fsh, tbuff, rc);
  rc = snprintf (tbuff, sizeof (tbuff), "#RACOUNT=%" PRId32 "\n", rafile->count);
  fileSharedWrite (rafile->fsh, tbuff, rc);
  if (! rafile->inbatch) {
    fileSharedFlush (rafile->fsh, FILESH_SYNC);
  }
  logProcEnd ("");
}

static void
raLock (rafile_t *rafile)
{
  int     rc;
  int     count;

  logProcBegin ();
  if (rafile->inbatch) {
    logProcEnd ("is-in-batch");
    return;
  }
  if (rafile->locked) {
    logProcEnd ("already");
    return;
  }

  /* the music database may be shared across multiple processes */
  rc = lockAcquire (RAFILE_LOCK_FN, PATHBLD_MP_NONE);
  count = 0;
  while (rc < 0 && count < 10) {
    mssleep (50);
    rc = lockAcquire (RAFILE_LOCK_FN, PATHBLD_MP_NONE);
    ++count;
  }
  if (rc < 0 && count >= 10) {
    /* ### FIX */
    /* global failure; stop everything */
  }
  rafile->locked = 1;
  logProcEnd ("");
}

static void
raUnlock (rafile_t *rafile)
{
  logProcBegin ();
  if (rafile->inbatch) {
    logProcEnd ("is-in-batch");
    return;
  }
  if (! rafile->locked) {
    logProcEnd ("not-locked");
    return;
  }

  lockRelease (RAFILE_LOCK_FN, PATHBLD_MP_NONE);
  rafile->locked = 0;
  logProcEnd ("");
}

static inline size_t
rrnToOffset (rafileidx_t rrn) {
 return ((rrn - 1) * RAFILE_REC_SIZE + RAFILE_HDR_SIZE);
}

/* in use as of 4.14.0 */
bool
raClear (rafile_t *rafile, rafileidx_t rrn)
{
  if (rafile == NULL) {
    return false;
  }

  if (rafile->openmode == FILESH_OPEN_READ) {
    return false;
  }

  logProcBegin ();
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad rrn %" PRId32, rrn);
    return false;
  }
  raLock (rafile);
  fileSharedSeek (rafile->fsh, rrnToOffset (rrn), SEEK_SET);
  fileSharedWrite (rafile->fsh, ranulls, RAFILE_REC_SIZE);
  if (! rafile->inbatch) {
    fileSharedFlush (rafile->fsh, FILESH_SYNC);
  }
  raUnlock (rafile);
  logProcEnd ("");
  return true;
}

/* for debugging only */

rafileidx_t
raGetSize (rafile_t *rafile) /* TESTING */
{
  return rafile->size;
}

rafileidx_t
raGetVersion (rafile_t *rafile)   /* TESTING */
{
  return rafile->version;
}

rafileidx_t
raGetNextRRN (rafile_t *rafile)   /* TESTING */
{
  raLock (rafile);
  return (rafile->count + 1L);
}

