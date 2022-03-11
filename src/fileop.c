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

static int fileopMakeRecursiveDir (const char *dirname);
static int fileopMkdir (const char *dirname);

/* note that the windows code will fail on a directory */
/* the unix code has been modified to match */
bool
fileopFileExists (const char *fname)
{
  int           rc;

#if _lib_MultiByteToWideChar
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;

    tfname = fileopToWideString (fname);
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

#if _lib_MultiByteToWideChar
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = fileopToWideString (fname);
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

#if _lib_MultiByteToWideChar
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;

    tfname = fileopToWideString (fname);
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
  int rc = unlink (fname);
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

#if _lib_MultiByteToWideChar
  {
    wchar_t       *tfname = NULL;
    wchar_t       *tmode = NULL;

    tfname = fileopToWideString (fname);
    tmode = fileopToWideString (mode);
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

#if _lib_MultiByteToWideChar

wchar_t *
fileopToWideString (const char *fname)
{
  size_t      len;
  wchar_t     *tfname = NULL;

  /* the documentation lies; len does not include room for the null byte */
  len = MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), NULL, 0);
  tfname = malloc ((len + 1) * sizeof (wchar_t));
  assert (tfname != NULL);
  MultiByteToWideChar (CP_UTF8, 0, fname, strlen (fname), tfname, len);
  tfname [len] = L'\0';
  return tfname;
}

#endif

/* internal routines */

static int
fileopMakeRecursiveDir (const char *dirname)
{
  char    tbuff [MAXPATHLEN];
  char    *p = NULL;
  size_t  len;

  strlcpy (tbuff, dirname, MAXPATHLEN);
  len = strlen (tbuff);
  if (tbuff [len - 1] == '/') {
    tbuff [len - 1] = '\0';
  }

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

  tdirname = fileopToWideString (dirname);
  rc = _wmkdir (tdirname);
  free (tdirname);
#endif
#if _args_mkdir == 2 && _define_S_IRWXU
  rc = mkdir (dirname, S_IRWXU);
#endif
  return rc;
}

