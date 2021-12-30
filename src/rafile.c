#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "rafile.h"
#include "lock.h"
#include "tmutil.h"
#include "log.h"
#include "portability.h"

static int  raReadHeader (rafile_t *);
static void raWriteHeader (rafile_t *, int);
static void raLock (rafile_t *);
static void raUnlock (rafile_t *);

static char ranulls [RAFILE_REC_SIZE];

rafile_t *
raOpen (char *fname, int version)
{
  struct stat     statbuf;
  rafile_t        *rafile;
  int             frc;
  int             rc;
  char            *mode;

  logProcBegin ("raOpen");
  rafile = malloc (sizeof (rafile_t));
  assert (rafile != NULL);
  rafile->fh = NULL;

  frc = stat (fname, &statbuf);
  mode = "r+";
  if (frc != 0) {
    mode = "w+";
  }

  rafile->fh = fopen (fname, mode);
  rafile->inbatch = 0;
  rafile->locked = 0;
  rafile->version = version;
  rafile->size = RAFILE_REC_SIZE;

  if (statbuf.st_size == 0L) {
    frc = 1;
  }
  if (frc == 0) {
    rc = raReadHeader (rafile);

    if (rc != 0) {
      /* probably an incorrect filename   */
      /* don't try to do anything with it */
      free (rafile);
      logProcEnd ("raOpen", "bad-header");
      return NULL;
    }
  }
  if (frc != 0) {
    rafile->count = 0L;
    raLock (rafile);
    raWriteHeader (rafile, version);
    raUnlock (rafile);
  }
  rafile->fname = strdup (fname);
  assert (rafile->fname != NULL);
  memset (ranulls, 0, RAFILE_REC_SIZE);
  logProcEnd ("raOpen", "");
  return rafile;
}

void
raClose (rafile_t *rafile)
{
  logProcBegin ("raClose");
  if (rafile != NULL) {
    fclose (rafile->fh);
    raUnlock (rafile);
    rafile->fh = NULL;
    if (rafile->fname != NULL) {
      free (rafile->fname);
    }
    free (rafile);
  }
  logProcEnd ("raClose", "");
}

size_t
raGetCount (rafile_t *rafile)
{
  return rafile->count;
}

size_t
raGetNextRRN (rafile_t *rafile)
{
  raLock (rafile);
  return (rafile->count + 1L);
}

void
raStartBatch (rafile_t *rafile)
{
  logProcBegin ("raStartBatch");
  raLock (rafile);
  rafile->inbatch = 1;
  logProcEnd ("raStartBatch", "");
}

void
raEndBatch (rafile_t *rafile)
{
  logProcBegin ("raEndBatch");
  rafile->inbatch = 0;
  raUnlock (rafile);
  logProcEnd ("raEndBatch", "");
}

int
raWrite (rafile_t *rafile, size_t rrn, char *data)
{
  size_t len = strlen (data);

  logProcBegin ("raWrite");
  /* leave one byte for the terminating null */
  if (len > (RAFILE_REC_SIZE - 1)) {
    logMsg (LOG_DBG, "bad data len", "%ld", len);
    return 1;
  }

  raLock (rafile);
  if (rrn == RAFILE_NEW) {
    ++rafile->count;
    rrn = rafile->count;
    raWriteHeader (rafile, rafile->version);
  } else {
    if (rrn > rafile->count) {
      rafile->count = rrn;
      raWriteHeader (rafile, rafile->version);
    }
  }
  fseek (rafile->fh, RRN_TO_OFFSET (rrn), SEEK_SET);
  fwrite (ranulls, RAFILE_REC_SIZE, 1, rafile->fh);
  fseek (rafile->fh, RRN_TO_OFFSET (rrn), SEEK_SET);
  fwrite (data, len, 1, rafile->fh);
  fflush (rafile->fh);
  raUnlock (rafile);
  logProcEnd ("raWrite", "");
  return 0;
}

int
raClear (rafile_t *rafile, size_t rrn)
{
  logProcBegin ("raClear");
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, "bad rrn", "%ld", rrn);
    return 1;
  }
  raLock (rafile);
  fseek (rafile->fh, RRN_TO_OFFSET (rrn), SEEK_SET);
  fwrite (ranulls, RAFILE_REC_SIZE, 1, rafile->fh);
  raUnlock (rafile);
  logProcEnd ("raClear", "");
  return 0;
}

size_t
raRead (rafile_t *rafile, size_t rrn, char *data)
{
  size_t        rc;

  logProcBegin ("raRead");
  if (rrn < 1L || rrn > rafile->count) {
    logMsg (LOG_DBG, "bad rrn", "%ld", rrn);
    return 0;
  }

  raLock (rafile);
  fseek (rafile->fh, RRN_TO_OFFSET (rrn), SEEK_SET);
  rc = fread (data, RAFILE_REC_SIZE, 1, rafile->fh);
  raUnlock (rafile);
  logProcEnd ("raRead", "");
  return rc;
}

/* local routines */

static int
raReadHeader (rafile_t *rafile)
{
  char      buff [90];
  char      tfmt [20];
  char      *rv;
  int       rrc;
  int       version;
  int       rasize;
  size_t    count;

  logProcBegin ("raReadHeader");
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
        snprintf (tfmt, sizeof (tfmt), "#RACOUNT=%s\n", SIZE_FMT);
        if (rv != NULL && sscanf (buff, tfmt, &count) == 1) {
          rafile->count = count;
          rrc = 0;
        }
      }
    }
  }
  raUnlock (rafile);
  logProcEnd ("raReadHeader", "");
  return rrc;
}

static void
raWriteHeader (rafile_t *rafile, int version)
{
  char    tfmt [20];

  logProcBegin ("raWriteHeader");
  /* locks are handled by the caller */
  /* the header never gets smaller, so it's not necessary to flush its data */
  fseek (rafile->fh, 0L, SEEK_SET);
  fprintf (rafile->fh, "#VERSION=%d\n", version);
  fprintf (rafile->fh, "# Do not edit this file.\n");
  fprintf (rafile->fh, "#RASIZE=%d\n", rafile->size);
  snprintf (tfmt, sizeof (tfmt), "#RACOUNT=%s\n", SIZE_FMT);
  fprintf (rafile->fh, tfmt, rafile->count);
  fflush (rafile->fh);
  logProcEnd ("raWriteHeader", "");
}

static void
raLock (rafile_t *rafile)
{
  int     rc;
  int     count;

  logProcBegin ("raLock");
  if (rafile->inbatch) {
    logProcEnd ("raLock", "in-batch");
    return;
  }

  /* the music database may be shared across multiple processes */
  rc = lockAcquire (RAFILE_LOCK_FN);
  count = 0;
  while (rc < 0 && count < 10) {
    msleep (50);
    rc = lockAcquire (RAFILE_LOCK_FN);
    ++count;
  }
  if (rc < 0 && count >= 10) {
    /* ### FIX */
    /* global failure; stop everything */
  }
  rafile->locked = 1;
  logProcEnd ("raLock", "");
}

static void
raUnlock (rafile_t *rafile)
{
  logProcBegin ("raUnlock");
  if (rafile->inbatch) {
    logProcEnd ("raUnlock", "in-batch");
    return;
  }

  lockRelease (RAFILE_LOCK_FN);
  rafile->locked = 0;
  logProcEnd ("raUnlock", "");
}
