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
#include "portability.h"

static void         logInit (void);
static const char * logTail (const char *fn);

static bdjlog_t *syslogs [LOG_MAX];
static char *   logbasenm [LOG_MAX];
static int      stderrLogging = 0;
static int      initialized = 0;

void
logStderr (void)
{
  stderrLogging = 1;
}

bdjlog_t *
logOpen (const char *fn, const char *processtag)
{
  bdjlog_t      *l;

  l = malloc (sizeof (bdjlog_t));
  assert (l != NULL);

  l->fh = NULL;
  l->indent = 0;
  l->processTag = processtag;
  l->fh = fopen (fn, "w");
  if (l->fh == NULL) {
    fprintf (stderr, "Unable to open %s %d %s\n", fn, errno, strerror (errno));
  }
  return l;
}

bdjlog_t *
logOpenAppend (const char *fn, const char *processtag)
{
  bdjlog_t      *l;

  l = malloc (sizeof (bdjlog_t));
  assert (l != NULL);

  l->fh = NULL;

  l->indent = 0;
  l->processTag = processtag;
  l->fh = fopen (fn, "a");
  if (l->fh == NULL) {
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

  fclose (l->fh);
  l->fh = NULL;
  free (l);
}

void
rlogProcBegin (const char *tag, const char *fn, int line)
{
  rlogVarMsg (LOG_DBG, fn, line, "-- %s begin", tag);
  syslogs [LOG_DBG]->indent += 2;
}

void
rlogProcEnd (const char *tag, const char *suffix, const char *fn, int line)
{
  syslogs [LOG_DBG]->indent -= 2;
  if (syslogs [LOG_DBG]->indent < 0) {
    syslogs [LOG_DBG]->indent = 0;
  }
  rlogVarMsg (LOG_DBG, fn, line, "-- %s end %s", tag, suffix);
}

void
rlogError (const char *msg, int err, const char *fn, int line)
{
  rlogVarMsg (LOG_ERR, fn, line, "err: %s %d %s", msg, err, strerror (err));
  rlogVarMsg (LOG_DBG, fn, line, "err: %s %d %s", msg, err, strerror (err));
}

void
rlogVarMsg (logidx_t idx, const char *fn, int line, const char *fmt, ...)
{
  FILE          *fh;
  bdjlog_t      *l;
  char          ttm [40];
  char          tbuff [4096];
  char          tfn [MAXPATHLEN];
  va_list       args;

  l = syslogs [idx];
  fh = stderr;
  if (! stderrLogging && (l == NULL || l->fh == NULL)) {
    return;
  }
  if (! stderrLogging && l != NULL && l->fh != NULL) {
    fh = l->fh;
  }

  tstamp (ttm, sizeof (ttm));
  *tbuff = '\0';
  *tfn = '\0';
  if (fmt != NULL) {
    va_start (args, fmt);
    vsnprintf (tbuff, sizeof (tbuff), fmt, args);
    va_end (args);
  }
  if (fn != NULL) {
    snprintf (tfn, MAXPATHLEN, "(%s / %d)", logTail (fn), line);
  }
  fprintf (fh, "%s: %s %*s%s %s\n", ttm, l->processTag,
      l->indent, "", tbuff, tfn);
  fflush (fh);
}

void
logStart (const char *processtag)
{
  char      tnm [MAXPATHLEN];
  char      tdt [40];

  logInit ();

  dstamp (tdt, sizeof (tdt));

  for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
    fileMakePath (tnm, MAXPATHLEN, "", logbasenm [idx], LOG_EXTENSION,
        FILE_MP_HOSTNAME | FILE_MP_USEIDX);
    makeBackups (tnm, 1);
    syslogs [idx] = logOpen (tnm, processtag);
    rlogVarMsg (idx, NULL, 0, "=== started %s", tdt);
  }
}

void
logStartAppend (const char *processnm, const char *processtag)
{
  char      tnm [MAXPATHLEN];
  char      tdt [40];

  logInit ();
  dstamp (tdt, sizeof (tdt));

  for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
    fileMakePath (tnm, MAXPATHLEN, "", logbasenm [idx], LOG_EXTENSION,
        FILE_MP_HOSTNAME | FILE_MP_USEIDX);
    syslogs [idx] = logOpenAppend (tnm, processtag);
    rlogVarMsg (idx, NULL, 0, "=== started %s", tdt);
  }
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

/* internal routines */

static void
logInit (void)
{
  if (! initialized) {
    logbasenm [LOG_ERR] = LOG_ERROR_NAME;
    logbasenm [LOG_SESS] = LOG_SESSION_NAME;
    logbasenm [LOG_DBG] = LOG_DEBUG_NAME;
    initialized = 1;
  }
}

const char *
logTail (const char *fn)
{
  const char    *p;

  p = fn + strlen (fn) - 1;
  while (p != fn && *p != '/' && *p != '\\') {
    --p;
  }
  if (p != fn) {
    ++p;
  }
  return p;
}
