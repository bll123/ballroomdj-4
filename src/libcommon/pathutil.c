/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "pathutil.h"

pathinfo_t *
pathInfo (const char *path)
{
  pathinfo_t    *pi = NULL;
  ssize_t       pos;
  ssize_t       last;
  bool          chkforext = true;
  bool          trailingslash = false;


  pi = mdmalloc (sizeof (pathinfo_t));

  pi->dirname = path;
  pi->filename = NULL;
  pi->basename = NULL;
  pi->extension = NULL;
  pi->dlen = 0;
  pi->flen = 0;
  pi->blen = 0;
  pi->elen = 0;

  last = strlen (path) - 1;
  chkforext = true;
  trailingslash = false;
  pos = 0;

  for (ssize_t i = last; i >= 0; --i) {
    if (path [i] == '/' || path [i] == '\\') {
      pos = i + 1;
      if (pos >= last) {
        /* no extension, continue back to find the basename */
        chkforext = false;
        trailingslash = true;
        continue;
      }
      break;
    }
    if (chkforext && path [i] == '.') {
      pi->extension = &path [i];
      pi->elen = (size_t) (last - i + 1);
      chkforext = false;
    }
  }
  if (pos > last) {
    --pos;
  }
  pi->basename = &path [pos];
  pi->filename = &path [pos];
  pi->blen = last - pos - pi->elen + 1;
  pi->flen = last - pos + 1;
  if (trailingslash && pos > 1) {
    --pi->blen;
    --pi->flen;
  }
  pi->dlen = last - pi->flen;
  if (trailingslash && last > 0) {
    --pi->dlen;
  }
  if (last == 0) {
    ++pi->dlen;
  }
  if (pos <= 1) {
    ++pi->dlen;
  }
#if 0  // debugging
 fprintf (stderr, "%s : last:%" PRId64 " pos:%" PRId64 "\n", path, (int64_t) last, (int64_t) pos);
 fprintf (stderr, "  dlen:%" PRId64 "\n", (int64_t) pi->dlen);
 fprintf (stderr, "  dir:%.*s\n", (int) pi->dlen, pi->dirname);
 fprintf (stderr, "  flen:%" PRId64 " blen:%" PRId64 " elen:%" PRId64 " ts:%d\n", (int64_t) pi->flen, (int64_t) pi->blen, (int64_t) pi->elen, trailingslash);
 fprintf (stderr, "  file:%.*s\n", (int) pi->flen, pi->filename);
 fprintf (stderr, "  base:%.*s\n", (int) pi->blen, pi->basename);
 fprintf (stderr, "  ext:%.*s\n", (int) pi->elen, pi->extension);
 fflush (stderr);
#endif

  return pi;
}

void
pathInfoFree (pathinfo_t *pi)
{
  if (pi != NULL) {
    mdfree (pi);
  }
}

bool
pathInfoExtCheck (pathinfo_t *pi, const char *extension)
{
  if (pi == NULL) {
    return false;
  }

  if (pi->elen == 0 && extension == NULL) {
    return true;
  }

  if (pi->elen != strlen (extension)) {
    return false;
  }

  if (pi->elen > 0 &&
      extension != NULL &&
      strncmp (pi->extension, extension, pi->elen) == 0) {
    return true;
  }

  return false;
}


void
pathWinPath (char *buff, size_t len)
{
  for (size_t i = 0; i < len; ++i) {
    if (buff [i] == '\0') {
      break;
    }
    if (buff [i] == '/') {
      buff [i] = '\\';
    }
  }
}

void
pathNormPath (char *buff, size_t len)
{
  for (size_t i = 0; i < len; ++i) {
    if (buff [i] == '\0') {
      break;
    }
    if (buff [i] == '\\') {
      buff [i] = '/';
    }
  }
}

/* remove all /./ ../dir components */
void
pathStripPath (char *buff, size_t len)
{
  size_t  j = 0;
  size_t  priorslash = 0;
  size_t  lastslash = 0;
  char    *p;

  pathNormPath (buff, len);
  j = 0;
  p = buff;
  for (size_t i = 0; i < len; ++i) {
    if (*p == '\0') {
      buff [j] = '\0';
      break;
    }
    if (*p == '/') {
      priorslash = lastslash;
      lastslash = j;
    }
    if ((j == 0 || j - 1 == lastslash) &&
        (len - i) > 1 && strncmp (p, "./", 2) == 0) {
      p += 2;
    }
    if ((j == 0 || j - 1 == lastslash) &&
        (len - i) > 2 && strncmp (p, "../", 3) == 0) {
      p += 3;
      j = priorslash + 1;
    }

    buff [j] = *p;
    ++j;
    ++p;
  }
}

void
pathRealPath (char *to, const char *from, size_t sz)
{
#if _lib_realpath
  (void) ! realpath (from, to);
#endif
#if _lib_GetFullPathName
  (void) ! GetFullPathName (from, sz, to, NULL);
#endif
}

