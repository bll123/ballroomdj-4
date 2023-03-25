/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "dirop.h"
#include "mdebug.h"
#include "osdir.h"
#include "osutils.h"

#if _hdr_windows
# include <windows.h>
#endif

static int diropMakeRecursiveDir (const char *dirname);
static int diropMkdir (const char *dirname);

int
diropMakeDir (const char *dirname)
{
  int rc;
  rc = diropMakeRecursiveDir (dirname);
  return rc;
}

void
diropDeleteDir (const char *dirname)
{
  dirhandle_t   *dh;
  char          *fname;
  char          temp [MAXPATHLEN];


  if (! fileopIsDirectory (dirname)) {
    return;
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
        diropDeleteDir (temp);
      } else {
        fileopDelete (temp);
      }
    }
    mdfree (fname);
  }

  osDirClose (dh);

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
}

/* internal routines */

static int
diropMakeRecursiveDir (const char *dirname)
{
  char    tbuff [MAXPATHLEN];
  char    *p = NULL;

  strlcpy (tbuff, dirname, MAXPATHLEN);
  stringTrimChar (tbuff, '/');

  for (p = tbuff + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
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

