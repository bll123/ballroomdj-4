/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#if _hdr_execinfo
# include <execinfo.h>
#endif

#include "bdj4.h"
#include "dirop.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"
#include "fileop.h"
#include "filemanip.h"
#include "fileshared.h"
#include "pathbld.h"
#include "pathutil.h"
#include "player.h"
#include "bdjstring.h"

/* for debugging and for test suite */
static const char *playerstateTxt [PL_STATE_MAX] = {
  [PL_STATE_UNKNOWN] = "unknown",
  [PL_STATE_STOPPED] = "stopped",
  [PL_STATE_LOADING] = "loading",
  [PL_STATE_PLAYING] = "playing",
  [PL_STATE_PAUSED] = "paused",
  [PL_STATE_IN_FADEOUT] = "in-fadeout",
  [PL_STATE_IN_GAP] = "in-gap",
};

/* for debugging */
static const char *stateTxt [BDJ4_STATE_MAX] = {
  [BDJ4_STATE_OFF] = "off",
  [BDJ4_STATE_START] = "start",
  [BDJ4_STATE_WAIT] = "wait",
  [BDJ4_STATE_PROCESS] = "process",
  [BDJ4_STATE_FINISH] = "finish",
};

typedef struct bdjlog {
  fileshared_t  *fhandle;
  int           opened;
  int           indent;
  ssize_t       level;
  const char    *processTag;
} bdjlog_t;

static void rlogStart (const char *processnm, const char *processtag, int truncflag, loglevel_t level);
static void rlogOpen (logidx_t idx, const char *fn, const char *processtag, int truncflag);
static void logInit (void);
static void logAlloc (void);
static const char * logTail (const char *fn);

static bdjlog_t *syslogs [LOG_MAX];
static char * logbasenm [LOG_MAX];
static int  initialized = 0;
static int  logsalloced = 0;

void
logClose (logidx_t idx)
{
  bdjlog_t      *l;

  logInit ();
  l = syslogs [idx];
  if (l == NULL) {
    return;
  }
  if (! l->opened) {
    return;
  }

  fileSharedClose (l->fhandle);
  l->opened = 0;
}

void
rlogProcBegin (loglevel_t level, const char *tag, const char *fn, int line)
{
  if (! logCheck (LOG_DBG, level)) {
    return;
  }
  rlogVarMsg (LOG_DBG, level, fn, line, "- %s begin", tag);
  syslogs [LOG_DBG]->indent += 2;
}

void
rlogProcEnd (loglevel_t level, const char *tag, const char *suffix,
    const char *fn, int line)
{
  if (! logCheck (LOG_DBG, level)) {
    return;
  }
  syslogs [LOG_DBG]->indent -= 2;
  if (syslogs [LOG_DBG]->indent < 0) {
    syslogs [LOG_DBG]->indent = 0;
  }
  rlogVarMsg (LOG_DBG, level, fn, line, "- %s end %s", tag, suffix);
}

void
rlogError (const char *msg, int err, const char *fn, int line)
{
  if (syslogs [LOG_ERR] == NULL) {
    return;
  }
  rlogVarMsg (LOG_ERR, LOG_IMPORTANT, fn, line,
      "err: %s %d %s", msg, err, strerror (err));
  rlogVarMsg (LOG_DBG, LOG_IMPORTANT, fn, line,
      "err: %s %d %s", msg, err, strerror (err));
}

