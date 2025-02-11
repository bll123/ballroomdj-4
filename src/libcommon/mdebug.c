/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */

/* this code is not thread safe */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#if _hdr_execinfo
# include <execinfo.h>
#endif

#include "bdjstring.h"
#include "mdebug.h"

typedef enum {
  MDEBUG_TYPE_ALLOC = 'a',
  MDEBUG_TYPE_REALLOC = 'r',
  MDEBUG_TYPE_STRDUP = 's',
  MDEBUG_TYPE_FREE = 'f',
  MDEBUG_TYPE_EXT_ALLOC = 'E',
  MDEBUG_TYPE_EXT_FREE = 'F',
  MDEBUG_TYPE_EXT_OPEN = 'o',
  MDEBUG_TYPE_EXT_SOCK = 'S',
  MDEBUG_TYPE_EXT_CLOSE = 'c',
  MDEBUG_TYPE_EXT_FOPEN = 'O',
  MDEBUG_TYPE_EXT_FCLOSE = 'C',
} mdebugtype_t;

enum {
  MDEBUG_ALLOC_BUMP = 1000,
  MDEBUG_BACKTRACE_SIZE = 30,
};

enum {
  MDEBUG_COUNT,
  MDEBUG_INT_ALLOC,
  MDEBUG_COUNT_MAX,
  MDEBUG_EXTALLOC,
  MDEBUG_EXTFREE,
  MDEBUG_MALLOC,
  MDEBUG_REALLOC,
  MDEBUG_STRDUP,
  MDEBUG_FREE,
  MDEBUG_ERRORS,
  MDEBUG_OPEN,
  MDEBUG_SOCK,
  MDEBUG_CLOSE,
  MDEBUG_FOPEN,
  MDEBUG_FCLOSE,
  MDEBUG_MEM_CURR,
  MDEBUG_MEM_MAX,
  MDEBUG_MAX,
};

enum {
  MAXSUBTAG = 80,
};

typedef struct {
  ssize_t       addr;
  mdebugtype_t  type;
  char          *fn;
  int           lineno;
  char          subtag [MAXSUBTAG];
  long          loc;
  ssize_t       sz;
  char          *bt;
} mdebug_t;

static char     mdebugtag [20] = { "none" };
static char     mdebugsubtag [MAXSUBTAG] = { "" };
static mdebug_t *mdebug = NULL;
static int32_t  mdebugcounts [MDEBUG_MAX];
static bool     initialized = false;
static bool     mdebugverbose = false;
static bool     mdebugnooutput = false;

