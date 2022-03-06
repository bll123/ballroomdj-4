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
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "filemanip.h"
#include "fileop.h"
#include "slist.h"
#include "pathutil.h"
#include "sysvars.h"

#if _hdr_windows
# include <windows.h>
#endif

int
filemanipMove (char *fname, char *nfn)
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
filemanipCopy (char *fname, char *nfn)
{
  char      cmd [MAXPATHLEN];
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];
  int       rc = -1;


  if (isWindows ()) {
    pathToWinPath (tfname, fname, MAXPATHLEN);
    pathToWinPath (tnfn, nfn, MAXPATHLEN);
#if _lib_CopyFile
    rc = CopyFile (tfname, tnfn, 0);
    rc = (rc == 0);
#endif
  } else {
    snprintf (cmd, sizeof (cmd), "cp -f '%s' '%s' >/dev/null 2>&1", fname, nfn);
    rc = system (cmd);
  }
  return rc;
}

/* link if possible, otherwise copy */
/* windows has had symlinks for ages, but they require either */
/* admin permission, or have the machine in developer mode */
int
filemanipLinkCopy (char *fname, char *nfn)
{
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];
  int       rc = -1;

  if (isWindows ()) {
    pathToWinPath (tfname, fname, MAXPATHLEN);
    pathToWinPath (tnfn, nfn, MAXPATHLEN);
#if _lib_CopyFile
    CopyFile (tfname, tnfn, 0);
#endif
  } else {
#if _lib_symlink
    rc = symlink (fname, nfn);
#endif
  }
  return rc;
}

slist_t *
filemanipBasicDirList (char *dirname, char *extension)
{
  DIR           *dh;
  struct dirent *dirent;
  slist_t       *fileList;
  pathinfo_t    *pi;
  char          temp [100];


  snprintf (temp, sizeof (temp), "read-dir-%s", dirname);
  if (extension != NULL) {
    strlcat (temp, "-", sizeof (temp));
    strlcat (temp, extension, sizeof (temp));
  }
  fileList = slistAlloc (temp, LIST_UNORDERED, NULL);
  dh = opendir (dirname);
  while ((dirent = readdir (dh)) != NULL) {
    if (strcmp (dirent->d_name, ".") == 0 ||
        strcmp (dirent->d_name, "..") == 0) {
      continue;
    }
    if (extension != NULL) {
      pi = pathInfo (dirent->d_name);
      if (pathInfoExtCheck (pi, extension)) {
        slistSetStr (fileList, dirent->d_name, NULL);
      }
      pathInfoFree (pi);
    } else {
      char    *cvtname;
      gsize   bread, bwrite;
      GError  *gerr;

      cvtname = g_filename_to_utf8 (dirent->d_name, strlen (dirent->d_name),
          &bread, &bwrite, &gerr);
      if (cvtname != NULL) {
        slistSetStr (fileList, cvtname, NULL);
      }
      free (cvtname);
    }
  }
  closedir (dh);
  return fileList;
}

void
filemanipDeleteDir (const char *dir)
{
  char      cmd [MAXPATHLEN];
  char      tdir [MAXPATHLEN];

  if (! fileopIsDirectory (dir)) {
    return;
  }

  if (isWindows ()) {
    pathToWinPath (tdir, dir, sizeof (tdir));
    snprintf (cmd, sizeof (cmd), "rmdir /s/q \"%s\" >NUL", tdir);
  } else {
    snprintf (cmd, sizeof (cmd), "rm -rf '%s' >/dev/null 2>&1", dir);
  }
  system (cmd);
}

