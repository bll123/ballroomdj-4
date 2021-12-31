#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _hdr_fcntl
# include <fcntl.h>
#endif
#if _hdr_unistd
# include <unistd.h>
#endif

#include "log.h"
#include "tmutil.h"
#include "fileutil.h"
#include "sysvars.h"
#include "portability.h"

static void         rlogStart (const char *processnm,
                        const char *processtag, int truncflag);
static bdjlog_t *   rlogOpen (const char *fn,
                        const char *processtag, int truncflag);
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
  bdjlog_t *l = rlogOpen (fn, processtag, 1);
  return l;
}

bdjlog_t *
logOpenAppend (const char *fn, const char *processtag)
{
  bdjlog_t *l = rlogOpen (fn, processtag, 0);
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

  close (l->fd);
  l->fd = -1;
  free (l);
}

void
rlogProcBegin (const char *tag, const char *fn, int line)
{
  rlogVarMsg (LOG_DBG, fn, line, "- %s begin", tag);
  syslogs [LOG_DBG]->indent += 2;
}

void
rlogProcEnd (const char *tag, const char *suffix, const char *fn, int line)
{
  syslogs [LOG_DBG]->indent -= 2;
  if (syslogs [LOG_DBG]->indent < 0) {
    syslogs [LOG_DBG]->indent = 0;
  }
  rlogVarMsg (LOG_DBG, fn, line, "- %s end %s", tag, suffix);
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
  int           fd;
  bdjlog_t      *l;
  char          ttm [40];
  char          tbuff [512];
  char          wbuff [1024];
  char          tfn [MAXPATHLEN];
  va_list       args;

  l = syslogs [idx];
  fd = STDERR_FILENO;
  if (! stderrLogging && (l == NULL || l->fd == -1)) {
    return;
  }
  if (! stderrLogging && l != NULL && l->fd != -1) {
    fd = l->fd;
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
    snprintf (tfn, MAXPATHLEN, "(%s: %d)", logTail (fn), line);
  }
  size_t wlen = snprintf (wbuff, sizeof (wbuff), "%s: %-2s %*s%s %s\n",
      ttm, l->processTag, l->indent, "", tbuff, tfn);
  if (write (fd, wbuff, wlen) < 0) {
    fprintf (stderr, "log write failed: %d %s\n", errno, strerror (errno));
  }
}

void
logStart (const char *processtag)
{
  rlogStart (NULL, processtag, 1);
}

void
logStartAppend (const char *processnm, const char *processtag)
{
  rlogStart (processnm, processtag, 0);
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
rlogStart (const char *processnm, const char *processtag, int truncflag)
{
  char      tnm [MAXPATHLEN];
  char      tdt [40];

  logInit ();
  dstamp (tdt, sizeof (tdt));

  for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
    fileMakePath (tnm, MAXPATHLEN, "", logbasenm [idx], LOG_EXTENSION,
        FILE_MP_HOSTNAME | FILE_MP_USEIDX);
    syslogs [idx] = rlogOpen (tnm, processtag, truncflag);
    if (processnm != NULL) {
      rlogVarMsg (idx, NULL, 0, "=== %s started %s", processnm, tdt);
    } else {
      rlogVarMsg (idx, NULL, 0, "=== started %s", tdt);
    }
  }
}

static bdjlog_t *
rlogOpen (const char *fn, const char *processtag, int truncflag)
{
  bdjlog_t      *l;
  int           flags;

  l = malloc (sizeof (bdjlog_t));
  assert (l != NULL);

  l->fd = -1;
  l->indent = 0;
  l->processTag = processtag;
  flags = O_WRONLY | O_APPEND | O_SYNC | O_CREAT | O_CLOEXEC;
  if (truncflag) {
    flags |= O_TRUNC;
  }
  l->fd = open (fn, flags);
  if (l->fd < 0) {
    fprintf (stderr, "Unable to open %s %d %s\n", fn, errno, strerror (errno));
  }
  return l;
}

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
