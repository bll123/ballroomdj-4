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

#include "bdjstring.h"
#include "fileop.h"
#include "portability.h"

static int fileopMakeRecursiveDir (const char *dirname);
static int fileopMkdir (const char *dirname);

inline bool
fileopExists (const char *fname)
{
  struct stat   statbuf;

  int rc = stat (fname, &statbuf);
  return (rc == 0);
}

inline bool
fileopIsDirectory (const char *fname)
{
  struct stat   statbuf;

  int rc = stat (fname, &statbuf);
  return ((statbuf.st_mode & S_IFDIR) == S_IFDIR);
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
#if _args_mkdir == 2 && _define_S_IRWXU
  rc = mkdir (dirname, S_IRWXU);
#endif
#if _args_mkdir == 1
  rc = mkdir (dirname);
#endif
  return rc;
}
