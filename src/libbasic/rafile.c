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
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "rafile.h"
#include "tmutil.h"

typedef struct rafile {
  FILE          *fh;
  char          *fname;
  int           version;
  rafileidx_t   size;
  rafileidx_t   count;
  unsigned int  inbatch;
  unsigned int  locked;
} rafile_t;

static char ranulls [RAFILE_REC_SIZE];

static int  raReadHeader (rafile_t *);
static void raWriteHeader (rafile_t *, int);
static void raLock (rafile_t *);
static void raUnlock (rafile_t *);
static size_t rrnToOffset (rafileidx_t rrn);

rafile_t *
raOpen (char *fname, int version)
{
  rafile_t        *rafile;
  bool            fexists;
  int             rc;
  char            *mode;

  logProcBegin ();
  rafile = mdmalloc (sizeof (rafile_t));
  rafile->fh = NULL;

  fexists = fileopFileExists (fname);
  mode = "rb+";
  if (! fexists) {
    mode = "wb+";
  }

  rafile->fh = fileopOpen (fname, mode);
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
  logProcBegin ();
  if (rafile != NULL) {
    mdextfclose (rafile->fh);
    fclose (rafile->fh);
    raUnlock (rafile);
    rafile->fh = NULL;
    dataFree (rafile->fname);
    mdfree (rafile);
  }
  logProcEnd ("");
}

rafileidx_t
raGetCount (rafile_t *rafile)
{
  return rafile->count;
}

void
raStartBatch (rafile_t *rafile)
{
  logProcBegin ();
  raLock (rafile);
  rafile->inbatch = 1;
  fflush (rafile->fh);
  logProcEnd ("");
}

void
raEndBatch (rafile_t *rafile)
{
  logProcBegin ();
  rafile->inbatch = 0;
  fflush (rafile->fh);
  fileopSync (rafile->fh);
  raUnlock (rafile);
  logProcEnd ("");
}

size_t
raWrite (rafile_t *rafile, rafileidx_t rrn, char *data, ssize_t len)
{
  bool    isnew = false;

  if (len == -1) {
    len = strlen (data);
  }
  logProcBegin ();
  /* leave one byte for the terminating null */
  if (len > (RAFILE_REC_SIZE - 1)) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad data len %" PRId64, (int64_t) len);
    return 0;
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
    fseek (rafile->fh, rrnToOffset (rrn), SEEK_SET);
    fwrite (ranulls, RAFILE_REC_SIZE, 1, rafile->fh);
  }
  fseek (rafile->fh, rrnToOffset (rrn), SEEK_SET);
  fwrite (data, len, 1, rafile->fh);
  if (isnew) {
    /* write one null byte to the next record so */
    /* that the last record has a size and is readable */
    fseek (rafile->fh, rrnToOffset (rrn + 1), SEEK_SET);
    fwrite (ranulls, 1, 1, rafile->fh);
  }
  if (! rafile->inbatch) {
    fflush (rafile->fh);
    fileopSync (rafile->fh);
  }
  raUnlock (rafile);
  logProcEnd ("");

  return len;
}

rafileidx_t
raRead (rafile_t *rafile, rafileidx_t rrn, char *data)
{
  rafileidx_t        rc;

  logProcBegin ();
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad rrn %" PRId32, rrn);
    return 0;
  }

  /* as there are multiple processes, */
  /* the reader's buffers must also be flushed */
  if (! rafile->inbatch) {
    fflush (rafile->fh);
  }
  raLock (rafile);
  fseek (rafile->fh, rrnToOffset (rrn), SEEK_SET);
  rc = (rafileidx_t) fread (data, RAFILE_REC_SIZE, 1, rafile->fh);
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

  logProcBegin ();
  /* as there are multiple processes, */
  /* the reader's buffers must also be flushed */
  if (! rafile->inbatch) {
    fflush (rafile->fh);
  }
  raLock (rafile);
  rrc = 1;
  fseek (rafile->fh, 0L, SEEK_SET);
  rv = fgets (buff, sizeof (buff) - 1, rafile->fh);
  if (rv != NULL && sscanf (buff, "#VERSION=%d\n", &version) == 1) {
    rafile->version = version;
    rv = fgets (buff, sizeof (buff) - 1, rafile->fh);
    if (rv != NULL) {
      rv = fgets (buff, sizeof (buff) - 1, rafile->fh);
      if (rv != NULL && sscanf (buff, "#RASIZE=%d\n", &rasize) == 1) {
        rafile->size = rasize;
        rv = fgets (buff, sizeof (buff) - 1, rafile->fh);
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
  logProcBegin ();
  /* locks are handled by the caller */
  /* the header never gets smaller, so it's not necessary to flush its data */
  fseek (rafile->fh, 0L, SEEK_SET);
  fprintf (rafile->fh, "#VERSION=%d\n", version);
  fprintf (rafile->fh, "# Do not edit this file.\n");
  fprintf (rafile->fh, "#RASIZE=%d\n", rafile->size);
  fprintf (rafile->fh, "#RACOUNT=%" PRId32 "\n", rafile->count);
  if (! rafile->inbatch) {
    fflush (rafile->fh);
    fileopSync (rafile->fh);
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

/* for debugging only */

/* this function is not in use, but keep the code, as it works */
int
raClear (rafile_t *rafile, rafileidx_t rrn)   /* TESTING */
{
  logProcBegin ();
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, LOG_RAFILE, "bad rrn %" PRId32, rrn);
    return 1;
  }
  raLock (rafile);
  fseek (rafile->fh, rrnToOffset (rrn), SEEK_SET);
  fwrite (ranulls, RAFILE_REC_SIZE, 1, rafile->fh);
  if (! rafile->inbatch) {
    fflush (rafile->fh);
    fileopSync (rafile->fh);
  }
  raUnlock (rafile);
  logProcEnd ("");
  return 0;
}

rafileidx_t
raGetSize (rafile_t *rafile) /* TESTING */
{
  return rafile->size;
}

rafileidx_t
raGetVersion (rafile_t *rafile) /* TESTING */
{
  return rafile->version;
}

rafileidx_t
raGetNextRRN (rafile_t *rafile)  /* TESTING */
{
  raLock (rafile);
  return (rafile->count + 1L);
}

