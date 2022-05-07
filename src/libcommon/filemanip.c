#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
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
#include "slist.h"
#include "osutils.h"
#include "pathutil.h"
#include "queue.h"
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
  rc = rename (fname, nfn);
  return rc;
}

int
filemanipCopy (const char *fname, const char *nfn)
{
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];
  int       rc = -1;


  if (isWindows ()) {
    strlcpy (tfname, fname, sizeof (tfname));
    pathWinPath (tfname, sizeof (tfname));
    strlcpy (tnfn, nfn, sizeof (tnfn));
    pathWinPath (tnfn, sizeof (tnfn));
#if _lib_CopyFileW
    {
      wchar_t   *wtfname;
      wchar_t   *wtnfn;
      wtfname = osToFSFilename (tfname);
      wtnfn = osToFSFilename (tnfn);

      rc = CopyFileW (wtfname, wtnfn, 0);
      rc = (rc == 0);
      free (wtfname);
      free (wtnfn);
    }
#endif
  } else {
    char    *data;
    FILE    *fh;
    size_t  len;
    size_t  trc = 0;

    data = filedataReadAll (fname, &len);
    fh = fopen (nfn, "w");
    if (fh != NULL) {
      trc = fwrite (data, len, 1, fh);
      fclose (fh);
    }
    free (data);
    rc = (trc == 1);
  }
  return rc;
}

/* link if possible, otherwise copy */
/* windows has had symlinks for ages, but they require either */
/* admin permission, or have the machine in developer mode */
int
filemanipLinkCopy (const char *fname, const char *nfn)
{
  int       rc = -1;

  if (isWindows ()) {
    rc = filemanipCopy (fname, nfn);
  } else {
#if _lib_symlink
    rc = symlink (fname, nfn);
#endif
  }
  return rc;
}

slist_t *
filemanipBasicDirList (const char *dirname, const char *extension)
{
  dirhandle_t   *dh;
  char          *fname;
  slist_t       *fileList;
  pathinfo_t    *pi;
  char          temp [MAXPATHLEN];
  char          *cvtname;
  gsize         bread, bwrite;
  GError        *gerr = NULL;


  if (! fileopIsDirectory (dirname)) {
    return NULL;
  }

  snprintf (temp, sizeof (temp), "basic-dir-%s", dirname);
  if (extension != NULL) {
    strlcat (temp, "-", sizeof (temp));
    strlcat (temp, extension, sizeof (temp));
  }
  fileList = slistAlloc (temp, LIST_UNORDERED, NULL);
  dh = osDirOpen (dirname);
  while ((fname = osDirIterate (dh)) != NULL) {
    snprintf (temp, sizeof (temp), "%s/%s", dirname, fname);
    if (fileopIsDirectory (temp)) {
      free (fname);
      continue;
    }

    if (extension != NULL) {
      pi = pathInfo (fname);
      if (pathInfoExtCheck (pi, extension) == false) {
        pathInfoFree (pi);
        free (fname);
        continue;
      }
      pathInfoFree (pi);
    }

    gerr = NULL;
    cvtname = g_filename_to_utf8 (fname, strlen (fname),
        &bread, &bwrite, &gerr);
    if (cvtname != NULL) {
      slistSetStr (fileList, cvtname, NULL);
    }
    free (cvtname);
    free (fname);
  }
  osDirClose (dh);

  return fileList;
}

slist_t *
filemanipRecursiveDirList (const char *dirname, int flags)
{
  dirhandle_t   *dh;
  char          *fname;
  slist_t       *fileList;
  queue_t       *dirQueue;
  char          temp [MAXPATHLEN];
  char          *cvtname;
  char          *p;
  size_t        dirnamelen;
  gsize         bread, bwrite;
  GError        *gerr = NULL;

  if (! fileopIsDirectory (dirname)) {
    return NULL;
  }

  dirnamelen = strlen (dirname);
  snprintf (temp, sizeof (temp), "rec-dir-%s", dirname);
  fileList = slistAlloc (temp, LIST_UNORDERED, NULL);
  dirQueue = queueAlloc (free);

  queuePush (dirQueue, strdup (dirname));
  while (queueGetCount (dirQueue) > 0) {
    char  *dir;

    dir = queuePop (dirQueue);

    dh = osDirOpen (dir);
    while ((fname = osDirIterate (dh)) != NULL) {
      if (strcmp (fname, ".") == 0 ||
          strcmp (fname, "..") == 0) {
        free (fname);
        continue;
      }

      gerr = NULL;
      cvtname = g_filename_to_utf8 (fname, strlen (fname),
          &bread, &bwrite, &gerr);
      snprintf (temp, sizeof (temp), "%s/%s", dir, cvtname);
      if (cvtname != NULL) {
        if (fileopIsDirectory (temp)) {
          queuePush (dirQueue, strdup (temp));
          if ((flags & FILEMANIP_DIRS) == FILEMANIP_DIRS) {
            p = temp + dirnamelen + 1;
            slistSetStr (fileList, temp, p);
          }
        } else {
          if ((flags & FILEMANIP_FILES) == FILEMANIP_FILES) {
            p = temp + dirnamelen + 1;
            slistSetStr (fileList, temp, p);
          }
        }
      }
      free (cvtname);
      free (fname);
    }

    osDirClose (dh);
    free (dir);
  }
  queueFree (dirQueue);

  /* need it sorted so that the relative filename can be retrieved */
  slistSort (fileList);
  return fileList;
}

void
filemanipDeleteDir (const char *dirname)
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
      free (fname);
      continue;
    }

    snprintf (temp, sizeof (temp), "%s/%s", dirname, fname);
    if (fname != NULL) {
      if (fileopIsDirectory (temp)) {
        filemanipDeleteDir (temp);
      } else {
        fileopDelete (temp);
      }
    }
    free (fname);
  }

  osDirClose (dh);

#if _lib_RemoveDirectoryW
  {
    wchar_t       * tdirname;

    tdirname = osToFSFilename (dirname);
    RemoveDirectoryW (tdirname);
    free (tdirname);
  }
#else
  rmdir (dirname);
#endif
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



