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

#include "bdjstring.h"
#include "pathutil.h"
#include "portability.h"

pathinfo_t *
pathInfo (char *path)
{
  pathinfo_t    *pi;
  ssize_t       pos;

  pi = malloc (sizeof (pathinfo_t));
  assert (pi != NULL);

  pi->dirname = NULL;
  pi->filename = NULL;
  pi->basename = NULL;
  pi->extension = NULL;
  pi->dlen = 0;
  pi->flen = 0;
  pi->blen = 0;
  pi->elen = 0;

  ssize_t last = (ssize_t) strlen (path) - 1;
  int chkforext = 1;
  int trailingslash = 0;
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
      pi->extension = &path [i + 1];
      pi->elen = (size_t) (last - i);
      chkforext = 0;
    }
  }
  if (pos > last) {
    --pos;
  }
  pi->basename = &path [pos];
  pi->filename = &path [pos];
  pi->blen = (size_t) (last - pos - (ssize_t) pi->elen + 1);
  if (pi->elen > 0) {
    --pi->blen;
  }
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
// fprintf (stderr, "%s : last:%ld pos:%ld\n", path, last, pos);
// fprintf (stderr, "  flen:%ld blen:%ld elen:%ld ts:%d\n", pi->flen, pi->blen, pi->elen, trailingslash);
// fprintf (stderr, "  dlen:%ld\n", pi->dlen);
// fprintf (stderr, "  dir:%s\n", pi->dirname);
// fprintf (stderr, "  file:%s\n", pi->filename);
// fprintf (stderr, "  base:%s\n", pi->basename);
// fprintf (stderr, "  ext:%s\n", pi->extension);
// fflush (stderr);

  return pi;
}

void
pathInfoFree (pathinfo_t *pi)
{
  if (pi != NULL) {
    free (pi);
  }
}

void
pathToWinPath (char *from, char *to, size_t maxlen)
{
  strlcpy (to, from, maxlen);
  for (size_t i = 0; i < maxlen; ++i) {
    if (to [i] == '\0') {
      break;
    }
    if (to [i] == '/') {
      to [i] = '\\';
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
pathRealPath (char *from, char *to)
{
#if _lib_realpath
  (void) ! realpath (from, to);
#endif
#if _lib_GetFullPathName
  (void) ! GetFullPathName (from, MAXPATHLEN, to, NULL);
#endif
}