void
rlogVarMsg (logidx_t idx, loglevel_t level,
    const char *fn, int line, const char *fmt, ...)
{
  bdjlog_t      *l;
  char          ttm [40];
  char          tbuff [LOG_MAX_BUFF];
  char          wbuff [LOG_MAX_BUFF];
  char          tfn [MAXPATHLEN];
  va_list       args;
  size_t        wlen;


  logInit ();

  l = syslogs [idx];
  if (l == NULL) {
    return;
  }
  if (! l->opened) {
    return;
  }

  if ((l->level & LOG_REDIR_INST) == LOG_REDIR_INST) {
    idx = LOG_INSTALL;
    l = syslogs [idx];
  }

  if (! logCheck (idx, level)) {
    return;
  }

  tmutilTstamp (ttm, sizeof (ttm));
  *tbuff = '\0';
  *tfn = '\0';
  if (fmt != NULL) {
    va_start (args, fmt);
    vsnprintf (tbuff, sizeof (tbuff), fmt, args);
    va_end (args);
  }
  if (fn != NULL) {
    snprintf (tfn, sizeof (tfn), "(%s: %d)", logTail (fn), line);
  }
  wlen = (size_t) snprintf (wbuff, sizeof (wbuff),
      "%s: %-4s %*s%s %s\n", ttm, l->processTag, l->indent, "", tbuff, tfn);
  wlen = wlen > LOG_MAX_BUFF ? LOG_MAX_BUFF - 1 : wlen;
  fileSharedWrite (l->fhandle, wbuff, wlen);
  if (idx == LOG_ERR) {
    l = syslogs [LOG_DBG];
    if (l->opened) {
      fileSharedWrite (l->fhandle, wbuff, wlen);
    }
  }
}

void
logSetLevelAll (loglevel_t level)
{
  logInit ();
  for (logidx_t idx = 0; idx < LOG_MAX; ++idx) {
    syslogs [idx]->level = level;
  }
}

void
logSetLevel (logidx_t idx, loglevel_t level, const char *processtag)
{
  logInit ();
  syslogs [idx]->level = level;
  syslogs [idx]->processTag = processtag;
}

/* these routines act upon all open logs */

void
logStart (const char *processnm, const char *processtag, loglevel_t level)
{
  logInit ();
  rlogStart (processnm, processtag, FILE_OPEN_TRUNCATE, level);
}

void
logStartAppend (const char *processnm, const char *processtag, loglevel_t level)
{
  logInit ();
  rlogStart (processnm, processtag, FILE_OPEN_APPEND, level);
}

void
logEnd (void)
{
  logInit ();

  if (logsalloced) {
    for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
      if (syslogs [idx] != NULL) {
        logClose (idx);
        mdfree (syslogs [idx]);
        syslogs [idx] = NULL;
      }
    }
    logsalloced = false;
  }
}

bool
logCheck (logidx_t idx, loglevel_t level)
{
  bdjlog_t      *l;

  logInit ();
  l = syslogs [idx];

  if (l == NULL) {
    return false;
  }
  if (! l->opened) {
    return false;
  }
  if (! (level & l->level)) {
    return false;
  }
  return true;
}

/* for debugging */
const char *
logPlstateDebugText (playerstate_t plstate)
{
  if (plstate < 0 || plstate >= PL_STATE_MAX) {
    return "invalid";
  }

  return playerstateTxt [plstate];
}

const char *
logStateDebugText (int state)
{
  if (state < 0 || state >= BDJ4_STATE_MAX) {
    return "invalid";
  }

  return stateTxt [state];
}

enum {
  LOG_BACKTRACE_SIZE = 30,
};

void
logBacktraceHandler (int sig)
{
  logBacktrace ();
  exit (1);
}

void
logBacktrace (void)
{
#if _lib_backtrace
  void    *array [LOG_BACKTRACE_SIZE];
  char    **out;
  size_t  size;

  size = backtrace (array, LOG_BACKTRACE_SIZE);
  out = backtrace_symbols (array, size);
  for (size_t i = 0; i < size; ++i) {
    if (syslogs [LOG_ERR] != NULL) {
      syslogs [LOG_ERR]->level |= LOG_IMPORTANT;
      rlogVarMsg (LOG_ERR, LOG_IMPORTANT, "bt", 0, "bt: %2ld: %s", i, out [i]);
    } else {
      fprintf (stderr, "bt: %2ld: %s\n", i, out [i]);
    }
  }
  mdfree (out);
#endif
}

