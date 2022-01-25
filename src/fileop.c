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

#include "fileop.h"

inline bool
fileopExists (char *fname)
{
  struct stat   statbuf;

  int rc = stat (fname, &statbuf);
  return (rc == 0);
}

inline int
fileopDelete (const char *fname)
{
  int rc = unlink (fname);
  return rc;
}

int
fileopMakeDir (char *dirname)
{
  int rc;
#if _args_mkdir == 2 && _define_S_IRWXU
  rc = mkdir (dirname, S_IRWXU);
#endif
#if _args_mkdir == 1
  rc = mkdir (dirname);
#endif
  return rc;
}

