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

  /* stat() on windows does some sort of filename conversion. */
  /* this is not wanted */
#if _lib_CreateFile
  HANDLE    handle;

  handle = CreateFile (fname,
      GENERIC_READ,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      0,      // FILE_FLAG_BACKUP_SEMANTICS is not specified, dirs will fail.
      NULL);
  rc = -1;
  if (handle != INVALID_HANDLE_VALUE) {
    CloseHandle (handle);
    rc = 0;
  }
#else
  {
    struct stat   statbuf;
    rc = stat (fname, &statbuf);
    /* match how the windows code works */
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

  /* stat() on windows does some sort of filename conversion. */
  /* this is not wanted */
#if _lib_CreateFile
  HANDLE    handle;
  LARGE_INTEGER lisz;

  handle = CreateFile (fname,
      GENERIC_READ,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      0,
      NULL);
  if (handle != INVALID_HANDLE_VALUE) {
    GetFileSizeEx (handle, &lisz);
    sz = lisz.QuadPart;
    CloseHandle (handle);
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

  /* stat() on windows does some sort of filename conversion. */
  /* this is not wanted */
#if _lib_CreateFile
  HANDLE    handle;
  FILE_BASIC_INFO basicInfo;

  handle = CreateFile (fname,
      GENERIC_READ,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS,
      NULL);
  rc = -1;
  if (handle != INVALID_HANDLE_VALUE) {
    GetFileInformationByHandleEx (handle,
        FileBasicInfo, &basicInfo, sizeof (basicInfo));
    if ((basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
        FILE_ATTRIBUTE_DIRECTORY) {
      rc = 0;
    }
    CloseHandle (handle);
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
