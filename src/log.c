#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "tmutil.h"
#include "fileutil.h"
#include "sysvars.h"

static bdjlog_t *syslogs [LOG_MAX];

static int      stderrLogging = 0;

void
logStderr (void)
{
fprintf (stderr, "start stderr\n"); fflush (stdout);
  stderrLogging = 1;
}

void
logDebugOn (bdjlog_t *l)
{
  if (l != NULL) {
    l->debugOn = 1;
  }
}

bdjlog_t *
logOpen (char *fn)
{
  bdjlog_t      *l;

  l = malloc (sizeof (bdjlog_t));
  assert (l != NULL);

  l->fh = NULL;
  l->loggingOn = 0;
  l->debugOn = 0;

  l->fh = fopen (fn, "w");
  if (l->fh != NULL) {
    l->loggingOn = 1;
  } else {
    fprintf (stderr, "Unable to open %s %d %s\n", fn, errno, strerror (errno));
  }
  return l;
}

void
logClose (logidx_t idx)
{
  bdjlog_t      *l;

  l = syslogs [idx];
  if (l == NULL) {
    return;
  }

  l->loggingOn = 0;
  fclose (l->fh);
  l->fh = NULL;
  free (l);
}

void
logError (logidx_t idx, char *msg, int err)
{
  bdjlog_t      *l;

  l = syslogs [idx];
  if (! stderrLogging && (l == NULL || l->fh == NULL || ! l->loggingOn)) {
fprintf (stderr, "log-a\n"); fflush (stdout);
    return;
  }

  logVarMsg (idx, msg, "err: %d %s", err, strerror (err));
}

void
logMsg (logidx_t idx, char *msg)
{
  bdjlog_t      *l;

  l = syslogs [idx];
  if (! stderrLogging && (l == NULL || l->fh == NULL || ! l->loggingOn)) {
fprintf (stderr, "log-b\n"); fflush (stdout);
    return;
  }

  logVarMsg (idx, msg, NULL);
}

void
logVarMsg (logidx_t idx, char *msg, char *fmt, ...)
{
  FILE          *fh;
  bdjlog_t      *l;
  char          ttm [40];
  char          tbuff [2048];
  va_list       args;

  l = syslogs [idx];
  fh = stderr;
  if (! stderrLogging && (l == NULL || l->fh == NULL || ! l->loggingOn)) {
fprintf (stderr, "log-c\n"); fflush (stdout);
    return;
  }
  if (! stderrLogging && l != NULL && l->fh != NULL && l->loggingOn) {
fprintf (stderr, "log-d\n"); fflush (stdout);
    fh = l->fh;
  }
  if (! stderrLogging && idx == LOG_DBG && l != NULL && ! l->debugOn) {
fprintf (stderr, "log-e\n"); fflush (stdout);
    return;
  }

  dstamp (ttm, sizeof (ttm));
  *tbuff = '\0';
  if (fmt != NULL) {
    va_start (args, fmt);
    vsnprintf (tbuff, sizeof (tbuff), fmt, args);
    va_end (args);
  }
  fprintf (fh, "%s %s %s\n", ttm, msg, tbuff);
  fflush (fh);
}

void
logStart (void)
{
  char      tnm [MAXPATHLEN];
  char      idxtag [20];

  *idxtag = '\0';
  if (lsysvars [SVL_BDJIDX] != 0) {
    snprintf (idxtag, sizeof (idxtag), "-%ld", lsysvars [SVL_BDJIDX]);
  }

  snprintf (tnm, MAXPATHLEN, LOG_ERROR_NAME,
      sysvars [SV_HOSTNAME], idxtag, LOG_EXTENSION);
  makeBackups (tnm, 1);
  syslogs [LOG_ERR] = NULL;
  syslogs [LOG_ERR] = logOpen (tnm);
  logMsg (LOG_ERR, "=== started");

  snprintf (tnm, MAXPATHLEN, LOG_SESSION_NAME,
      sysvars [SV_HOSTNAME], idxtag, LOG_EXTENSION);
  makeBackups (tnm, 1);
  syslogs [LOG_SESS] = NULL;
  syslogs [LOG_SESS] = logOpen (tnm);
  logMsg (LOG_SESS, "=== started");

  snprintf (tnm, MAXPATHLEN, LOG_DEBUG_NAME,
      sysvars [SV_HOSTNAME], idxtag, LOG_EXTENSION);
  makeBackups (tnm, 1);
  syslogs [LOG_DBG] = NULL;
  syslogs [LOG_DBG] = logOpen (tnm);
  logMsg (LOG_DBG, "=== started");
}

void
logEnd (void)
{
  logClose (LOG_ERR);
  logClose (LOG_SESS);
  logClose (LOG_DBG);
  syslogs [LOG_ERR] = NULL;
  syslogs [LOG_SESS] = NULL;
  syslogs [LOG_DBG] = NULL;
}

