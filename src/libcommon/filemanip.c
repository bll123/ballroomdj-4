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
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "sysvars.h"

#if _hdr_windows
# include <windows.h>
#endif

int
filemanipMove (const char *fname, const char *nfn)
{
  int       rc = -1;

  /*
   * Windows won't rename to an existing file, but does
   * not return an error.
   */
  if (isWindows()) {
    fileopDelete (nfn);
  }
#if _lib__wrename
  {
    wchar_t   *wfname;
    wchar_t   *wnfn;

    wfname = osToWideChar (fname);
    wnfn = osToWideChar (nfn);
    rc = _wrename (wfname, wnfn);
    mdfree (wfname);
    mdfree (wnfn);
  }
#else
  rc = rename (fname, nfn);
#endif
  return rc;
}

int
filemanipCopy (const char *fname, const char *nfn)
{
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];
  int       rc = -1;
  time_t    origtm;

  origtm = fileopModTime (fname);

  if (isWindows ()) {
    strlcpy (tfname, fname, sizeof (tfname));
    pathDisplayPath (tfname, sizeof (tfname));
    strlcpy (tnfn, nfn, sizeof (tnfn));
    pathDisplayPath (tnfn, sizeof (tnfn));
    origtm = fileopModTime (tfname);
#if _lib_CopyFileW
    {
      wchar_t   *wtfname;
      wchar_t   *wtnfn;
      wtfname = osToWideChar (tfname);
      wtnfn = osToWideChar (tnfn);

      rc = CopyFileW (wtfname, wtnfn, 0);
      rc = rc != 0 ? 0 : -1;
      mdfree (wtfname);
      mdfree (wtnfn);
    }
    fileopSetModTime (tnfn, origtm);
#endif
  } else {
    char    *data;
    FILE    *fh;
    size_t  len;
    size_t  trc = 0;

    data = filedataReadAll (fname, &len);
    if (data != NULL) {
      fh = fileopOpen (nfn, "w");
      if (fh != NULL) {
        trc = fwrite (data, len, 1, fh);
        mdextfclose (fh);
        fclose (fh);
      }
      mdfree (data);
    }
    rc = trc == 1 ? 0 : -1;
  }

  fileopSetModTime (nfn, origtm);

  return rc;
}

/* link if possible, otherwise copy */
/* windows has had symlinks for ages, but they require either */
/* admin permission, or have the machine in developer mode */
/* shell links would be fine probably, but creating them is a hassle */
int
filemanipLinkCopy (const char *fname, const char *nfn)
{
  int       rc = -1;

  if (isWindows ()) {
    rc = filemanipCopy (fname, nfn);
  } else {
    osCreateLink (fname, nfn);
  }
  return rc;
}

void
filemanipBackup (const char *fname, int count)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];

  for (int i = count; i >= 1; --i) {
    snprintf (nfn, sizeof (nfn), "%s.bak.%d", fname, i);
    snprintf (ofn, sizeof (ofn), "%s.bak.%d", fname, i - 1);
    if (i - 1 == 0) {
      strlcpy (ofn, fname, sizeof (ofn));
    }
    if (fileopFileExists (ofn)) {
      if ((i - 1) != 0) {
        filemanipMove (ofn, nfn);
      } else {
        filemanipCopy (ofn, nfn);
      }
    }
  }
  return;
}

void
filemanipRenameAll (const char *ofname, const char *nfname)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];
  int       count = 10;

  for (int i = count; i >= 1; --i) {
    snprintf (ofn, sizeof (ofn), "%s.bak.%d", ofname, i);
    if (fileopFileExists (ofn)) {
      snprintf (nfn, sizeof (nfn), "%s.bak.%d", nfname, i);
      filemanipMove (ofn, nfn);
    }
  }
  if (fileopFileExists (ofname)) {
    filemanipMove (ofname, nfname);
  }
  return;
}

void
filemanipDeleteAll (const char *name)
{
  char      ofn [MAXPATHLEN];
  int       count = 10;

  for (int i = count; i >= 1; --i) {
    snprintf (ofn, sizeof (ofn), "%s.bak.%d", name, i);
    if (fileopFileExists (ofn)) {
      fileopDelete (ofn);
    }
  }
  if (fileopFileExists (name)) {
    fileopDelete (name);
  }
  return;
}



