/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#define _GNU_SOURCE 1   // for statx()

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
#include <utime.h>
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

#include "fileop.h"
#include "mdebug.h"
#include "osutils.h"

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

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) == S_IFDIR) {
      rc = -1;
    }
    mdfree (tfname);
  }
#else
  {
    struct stat   statbuf;
    rc = stat (fname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) == S_IFDIR) {
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

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      sz = statbuf.st_size;
    }
    mdfree (tfname);
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

time_t
fileopModTime (const char *fname)
{
  time_t  mtime = 0;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      mtime = statbuf.st_mtime;
    }
    mdfree (tfname);
  }
#else
  {
    int rc;
    struct stat statbuf;

    rc = stat (fname, &statbuf);
    if (rc == 0) {
      mtime = statbuf.st_mtime;
    }
  }
#endif
  return mtime;
}

void
fileopSetModTime (const char *fname, time_t mtime)
{
#if _lib__wstat && _lib__wutime
  {
    struct _stat  statbuf;
    struct _utimbuf utimebuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      utimebuf.actime = statbuf.st_atime;
      utimebuf.modtime = mtime;
      _wutime (tfname, &utimebuf);
    }
    mdfree (tfname);
  }
#else
  {
    int rc;
    struct stat statbuf;
    struct utimbuf utimebuf;

    rc = stat (fname, &statbuf);
    if (rc == 0) {
      utimebuf.actime = statbuf.st_atime;
      utimebuf.modtime = mtime;
      utime (fname, &utimebuf);
    }
  }
#endif
}

time_t
fileopCreateTime (const char *fname)
{
  time_t  ctime = 0;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0) {
      ctime = statbuf.st_ctime;
    }
    mdfree (tfname);
  }
#else
  {
    int rc = -1;
#if _lib_statx && _GNU_SOURCE
    struct statx statbufx;
#endif
    struct stat statbuf;

#if _lib_statx && _GNU_SOURCE
    rc = statx (0, fname, 0, STATX_BTIME, &statbufx);
    if (rc == 0) {
      if ((statbufx.stx_mask & STATX_BTIME) == STATX_BTIME) {
        ctime = statbufx.stx_btime.tv_sec;
      }
    }
#endif

    if (ctime == 0) {
      rc = stat (fname, &statbuf);
      if (rc == 0) {
#if _mem_struct_stat_st_birthtime
        ctime = statbuf.st_birthtime;
#else
        ctime = statbuf.st_ctime;
#endif
      }
    }
  }
#endif
  return ctime;
}


bool
fileopIsDirectory (const char *fname)
{
  int         rc;

#if _lib__wstat
  {
    struct _stat  statbuf;
    wchar_t       *tfname = NULL;

    tfname = osToWideChar (fname);
    rc = _wstat (tfname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFDIR) {
      rc = -1;
    }
    mdfree (tfname);
  }
#else
  {
    struct stat   statbuf;
    rc = stat (fname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFDIR) {
      rc = -1;
    }
  }
#endif
  return (rc == 0);
}

int
fileopDelete (const char *fname)
{
  int     rc;
#if _lib__wunlink
  wchar_t *tname;
#endif

  if (fname == NULL) {
    return 0;
  }

#if _lib__wunlink
  tname = osToWideChar (fname);
  rc = _wunlink (tname);
  mdfree (tname);
#else
  rc = unlink (fname);
#endif
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

    tfname = osToWideChar (fname);
    tmode = osToWideChar (mode);
    fh = _wfopen (tfname, tmode);
    mdfree (tfname);
    mdfree (tmode);
  }
#else
  {
    fh = fopen (fname, mode);
  }
#endif
  return fh;
}