static void * mdextalloc_a (void *data, const char *fn, int lineno, const char *tag, int type, int ctype);
static void mdfree_a (void *data, const char *fn, int lineno, const char *tag, int ctype);
static void mdebugResize (void);
static void mdebugAdd (void *data, mdebugtype_t type, const char *fn, int lineno, ssize_t sz);
static void mdebugDel (int32_t idx);
static int32_t mdebugFind (void *data);
static int  mdebugComp (const void *a, const void *b);
static void mdebugSort (void);
static void mdebugLog (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

void
mdfree_r (void *data, const char *fn, int lineno)
{
  mdfree_a (data, fn, lineno, "free", MDEBUG_FREE);
  if (data != NULL) {
    free (data);
  }
}

void
mdextfree_r (void *data, const char *fn, int lineno)
{
  if (data == NULL) {
    return;
  }
  mdfree_a (data, fn, lineno, "ext-free", MDEBUG_EXTFREE);
}

void *
mdmalloc_r (size_t sz, const char *fn, int lineno)
{
  void    *data;

  if (initialized) {
    mdebugResize ();
  }
  data = malloc (sz);
  if (initialized) {
    if (mdebugverbose) {
      mdebugLog ("%4s %s %p malloc %s %d\n", mdebugtag, mdebugsubtag, data, fn, lineno);
    }
    mdebugAdd (data, MDEBUG_TYPE_ALLOC, fn, lineno, sz);
    mdebugcounts [MDEBUG_MALLOC] += 1;
  }
  return data;
}

void *
mdrealloc_r (void *data, size_t sz, const char *fn, int lineno)
{
  void  *ndata;

  if (initialized) {
    mdebugResize ();
  }
  if (initialized && data == NULL) {
    mdebugcounts [MDEBUG_MALLOC] += 1;
  }
  if (initialized && data != NULL) {
    int32_t loc;

    loc = mdebugFind (data);
    if (loc >= 0) {
      if (mdebugverbose) {
        mdebugLog ("%4s %s %p realloc %s %d\n", mdebugtag, mdebugsubtag, data, fn, lineno);
      }
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %s %p realloc-bad %s %d\n", mdebugtag, mdebugsubtag, data, fn, lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugcounts [MDEBUG_REALLOC] += 1;
  }
  ndata = realloc (data, sz);
  if (initialized && data == NULL) {
    if (mdebugverbose) {
      mdebugLog ("%4s %s %p realloc-init %s %d\n", mdebugtag, mdebugsubtag, ndata, fn, lineno);
    }
    mdebugAdd (ndata, MDEBUG_TYPE_ALLOC, fn, lineno, sz);
  }
  if (initialized && data != NULL) {
    if (mdebugverbose) {
      mdebugLog ("%4s %s %p realloc %s %d\n", mdebugtag, mdebugsubtag, ndata, fn, lineno);
    }
    mdebugAdd (ndata, MDEBUG_TYPE_REALLOC, fn, lineno, sz);
  }
  return ndata;
}

char *
mdstrdup_r (const char *s, const char *fn, int lineno)
{
  char    *str;

  if (initialized) {
    mdebugResize ();
  }
  str = strdup (s);
  if (initialized) {
    if (mdebugverbose) {
      mdebugLog ("%4s %s %p strdup %s %d\n", mdebugtag, mdebugsubtag, str, fn, lineno);
    }
    mdebugAdd (str, MDEBUG_TYPE_STRDUP, fn, lineno, strlen (s) + 1);
    mdebugcounts [MDEBUG_STRDUP] += 1;
  }
  return str;
}

void *
mdextalloc_r (void *data, const char *fn, int lineno)
{
  return mdextalloc_a (data, fn, lineno, "ext-alloc",
      MDEBUG_TYPE_EXT_ALLOC, MDEBUG_EXTALLOC);
}

void
mdextopen_r (long fd, const char *fn, int lineno)
{
  void  *data = NULL;

  data = (void *) (uintptr_t) fd;
  mdextalloc_a (data, fn, lineno, "open",
      MDEBUG_TYPE_EXT_OPEN, MDEBUG_OPEN);
}

void
mdextsock_r (long fd, const char *fn, int lineno)
{
  void  *data = NULL;

  data = (void *) (uintptr_t) fd;
  mdextalloc_a (data, fn, lineno, "sock",
      MDEBUG_TYPE_EXT_SOCK, MDEBUG_SOCK);
}

void
mdextclose_r (long fd, const char *fn, int lineno)
{
  void  *data = NULL;

  data = (void *) (uintptr_t) fd;
  mdfree_a (data, fn, lineno, "close", MDEBUG_CLOSE);
}

void
mdextfopen_r (void *data, const char *fn, int lineno)
{
  mdextalloc_a (data, fn, lineno, "fopen",
      MDEBUG_TYPE_EXT_FOPEN, MDEBUG_FOPEN);
}

void
mdextfclose_r (void *data, const char *fn, int lineno)
{
  mdfree_a (data, fn, lineno, "fclose", MDEBUG_FCLOSE);
}

void
dataFree_r (void *data)
{
  if (data != NULL) {
    mdfree (data);
  }
}

void
mddataFree_r (void *data, const char *fn, int lineno)
{
  if (data != NULL) {
    mdfree_r (data, fn, lineno);
  }
}

void
mdebugInit (const char *tag)
{
  if (! initialized) {
    mdebugcounts [MDEBUG_INT_ALLOC] += MDEBUG_ALLOC_BUMP;
    mdebug = realloc (mdebug, mdebugcounts [MDEBUG_INT_ALLOC] * sizeof (mdebug_t));
    stpecpy (mdebugtag, mdebugtag + sizeof (mdebugtag), tag);
    *mdebugsubtag = '\0';
    for (int i = 0; i < MDEBUG_MAX; ++i) {
      mdebugcounts [i] = 0;
    }
    for (int i = 0; i < mdebugcounts [MDEBUG_INT_ALLOC]; ++i) {
#if MDEBUG_ENABLE_BACKTRACE
      mdebug [i].bt = NULL;
#endif
      mdebug [i].fn = NULL;
    }
    initialized = true;
  }
}

void
mdebugSubTag (const char *tag)
{
  stpecpy (mdebugsubtag, mdebugsubtag + sizeof (mdebugsubtag), tag);
}

void
mdebugReport (void)
{
  if (initialized) {
    if (! mdebugnooutput) {
      mdebugLog ("== %s ==\n", mdebugtag);
    }

    for (int32_t i = 0; i < mdebugcounts [MDEBUG_COUNT]; ++i) {
      mdebugcounts [MDEBUG_ERRORS] += 1;
      if (mdebugnooutput) {
        continue;
      }
      mdebugLog ("%4s %s 0x%08" PRIx64 " no-free %c %s %d\n", mdebugtag,
          mdebug [i].subtag,
          (int64_t) mdebug [i].addr, mdebug [i].type,
          mdebug [i].fn, mdebug [i].lineno);
#if MDEBUG_ENABLE_BACKTRACE
      if (mdebug [i].bt != NULL) {
        mdebugLog ("%s\n", mdebug [i].bt);
        free (mdebug [i].bt);
      }
      mdebug [i].bt = NULL;
#endif
    }

    if (mdebugnooutput) {
      return;
    }

    mdebugLog ("   count: %" PRId32 "\n", mdebugcounts [MDEBUG_COUNT]);
    mdebugLog ("  ERRORS: %" PRId32 "\n", mdebugcounts [MDEBUG_ERRORS]);
    mdebugLog ("  malloc: %" PRId32 "\n", mdebugcounts [MDEBUG_MALLOC]);
    mdebugLog (" realloc: %" PRId32 "\n", mdebugcounts [MDEBUG_REALLOC]);
    mdebugLog ("  strdup: %" PRId32 "\n", mdebugcounts [MDEBUG_STRDUP]);
    mdebugLog (" Emalloc: %" PRId32 "\n", mdebugcounts [MDEBUG_EXTALLOC]);
    mdebugLog ("   Efree: %" PRId32 "\n", mdebugcounts [MDEBUG_EXTFREE]);
    mdebugLog ("    free: %" PRId32 "\n", mdebugcounts [MDEBUG_FREE]);
    mdebugLog ("     max: %" PRId32 "\n", mdebugcounts [MDEBUG_COUNT_MAX]);
    mdebugLog ("    open: %" PRId32 "\n", mdebugcounts [MDEBUG_OPEN]);
    mdebugLog ("    sock: %" PRId32 "\n", mdebugcounts [MDEBUG_SOCK]);
    mdebugLog ("   close: %" PRId32 "\n", mdebugcounts [MDEBUG_CLOSE]);
    mdebugLog ("   fopen: %" PRId32 "\n", mdebugcounts [MDEBUG_FOPEN]);
    mdebugLog ("  fclose: %" PRId32 "\n", mdebugcounts [MDEBUG_FCLOSE]);
    mdebugLog ("mem-curr: %" PRId32 "\n", mdebugcounts [MDEBUG_MEM_CURR]);
    mdebugLog (" mem-max: %" PRId32 "\n", mdebugcounts [MDEBUG_MEM_MAX]);
  }
}

void
mdebugCleanup (void)
{
  if (initialized) {
    for (int i = 0; i < mdebugcounts [MDEBUG_INT_ALLOC]; ++i) {
#if MDEBUG_ENABLE_BACKTRACE
      if (mdebug [i].bt != NULL) {
        free (mdebug [i].bt);
      }
#endif
      if (mdebug [i].fn != NULL && strcmp (mdebug [i].fn, "null") != 0) {
        free (mdebug [i].fn);
      }
    }

    if (mdebug != NULL) {
      free (mdebug);
      mdebug = NULL;
    }
    for (int i = 0; i < MDEBUG_MAX; ++i) {
      mdebugcounts [i] = 0;
    }
    initialized = false;
  }
}

/* the following routines are for the test suite */

int32_t
mdebugCount (void) /* TESTING */
{
  return mdebugcounts [MDEBUG_COUNT];
}

int32_t
mdebugErrors (void) /* TESTING */
{
  return mdebugcounts [MDEBUG_ERRORS];
}

void
mdebugSetVerbose (void) /* TESTING */
{
  mdebugverbose = true;
}

/* for testing */
void
mdebugSetNoOutput (void) /* TESTING */
{
  mdebugnooutput = true;
}

/* internal routines */

static void
mdfree_a (void *data, const char *fn, int lineno, const char *tag, int ctype)
{
  int32_t     loc;

  if (initialized && data == NULL) {
    mdebugLog ("%4s %s %p %s-null %s %d\n", mdebugtag, mdebugsubtag, data, tag, fn, lineno);
    mdebugcounts [MDEBUG_ERRORS] += 1;
  }
  if (initialized && data != NULL) {
    loc = mdebugFind (data);
    if (loc >= 0) {
      if (mdebugverbose) {
        mdebugLog ("%4s %s %p %s-ok %s %d\n", mdebugtag, mdebugsubtag, data, tag, fn, lineno);
      }
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %s %p %s-bad %s %d\n", mdebugtag, mdebugsubtag, data, tag, fn, lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugcounts [ctype] += 1;
  }
}

static void *
mdextalloc_a (void *data, const char *fn, int lineno,
    const char *tag, int type, int ctype)
{
  if (initialized && data != NULL) {
    mdebugResize ();
    if (mdebugverbose) {
      mdebugLog ("%4s %s %p %s %s %d\n", mdebugtag, mdebugsubtag, data, tag, fn, lineno);
    }
    mdebugAdd (data, type, fn, lineno, 0);
    mdebugcounts [ctype] += 1;
  }
  return data;
}

static void
mdebugResize (void)
{
  int32_t     tval;

  tval = mdebugcounts [MDEBUG_COUNT] + 1;
  if (tval >= mdebugcounts [MDEBUG_INT_ALLOC]) {
    mdebugcounts [MDEBUG_INT_ALLOC] += MDEBUG_ALLOC_BUMP;
    mdebug = realloc (mdebug, mdebugcounts [MDEBUG_INT_ALLOC] *
        sizeof (mdebug_t));
    for (int i = tval; i < mdebugcounts [MDEBUG_INT_ALLOC]; ++i) {
#if MDEBUG_ENABLE_BACKTRACE
      mdebug [i].bt = NULL;
#endif
      mdebug [i].fn = NULL;
    }
  }
}

static void
mdebugAdd (void *data, mdebugtype_t type, const char *fn, int lineno, ssize_t sz)
{
  size_t    tidx;

  tidx = mdebugcounts [MDEBUG_COUNT];
  mdebug [tidx].addr = (ssize_t) data;
  mdebug [tidx].type = type;
  mdebug [tidx].fn = (fn == NULL ? "null" : strdup (fn));
  mdebug [tidx].lineno = lineno;
  mdebug [tidx].loc = mdebugcounts [MDEBUG_COUNT];
  mdebug [tidx].sz = sz;
  stpecpy (mdebug [tidx].subtag, mdebug [tidx].subtag + MAXSUBTAG, mdebugsubtag);
#if MDEBUG_ENABLE_BACKTRACE
  mdebug [tidx].bt = mdebugBacktrace ();
#endif
  mdebugcounts [MDEBUG_COUNT] += 1;
  if (mdebugcounts [MDEBUG_COUNT] > mdebugcounts [MDEBUG_COUNT_MAX]) {
    mdebugcounts [MDEBUG_COUNT_MAX] = mdebugcounts [MDEBUG_COUNT];
  }

  mdebugcounts [MDEBUG_MEM_CURR] += sz;
  if (mdebugcounts [MDEBUG_MEM_CURR] > mdebugcounts [MDEBUG_MEM_MAX]) {
    mdebugcounts [MDEBUG_MEM_MAX] = mdebugcounts [MDEBUG_MEM_CURR];
  }

  mdebugSort ();
}

static void
mdebugDel (int32_t idx)
{
#if MDEBUG_ENABLE_BACKTRACE
  if (mdebug [idx].bt != NULL) {
    free (mdebug [idx].bt);
  }
  mdebug [idx].bt = NULL;
#endif
  if (mdebug [idx].fn != NULL && strcmp (mdebug [idx].fn, "null") != 0) {
    free (mdebug [idx].fn);
  }

  mdebugcounts [MDEBUG_MEM_CURR] -= mdebug [idx].sz;

  for (int32_t i = idx; i + 1 < mdebugcounts [MDEBUG_COUNT]; ++i) {
    memcpy (&mdebug [i], &mdebug [i + 1], sizeof (mdebug_t));
    mdebug [i].loc = i;
  }
  mdebugcounts [MDEBUG_COUNT] -= 1;
  mdebug [mdebugcounts [MDEBUG_COUNT]].fn = NULL;
  mdebug [mdebugcounts [MDEBUG_COUNT]].bt = NULL;
}

static int32_t
mdebugFind (void *data)
{
  mdebug_t    tmpmd;
  mdebug_t    *fmd;
  int32_t     loc = -1;

  tmpmd.addr = (ssize_t) data;
  fmd = bsearch (&tmpmd, mdebug, mdebugcounts [MDEBUG_COUNT],
      sizeof (mdebug_t), mdebugComp);
  if (fmd != NULL) {
    loc = fmd->loc;
  }
  return loc;
}

static int
mdebugComp (const void *a, const void *b)
{
  const mdebug_t  *mda = a;
  const mdebug_t  *mdb = b;

  if (mda->addr < mdb->addr) {
    return -1;
  }
  if (mda->addr > mdb->addr) {
    return 1;
  }
  return 0;
}

static void
mdebugSort (void)
{
  int32_t         last;
  int32_t         prior;
  mdebug_t        tmp;

  last = mdebugcounts [MDEBUG_COUNT] - 1;
  prior = last - 1;
  while (prior >= 0 && last >= 0) {
    if (mdebug [last].addr < mdebug [prior].addr) {
      memcpy (&tmp, &mdebug [last], sizeof (mdebug_t));
      memcpy (&mdebug [last], &mdebug [prior], sizeof (mdebug_t));
      memcpy (&mdebug [prior], &tmp, sizeof (mdebug_t));
      mdebug [last].loc = last;
      mdebug [prior].loc = prior;
    } else {
      break;
    }
    --last;
    --prior;
  }
}

static void
mdebugLog (const char *fmt, ...)
{
  va_list   args;
  FILE      *fh;

  if (mdebugnooutput) {
    return;
  }

#if _WIN32
  fh = fopen ("mdebug.txt", "a");
#else
  fh = stderr;
#endif
  va_start (args, fmt);
  vfprintf (fh, fmt, args);
  va_end (args);
#if _WIN32
  fclose (fh);
#endif
}

#if MDEBUG_ENABLE_BACKTRACE

char *
mdebugBacktrace (void)
{
  char    *disp = NULL;

# if _lib_backtrace
  void    *array [MDEBUG_BACKTRACE_SIZE];
  char    **out;
  size_t  size;
  size_t  dsz = 0;
  char    tmp [2048];

  size = backtrace (array, MDEBUG_BACKTRACE_SIZE);
  out = backtrace_symbols (array, size);
  for (size_t i = 0; i < size; ++i) {
    size_t    odsz;

    odsz = dsz;
    dsz += snprintf (tmp, sizeof (tmp), "  %2ld: %s\n", i, out [i]);
    dsz += 1;
    disp = realloc (disp, dsz);
    disp [odsz] = '\0';
    strcat (disp, tmp);
  }
  free (out);
# endif

  return disp;
}

#endif /* MDEBUG_ENABLE_BACKTRACE */
