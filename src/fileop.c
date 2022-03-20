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

#if _hdr_io
# include <io.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "osutils.h"

static int fileopMakeRecursiveDir (const char *dirname);
static int fileopMkdir (const char *dirname);

/* note that the windows code will fail on a directory */
/* the unix code has been modified to match */
bool
fileopFileExists (const char *fname)
{
  int           rc;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;

    tfname = osToFSFilename (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFDIR) == S_IFDIR) {
      rc = -1;
    }
    free (tfname);
  }
#else
  {
    struct stat   statbuf;
    rc = stat (fname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFDIR) == S_IFDIR) {
      rc = -1;
    }
  }
#endif
  return (rc == 0);
}

ssize_t
fileopSize (const char *fname)
{
  ssize_t       sz = -1;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToFSFilename (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
    free (tfname);
  }
#else
  {
    int rc;
    struct stat statbuf;

    rc = stat (fname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
  }
#endif
  return sz;
}

bool
fileopIsDirectory (const char *fname)
{
  int         rc;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;

    tfname = osToFSFilename (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFDIR) != S_IFDIR) {
      rc = -1;
    }
    free (tfname);
  }
#else
  {
    struct stat   statbuf;
    rc = stat (fname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFDIR) != S_IFDIR) {
      rc = -1;
    }
  }
#endif
  return (rc == 0);
}

inline int
fileopDelete (const char *fname)
{
  int     rc;

#if _lib__wunlink
  wchar_t *tname;
  tname = osToFSFilename (fname);
  rc = _wunlink (tname);
  free (tname);
#else
  rc = unlink (fname);
#endif
  return rc;
}

int
fileopMakeDir (const char *dirname)
{
  int rc;
  rc = fileopMakeRecursiveDir (dirname);
  return rc;
}

FILE *
fileopOpen (const char *fname, const char *mode)
{
  FILE          *fh;

#if _lib__wfopen
  {
    wchar_t       *tfname = NULL;
    wchar_t       *tmode = NULL;

    tfname = osToFSFilename (fname);
    tmode = osToFSFilename (mode);
    fh = _wfopen (tfname, tmode);
    free (tfname);
    free (tmode);
  }
#else
  {
    fh = fopen (fname, mode);
  }
#endif
  return fh;
}

/* internal routines */

static int
fileopMakeRecursiveDir (const char *dirname)
{
  char    tbuff [MAXPATHLEN];
  char    *p = NULL;

  strlcpy (tbuff, dirname, MAXPATHLEN);
  stringTrimChar (tbuff, '/');

  for (p = tbuff + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      fileopMkdir (tbuff);
      *p = '/';
    }
  }
  fileopMkdir (tbuff);
  return 0;
}

static int
fileopMkdir (const char *dirname)
{
  int   rc;

#if _args_mkdir == 1      // windows
  wchar_t   *tdirname = NULL;

  tdirname = osToFSFilename (dirname);
  rc = _wmkdir (tdirname);
  free (tdirname);
#endif
#if _args_mkdir == 2 && _define_S_IRWXU
  rc = mkdir (dirname, S_IRWXU);
#endif
  return rc;
}

