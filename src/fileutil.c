#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "fileutil.h"

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
  fi->dlen = (size_t) (last - fi->flen);
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
