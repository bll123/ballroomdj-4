#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "tmutil.h"

static bdjlog_t *syslogs [LOG_MAX];

bdjlog_t *
logOpen (char *fn)
{
  bdjlog_t      *l;

  l = malloc (sizeof (bdjlog_t));
  assert (l != NULL);

  l->fh = NULL;
  l->loggingOn = 0;

  l->fh = fopen (fn, "a");
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
  if (l == NULL || l->fh == NULL || ! l->loggingOn) {
    return;
  }

  logVarMsg (idx, msg, "err: %d %s", err, strerror (err));
}

void
logMsg (logidx_t idx, char *msg)
{
  bdjlog_t      *l;

  l = syslogs [idx];
  if (l == NULL || l->fh == NULL || ! l->loggingOn) {
    return;
  }

  logVarMsg (idx, msg, NULL);
}

void
logVarMsg (logidx_t idx, char *msg, char *fmt, ...)
{
  bdjlog_t      *l;
  char          tbuff [40];
  va_list       args;

  l = syslogs [idx];
  if (l == NULL || l->fh == NULL || ! l->loggingOn) {
    return;
  }

  dstamp (tbuff, sizeof (tbuff));
  fprintf (l->fh, "%s %s", tbuff, msg);
  if (fmt != NULL) {
    va_start (args, fmt);
    vfprintf (l->fh, fmt, args);
    va_end (args);
  }
  fprintf (l->fh, "\n");
  fflush (l->fh);
}

void
logStart (void)
{
  syslogs [LOG_ERR] = NULL;
  syslogs [LOG_SESS] = NULL;
  syslogs [LOG_ERR] = logOpen ("data/logerror.txt");
  syslogs [LOG_SESS] = logOpen ("data/logsession.txt");
  logMsg (LOG_ERR, "=== started");
  logMsg (LOG_SESS, "=== started");
}

void
logEnd (void)
{
  logClose (LOG_ERR);
  logClose (LOG_SESS);
  syslogs [LOG_ERR] = NULL;
  syslogs [LOG_SESS] = NULL;
}

