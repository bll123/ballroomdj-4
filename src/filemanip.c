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
#if _hdr_unistd
# include <unistd.h>
#endif
#if _hdr_io
# include <io.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "filemanip.h"
#include "fileop.h"
#include "bdjstring.h"
#include "portability.h"
#include "pathutil.h"
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

