#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "pathutil.h"

pathinfo_t *
pathInfo (const char *path)
{
  pathinfo_t    *pi = NULL;
  ssize_t       pos;
  ssize_t       last;
  int           chkforext;
  int           trailingslash;


  pi = malloc (sizeof (pathinfo_t));
  assert (pi != NULL);

  pi->dirname = path;
  pi->filename = NULL;
  pi->basename = NULL;
  pi->extension = NULL;
  pi->dlen = 0;
  pi->flen = 0;
  pi->blen = 0;
  pi->elen = 0;

  last = (ssize_t) strlen (path) - 1;
  chkforext = 1;
  trailingslash = 0;
  pos = 0;

  for (ssize_t i = last; i >= 0; --i) {
    if (path [i] == '/' || path [i] == '\\') {
      pos = i + 1;
      if (pos >= last) {
        /* no extension, continue back to find the basename */
        chkforext = 0;
        trailingslash = 1;
        continue;
      }
      break;
    }
    if (chkforext && path [i] == '.') {
      pi->extension = &path [i];
      pi->elen = (size_t) (last - i + 1);
      chkforext = 0;
    }
  }
  if (pos > last) {
    --pos;
  }
  pi->basename = &path [pos];
  pi->filename = &path [pos];
  pi->blen = (size_t) (last - pos - (ssize_t) pi->elen + 1);
  pi->flen = (size_t) (last - pos + 1);
  if (trailingslash && pos > 1) {
    --pi->blen;
    --pi->flen;
  }
  pi->dlen = (size_t) (last - (ssize_t) pi->flen);
  if (trailingslash && last > 0) {
    --pi->dlen;
  }
  if (last == 0) {
    ++pi->dlen;
  }
  if (pos <= 1) {
    ++pi->dlen;
  }
#if 0
 fprintf (stderr, "%s : last:%ld pos:%ld\n", path, last, pos);
 fprintf (stderr, "  dlen:%ld\n", pi->dlen);
 fprintf (stderr, "  flen:%ld blen:%ld elen:%ld ts:%d\n", pi->flen, pi->blen, pi->elen, trailingslash);
 fprintf (stderr, "  dir:%.*s\n", (int) pi->dlen, pi->dirname);
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
    free (pi);
  }
}

bool
pathInfoExtCheck (pathinfo_t *pi, const char *extension)
{
  if (pi->elen != strlen (extension)) {
    return false;
  }

  if (strncmp (pi->extension, extension, pi->elen) == 0) {
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

