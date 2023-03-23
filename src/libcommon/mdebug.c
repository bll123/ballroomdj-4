/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */

/* this code is not thread safe */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#include "mdebug.h"

typedef enum {
  MDEBUG_TYPE_ALLOC = 'a',
  MDEBUG_TYPE_REALLOC = 'r',
  MDEBUG_TYPE_STRDUP = 's',
  MDEBUG_TYPE_FREE = 'f',
  MDEBUG_TYPE_EXT_ALLOC = 'E',
  MDEBUG_TYPE_EXT_FREE = 'F',
} mdebugtype_t;

enum {
  MDEBUG_ALLOC_BUMP = 1000,
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
  MDEBUG_MAX,
};

typedef struct {
  ssize_t       addr;
  mdebugtype_t  type;
  const char    *fn;
  int           lineno;
  long          loc;
} mdebug_t;

static char     mdebugtag [20] = { "none" };
static mdebug_t *mdebug = NULL;
static long     mdebugcounts [MDEBUG_MAX];
static bool     initialized = false;
static bool     mdebugverbose = false;

static void mdebugResize (void);
static void mdebugAdd (void *data, mdebugtype_t type, const char *fn, int lineno);
static void mdebugDel (long idx);
static long mdebugFind (void *data);
static int mdebugComp (const void *a, const void *b);
static void mdebugSort (void);
static void mdebugLog (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

void
mdfree_r (void *data, const char *fn, int lineno)
{
  long  loc;

  if (initialized && data == NULL) {
    mdebugLog ("%4s %p free-null %s %d\n", mdebugtag, data, fn, lineno);
  }
  if (initialized && data != NULL) {
    loc = mdebugFind (data);
    if (loc >= 0) {
      if (mdebugverbose) {
        mdebugLog ("%4s %p free-ok %s %d\n", mdebugtag, data, fn, lineno);
      }
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p free-bad %s %d\n", mdebugtag, data, fn, lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugcounts [MDEBUG_FREE] += 1;
  }
  if (data != NULL) {
    free (data);
  }
}

void
mdextfree_r (void *data, const char *fn, int lineno)
{
  long  loc;

  if (initialized && data != NULL) {
    loc = mdebugFind (data);
    if (loc >= 0) {
      if (mdebugverbose) {
        mdebugLog ("%4s %p ext-free-ok %s %d\n", mdebugtag, data, fn, lineno);
      }
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p ext-free-dup %s %d\n", mdebugtag, data, fn, lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugcounts [MDEBUG_EXTFREE] += 1;
  }
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
      mdebugLog ("%4s %p malloc %s %d\n", mdebugtag, data, fn, lineno);
    }
    mdebugAdd (data, MDEBUG_TYPE_ALLOC, fn, lineno);
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
  if (initialized && data != NULL) {
    long  loc;

    loc = mdebugFind (data);
    if (loc >= 0) {
      if (mdebugverbose) {
        mdebugLog ("%4s %p realloc %s %d\n", mdebugtag, data, fn, lineno);
      }
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p realloc-bad %s %d\n", mdebugtag, data, fn, lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugcounts [MDEBUG_REALLOC] += 1;
  } else {
    mdebugcounts [MDEBUG_MALLOC] += 1;
  }
  ndata = realloc (data, sz);
  if (initialized) {
    if (mdebugverbose) {
      mdebugLog ("%4s %p realloc-init %s %d\n", mdebugtag, data, fn, lineno);
    }
    mdebugAdd (ndata, MDEBUG_TYPE_REALLOC, fn, lineno);
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
      mdebugLog ("%4s %p strdup %s %d\n", mdebugtag, str, fn, lineno);
    }
    mdebugAdd (str, MDEBUG_TYPE_STRDUP, fn, lineno);
    mdebugcounts [MDEBUG_STRDUP] += 1;
  }
  return str;
}

void *
mdextalloc_r (void *data, const char *fn, int lineno)
{
  if (initialized && data != NULL) {
    mdebugResize ();
    if (mdebugverbose) {
      mdebugLog ("%4s %p ext-alloc %s %d\n", mdebugtag, data, fn, lineno);
    }
    mdebugAdd (data, MDEBUG_TYPE_ALLOC, fn, lineno);
    mdebugcounts [MDEBUG_EXTALLOC] += 1;
  }
  return data;
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
    strcpy (mdebugtag, tag);
    for (int i = 0; i < MDEBUG_MAX; ++i) {
      mdebugcounts [i] = 0;
    }
    initialized = true;
  }
}

void
mdebugReport (void)
{
  if (initialized) {
    mdebugLog ("== %s ==\n", mdebugtag);
    for (long i = 0; i < mdebugcounts [MDEBUG_COUNT]; ++i) {
      mdebugLog ("%4s 0x%08" PRIx64 " no-free %c %s %d\n", mdebugtag,
          (int64_t) mdebug [i].addr, mdebug [i].type, mdebug [i].fn, mdebug [i].lineno);
      mdebugcounts [MDEBUG_ERRORS] += 1;
    }
    mdebugLog ("  count: %ld\n", mdebugcounts [MDEBUG_COUNT]);
    mdebugLog (" ERRORS: %ld\n", mdebugcounts [MDEBUG_ERRORS]);
    mdebugLog (" malloc: %ld\n", mdebugcounts [MDEBUG_MALLOC]);
    mdebugLog ("realloc: %ld\n", mdebugcounts [MDEBUG_REALLOC]);
    mdebugLog (" strdup: %ld\n", mdebugcounts [MDEBUG_STRDUP]);
    mdebugLog ("Emalloc: %ld\n", mdebugcounts [MDEBUG_EXTALLOC]);
    mdebugLog ("  Efree: %ld\n", mdebugcounts [MDEBUG_EXTFREE]);
    mdebugLog ("   free: %ld\n", mdebugcounts [MDEBUG_FREE]);
    mdebugLog ("    max: %ld\n", mdebugcounts [MDEBUG_COUNT_MAX]);
  }
}

void
mdebugCleanup (void)
{
  if (initialized) {
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

long
mdebugCount (void)
{
  return mdebugcounts [MDEBUG_COUNT];
}

long
mdebugErrors (void)
{
  return mdebugcounts [MDEBUG_ERRORS];
}

/* internal routines */

static void
mdebugResize (void)
{
  long    tval;

  tval = mdebugcounts [MDEBUG_COUNT] + 1;
  if (tval >= mdebugcounts [MDEBUG_INT_ALLOC]) {
    mdebugcounts [MDEBUG_INT_ALLOC] += MDEBUG_ALLOC_BUMP;
    mdebug = realloc (mdebug, mdebugcounts [MDEBUG_INT_ALLOC] *
        sizeof (mdebug_t));
  }
}

static void
mdebugAdd (void *data, mdebugtype_t type, const char *fn, int lineno)
{
  mdebug [mdebugcounts [MDEBUG_COUNT]].addr = (ssize_t) data;
  mdebug [mdebugcounts [MDEBUG_COUNT]].type = type;
  mdebug [mdebugcounts [MDEBUG_COUNT]].fn = fn;
  mdebug [mdebugcounts [MDEBUG_COUNT]].lineno = lineno;
  mdebug [mdebugcounts [MDEBUG_COUNT]].loc = mdebugcounts [MDEBUG_COUNT];
  mdebugcounts [MDEBUG_COUNT] += 1;
  if (mdebugcounts [MDEBUG_COUNT] > mdebugcounts [MDEBUG_COUNT_MAX]) {
    mdebugcounts [MDEBUG_COUNT_MAX] = mdebugcounts [MDEBUG_COUNT];
  }
  mdebugSort ();
}

static void
mdebugDel (long idx)
{
  for (long i = idx; i + 1 < mdebugcounts [MDEBUG_COUNT]; ++i) {
    memcpy (&mdebug [i], &mdebug [i + 1], sizeof (mdebug_t));
    mdebug [i].loc = i;
  }
  mdebugcounts [MDEBUG_COUNT] -= 1;
}

static long
mdebugFind (void *data)
{
  mdebug_t    tmpmd;
  mdebug_t    *fmd;
  long        loc = -1;

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
  long            last;
  long            prior;
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
#if __WINNT__
  FILE      *fh;

  fh = fopen ("mdebug.txt", "a");
  va_start (args, fmt);
  vfprintf (fh, fmt, args);
  va_end (args);
  fclose (fh);
#else
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
#endif
}

