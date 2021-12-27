#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "fileutil.h"
#include "sysvars.h"
#include "bdjstring.h"

fileinfo_t *
fileInfo (char *path)
{
  fileinfo_t    *fi;
  ssize_t       pos;

  fi = malloc (sizeof (fileinfo_t));
  assert (fi != NULL);

  fi->dirname = NULL;
  fi->filename = NULL;
  fi->basename = NULL;
  fi->extension = NULL;
  fi->dlen = 0;
  fi->flen = 0;
  fi->blen = 0;
  fi->elen = 0;

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
      pos = i + 1;
      fi->extension = &path [pos];
      fi->elen = (size_t) (last - pos + 1);
      chkforext = 0;
    }
  }
  if (pos > last) {
    --pos;
  }
  fi->basename = &path [pos];
  fi->filename = &path [pos];
  fi->blen = (size_t) (last - pos - (ssize_t) fi->elen + 1);
  if (fi->elen > 0) {
    --fi->blen;
  }
  fi->flen = (size_t) (last - pos + 1);
  if (trailingslash && pos > 1) {
    --fi->blen;
    --fi->flen;
  }
  fi->dlen = (size_t) (last - (ssize_t) fi->flen);
  if (trailingslash && last > 0) {
    --fi->dlen;
  }
  if (last == 0) {
    ++fi->dlen;
  }
  if (pos <= 1) {
    ++fi->dlen;
  }
  // printf ("%s : last:%ld pos:%ld\n", path, last, pos);
  // printf ("  flen:%ld blen:%ld elen:%ld ts:%d\n", fi->flen, fi->blen, fi->elen, trailingslash);
  // printf ("  dlen:%ld\n", fi->dlen);
  // fflush (stdout);

  return fi;
}

void
fileInfoFree (fileinfo_t *fi)
{
  if (fi != NULL) {
    free (fi);
  }
}


void
makeBackups (char *fname, int count)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];

  for (int i = count; i >= 1; --i) {
    snprintf (nfn, MAXPATHLEN, "%s.bak.%d", fname, i);
    snprintf (ofn, MAXPATHLEN, "%s.bak.%d", fname, i - 1);
    if (i - 1 == 0) {
      strlcpy (ofn, fname, MAXPATHLEN);
    }
    if (fileExists (ofn)) {
      if ((i - 1) != 0) {
        fileMove (ofn, nfn);
      } else {
        fileCopy (ofn, nfn);
      }
    }
  }
  return;
}

/* boolean */
inline int
fileExists (char *fname)
{
  struct stat   statbuf;

  int rc = stat (fname, &statbuf);
  return (rc == 0);
}

int
fileCopy (char *fname, char *nfn)
{
  char      cmd [MAXPATHLEN];
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];

  if (isWindows ()) {
    toWinPath (fname, tfname, MAXPATHLEN);
    toWinPath (nfn, tnfn, MAXPATHLEN);
    snprintf (cmd, MAXPATHLEN, "copy /y/b \"%s\" \"%s\" >NUL",
        tfname, tnfn);
  } else {
    snprintf (cmd, MAXPATHLEN, "cp -f '%s' '%s' >/dev/null 2>&1", fname, nfn);
  }
  int rc = system (cmd);
  return rc;
}

int
fileMove (char *fname, char *nfn)
{
  int       rc = -1;

  /*
   * Windows won't rename to an existing file, but does
   * not return an error.
   */
  if (isWindows()) {
    fileDelete (nfn);
  }
  rc = rename (fname, nfn);
  return rc;
}

inline int
fileDelete (char *fname)
{
  int rc = unlink (fname);
  return rc;
}

char *
fileReadAll (char *fname)
{
  FILE        *fh;
  struct stat statbuf;
  int         rc;
  size_t      len;
  char        *data;

  rc = stat (fname, &statbuf);
  if (rc != 0) {
    return NULL;
  }
  fh = fopen (fname, "r");
  data = malloc ((size_t) statbuf.st_size);
  assert (data != NULL);
  len = fread (data, (size_t) statbuf.st_size, 1, fh);
  assert (len == (size_t) statbuf.st_size);
  fclose (fh);

  return data;
}

void
toWinPath (char *from, char *to, size_t maxlen)
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
