/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>

#include <glib.h>

#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "dirop.h"
#include "mdebug.h"
#include "osdir.h"
#include "osutils.h"
#include "sysvars.h"

static int diropMakeRecursiveDir (const char *dirname);
static int diropMkdir (const char *dirname);

int
diropMakeDir (const char *dirname)
{
  int rc;
  rc = diropMakeRecursiveDir (dirname);
  return rc;
}

/* deletes the directory */
/* if the directory only contains other dirs, the empty dirs will */
/* be recursively deleted */
bool
diropDeleteDir (const char *dirname, int flags)
{
  dirhandle_t   *dh;
  char          *fname;
  char          temp [MAXPATHLEN];

  if (! fileopIsDirectory (dirname)) {
    return false;
  }

  dh = osDirOpen (dirname);
  while ((fname = osDirIterate (dh)) != NULL) {
    if (strcmp (fname, ".") == 0 ||
        strcmp (fname, "..") == 0) {
      mdfree (fname);
      continue;
    }

    snprintf (temp, sizeof (temp), "%s/%s", dirname, fname);
    if (fname != NULL) {
      if (fileopIsDirectory (temp)) {
        if (! diropDeleteDir (temp, flags)) {
          mdfree (fname);
          osDirClose (dh);
          return false;
        }
      } else {
        if ((flags & DIROP_ONLY_IF_EMPTY) == DIROP_ONLY_IF_EMPTY) {
          mdfree (fname);
          osDirClose (dh);
          return false;
        }
        fileopDelete (temp);
      }
    }
    mdfree (fname);
  }

  osDirClose (dh);

  if (! isWindows ()) {
    /* in case the dir is actually a link... */
    fileopDelete (dirname);
  }

#if _lib_RemoveDirectoryW
  {
    wchar_t       * tdirname;

    tdirname = osToWideChar (dirname);
    RemoveDirectoryW (tdirname);
    mdfree (tdirname);
  }
#else
  rmdir (dirname);
#endif

  return true;
}

/* internal routines */

static int
diropMakeRecursiveDir (const char *dirname)
{
  char    tbuff [MAXPATHLEN];
  char    *p = NULL;

  stpecpy (tbuff, tbuff + MAXPATHLEN, dirname);
  stringTrimChar (tbuff, '/');

  for (p = tbuff + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      diropMkdir (tbuff);
      *p = '/';
    }
  }
  diropMkdir (tbuff);
  return 0;
}

static int
diropMkdir (const char *dirname)
{
  int   rc;

#if _args_mkdir == 1      // windows
  wchar_t   *tdirname = NULL;

  tdirname = osToWideChar (dirname);
  rc = _wmkdir (tdirname);
  mdfree (tdirname);
#endif
#if _args_mkdir == 2 && _define_S_IRWXU
  rc = mkdir (dirname, S_IRWXU);
#endif
  return rc;
}