void
logBasic (const char *fmt, ...)
{
  char      ttm [40];
  FILE      *fh;
  va_list   args;

  tmutilTstamp (ttm, sizeof (ttm));
  fh = fileopOpen ("out.txt", "a");
  va_start (args, fmt);
  fprintf (fh, "%s ", ttm);
  vfprintf (fh, fmt, args);
  va_end (args);
  mdextfclose (fh);
  fclose (fh);
}

void
logStderr (const char *fmt, ...)
{
  char      ttm [40];
  va_list   args;

  tmutilTstamp (ttm, sizeof (ttm));
  va_start (args, fmt);
  fprintf (stderr, "%s ", ttm);
  vfprintf (stderr, fmt, args);
  va_end (args);
}

/* internal routines */

static void
rlogStart (const char *processnm, const char *processtag,
    int truncflag, loglevel_t level)
{
  char      tnm [MAXPATHLEN];
  char      tdt [40];

  logInit ();
  tmutilDstamp (tdt, sizeof (tdt));

  for (logidx_t idx = 0; idx < LOG_MAX; ++idx) {
    pathbldMakePath (tnm, sizeof (tnm), logbasenm [idx], LOG_EXTENSION,
        PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX);
    rlogOpen (idx, tnm, processtag, truncflag);
    syslogs [idx]->level = level;
    rlogVarMsg (idx, LOG_IMPORTANT, NULL, 0, "=== %s started %s pid %ld", processnm, tdt, (long) getpid ());
  }
}

static void
rlogOpen (logidx_t idx, const char *fn, const char *processtag, int truncflag)
{
  bdjlog_t      *l = NULL;
  pathinfo_t    *pi;
  char          tbuff [MAXPATHLEN];

  logInit ();
  logAlloc ();

  l = syslogs [idx];
  if (l == NULL) {
    return;
  }

  pi = pathInfo (fn);
  strlcpy (tbuff, pi->dirname, pi->dlen + 1);
  if ( ! fileopIsDirectory (tbuff)) {
    diropMakeDir (tbuff);
  }
  pathInfoFree (pi);

  if (truncflag == FILE_OPEN_TRUNCATE && idx != LOG_INSTALL) {
    filemanipBackup (fn, 2);
  }

  l->processTag = processtag;
  if (idx == LOG_INSTALL) {
    /* never truncate the installation log */
    truncflag = FILE_OPEN_APPEND;
  }
  l->fhandle = fileSharedOpen (fn, truncflag);
  if (l->fhandle == NULL) {
    fprintf (stderr, "%s: Unable to open %s %d %s\n",
        processtag, fn, errno, strerror (errno));
  }
  l->opened = 1;
}

static void
logInit (void)
{
  if (! initialized) {
    logbasenm [LOG_ERR] = LOG_ERROR_NAME;
    logbasenm [LOG_SESS] = LOG_SESSION_NAME;
    logbasenm [LOG_DBG] = LOG_DEBUG_NAME;
    logbasenm [LOG_INSTALL] = LOG_INSTALL_NAME;
    logbasenm [LOG_GTK] = LOG_GTK_NAME;
    for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
      syslogs [idx] = NULL;
    }
    initialized = 1;
  }
}

void
logAlloc (void)
{
  bdjlog_t      *l = NULL;

  if (! logsalloced) {
    for (logidx_t idx = LOG_ERR; idx < LOG_MAX; ++idx) {
      if (syslogs [idx] != NULL) {
        continue;
      }

      l = mdmalloc (sizeof (bdjlog_t));
      l->opened = 0;
      l->indent = 0;
      l->level = 0;
      l->processTag = "unkn";
      syslogs [idx] = l;
    }
    logsalloced = true;
  }
}

inline const char *
logTail (const char *fn)
{
  const char    *p;
  size_t        len;
  bool          fp = false;

  len = strlen (fn);
  if (*fn == '/' || *fn == '\\' || (len > 2 && fn [1] == ':')) {
    fp = true;
  }
  if (! fp) {
    return fn;
  }

  p = fn + len - 1;
  while (p != fn && *p != '/' && *p != '\\') {
    --p;
  }
  if (p != fn) {
    ++p;
  }
  return p;
}
