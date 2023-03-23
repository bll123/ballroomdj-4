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

typedef struct {
  ssize_t       addr;
  mdebugtype_t  type;
  const char    *fn;
  int           lineno;
  long          loc;
} mdebug_t;

static char     mdebugtag [20] = { "none" };
static mdebug_t *mdebug = NULL;
static long     mdebugcount = 0;
static long     mdebugmax = 0;
static long     mdebugextalloccount = 0;
static long     mdebugmalloccount = 0;
static long     mdebugrealloccount = 0;
static long     mdebugstrdupcount = 0;
static long     mdebugfreecount = 0;
static long     mdebugalloc = 0;
static long     mdebugerrors = 0;
static bool     initialized = false;

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
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p free-bad %s %d\n", mdebugtag, data, fn, lineno);
      ++mdebugerrors;
    }
    ++mdebugfreecount;
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
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p ext-free-dup %s %d\n", mdebugtag, data, fn, lineno);
      ++mdebugerrors;
    }
    ++mdebugfreecount;
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
    mdebugAdd (data, MDEBUG_TYPE_ALLOC, fn, lineno);
    ++mdebugmalloccount;
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
      mdebugDel (loc);
    } else {
      mdebugLog ("%4s %p realloc-bad %s %d\n", mdebugtag, data, fn, lineno);
      ++mdebugerrors;
    }
  }
  ndata = realloc (data, sz);
  if (initialized) {
    mdebugAdd (ndata, MDEBUG_TYPE_REALLOC, fn, lineno);
    ++mdebugrealloccount;
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
    mdebugAdd (str, MDEBUG_TYPE_STRDUP, fn, lineno);
    ++mdebugstrdupcount;
  }
  return str;
}

void *
mdextalloc_r (void *data, const char *fn, int lineno)
{
  if (initialized && data != NULL) {
    mdebugResize ();
    mdebugAdd (data, MDEBUG_TYPE_ALLOC, fn, lineno);
    ++mdebugextalloccount;
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
    mdebugalloc += MDEBUG_ALLOC_BUMP;
    mdebug = realloc (mdebug, mdebugalloc * sizeof (mdebug_t));
    strcpy (mdebugtag, tag);
    initialized = true;
  }
}

void
mdebugReport (void)
{
  if (initialized) {
    mdebugLog ("== %s ==\n", mdebugtag);
    for (long i = 0; i < mdebugcount; ++i) {
      mdebugLog ("%4s 0x%08" PRIx64 " no-free %c %s %d\n", mdebugtag,
          (int64_t) mdebug [i].addr, mdebug [i].type, mdebug [i].fn, mdebug [i].lineno);
      ++mdebugerrors;
    }
    mdebugLog ("  count: %ld\n", mdebugcount);
    mdebugLog (" ERRORS: %ld\n", mdebugerrors);
    mdebugLog (" malloc: %ld\n", mdebugmalloccount);
    mdebugLog ("realloc: %ld\n", mdebugrealloccount);
    mdebugLog (" strdup: %ld\n", mdebugstrdupcount);
    mdebugLog ("Emalloc: %ld\n", mdebugextalloccount);
    mdebugLog ("   free: %ld\n", mdebugfreecount);
    mdebugLog ("    max: %ld\n", mdebugmax);
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
    mdebugcount = 0;
    mdebugalloc = 0;
    mdebugerrors = 0;
    initialized = false;
  }
}

long
mdebugCount (void)
{
  return mdebugcount;
}

long
mdebugErrors (void)
{
  return mdebugerrors;
}

/* internal routines */

static void
mdebugResize (void)
{
  long    tval;

  tval = mdebugcount + 1;
  if (tval >= mdebugalloc) {
    mdebugalloc += MDEBUG_ALLOC_BUMP;
    mdebug = realloc (mdebug, mdebugalloc * sizeof (mdebug_t));
  }
}

static void
mdebugAdd (void *data, mdebugtype_t type, const char *fn, int lineno)
{
  mdebug [mdebugcount].addr = (ssize_t) data;
  mdebug [mdebugcount].type = type;
  mdebug [mdebugcount].fn = fn;
  mdebug [mdebugcount].lineno = lineno;
  mdebug [mdebugcount].loc = mdebugcount;
  ++mdebugcount;
  if (mdebugcount > mdebugmax) {
    mdebugmax = mdebugcount;
  }
  mdebugSort ();
}

static void
mdebugDel (long idx)
{
  for (long i = idx; i + 1 < mdebugcount; ++i) {
    memcpy (&mdebug [i], &mdebug [i + 1], sizeof (mdebug_t));
    mdebug [i].loc = i;
  }
  --mdebugcount;
}

static long
mdebugFind (void *data)
{
  mdebug_t    tmpmd;
  mdebug_t    *fmd;
  long        loc = -1;

  tmpmd.addr = (ssize_t) data;
  fmd = bsearch (&tmpmd, mdebug, mdebugcount, sizeof (mdebug_t), mdebugComp);
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

  last = mdebugcount - 1;
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

