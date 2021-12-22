#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "bdjlog.h"
#include "tmutil.h"

FILE   *bdjlogfh = NULL;
int    loggingOn = 0;

void
startBDJLog (char *fn)
{
  bdjlogfh = fopen (fn, "a");
  if (bdjlogfh != NULL) {
    loggingOn = 1;
  }
  bdjlog ("=== started");
}

void
bdjlogError (char *msg, int err)
{
  if (! loggingOn) {
    return;
  }

  bdjlogMsg (msg, "err: %d %s", err, strerror (err));
}

void
bdjlog (char *msg)
{
  if (! loggingOn) {
    return;
  }

  bdjlogMsg (msg, NULL, 0);
}

void
bdjlogMsg (char *msg, char *fmt, int argcount, ...)
{
  char      tbuff [40];
  va_list   valist;

  if (! loggingOn) {
    return;
  }

  dstamp (tbuff, sizeof (tbuff));
  fprintf (bdjlogfh, "%s %s", tbuff, msg);
  if (fmt != NULL && argcount > 0) {
    va_start (valist, argcount);
    vfprintf (bdjlogfh, fmt, valist);
  }
  fprintf (bdjlogfh, "\n");
  fflush (bdjlogfh);
}
