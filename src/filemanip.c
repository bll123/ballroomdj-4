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
#if _hdr_io
# include <io.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "filemanip.h"
#include "fileop.h"
#include "list.h"
#include "pathutil.h"
#include "portability.h"
#include "sysvars.h"

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
  int       rc;


  if (isWindows ()) {
    pathToWinPath (fname, tfname, MAXPATHLEN);
    pathToWinPath (nfn, tnfn, MAXPATHLEN);
    snprintf (cmd, MAXPATHLEN, "copy /y/b \"%s\" \"%s\" >NUL",
        tfname, tnfn);
  } else {
    snprintf (cmd, MAXPATHLEN, "cp -f '%s' '%s' >/dev/null 2>&1", fname, nfn);
  }
  rc = system (cmd);
  return rc;
}

/* link if possible, otherwise copy */
int
filemanipLinkCopy (char *fname, char *nfn)
{
  char      cmd [MAXPATHLEN];
  char      tfname [MAXPATHLEN];
  char      tnfn [MAXPATHLEN];
  int       rc = -1;

  if (isWindows ()) {
    pathToWinPath (fname, tfname, MAXPATHLEN);
    pathToWinPath (nfn, tnfn, MAXPATHLEN);
    snprintf (cmd, MAXPATHLEN, "copy /y/b \"%s\" \"%s\" >NUL",
        tfname, tnfn);
    rc = system (cmd);
  } else {
#if _lib_symlink
    rc = symlink (fname, nfn);
#endif
  }
  return rc;
}

list_t *
filemanipBasicDirList (char *dirname, char *extension)
{
  DIR           *dh;
  struct dirent *dirent;
  list_t        *fileList;
  pathinfo_t    *pi;
  size_t        elen = 0;

  if (extension != NULL) {
    elen = strlen (extension);
  }

  fileList = listAlloc (dirname, LIST_UNORDERED, istringCompare, free, NULL);
  dh = opendir (dirname);
  while ((dirent = readdir (dh)) != NULL) {
    if (extension != NULL) {
      pi = pathInfo (dirent->d_name);
      if (elen == pi->elen &&
          strncmp (pi->extension, extension, pi->elen) == 0) {
        listSetData (fileList, dirent->d_name, NULL);
      }
      pathInfoFree (pi);
    } else {
      listSetData (fileList, dirent->d_name, NULL);
    }
  }
  closedir (dh);
  return fileList;
}
