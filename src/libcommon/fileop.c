/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#define _GNU_SOURCE 1   // for statx()

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <wchar.h>

#if _hdr_io
# include <io.h>
#endif
#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
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
  int   rc;
  bool  brc = 0;

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
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
  if (rc == 0) {
    brc = 1;
  }

  return brc;
}

ssize_t
fileopSize (const char *fname)
{
  ssize_t       sz = -1;

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
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

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
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
#if _lib__wstat64 && _lib__wutime
  {
    struct __stat64  statbuf;
    struct _utimbuf utimebuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
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

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;
    int           rc;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
    if (rc == 0) {
      ctime = statbuf.st_ctime;
    }
    mdfree (tfname);
  }
#else
  {
    int rc = -1;
# if _lib_statx && _GNU_SOURCE
    struct statx statbufx;
# endif
    struct stat statbuf;

# if _lib_statx && _GNU_SOURCE
    rc = statx (0, fname, 0, STATX_BTIME, &statbufx);
    if (rc == 0) {
      if ((statbufx.stx_mask & STATX_BTIME) == STATX_BTIME) {
        ctime = statbufx.stx_btime.tv_sec;
      }
    }
# endif

    if (ctime == 0) {
      rc = stat (fname, &statbuf);
      if (rc == 0) {
# if _mem_struct_stat_st_birthtime
        ctime = statbuf.st_birthtime;
# else
        /* use the smaller of ctime and mtime */
        if (statbuf.st_ctime < statbuf.st_mtime) {
          ctime = statbuf.st_ctime;
        } else {
          ctime = statbuf.st_mtime;
        }
# endif
      }
    }
  }
#endif
  return ctime;
}


bool
fileopIsDirectory (const char *fname)
{
  int   rc;
  bool  brc = 0;

#if _lib__wstat64
  {
    struct __stat64  statbuf;
    wchar_t       *tfname = NULL;

    tfname = osToWideChar (fname);
    rc = _wstat64 (tfname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFDIR) {
      rc = -1;
    }
    mdfree (tfname);
  }
#else
  {
    struct stat   statbuf;

    memset (&statbuf, '\0', sizeof (struct stat));
    rc = stat (fname, &statbuf);
    if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFDIR) {
      rc = -1;
    }
  }
#endif
  /* had some trouble with the optimizer, so spell out the values */
  if (rc == 0) {
    brc = 1;
  }

  return brc;
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
  FILE          *fh = NULL;

#if _lib__wfopen_s
  {
    wchar_t       *tfname = NULL;
    wchar_t       *tmode = NULL;

    tfname = osToWideChar (fname);
    tmode = osToWideChar (mode);
    _wfopen_s (&fh, tfname, tmode);
    mdextfopen (fh);
    mdfree (tfname);
    mdfree (tmode);
  }
#else
  {
    fh = fopen (fname, mode);
    mdextfopen (fh);
  }
#endif
  return fh;
}

bool
fileopIsAbsolutePath (const char *fname)
{
  bool    rc = false;
  size_t  len;

  len = strlen (fname);
  if ((len > 0 && fname [0] == '/') ||
      (len > 2 && fname [1] == ':' && fname [2] == '/')) {
    rc = true;
  }

  return rc;
}

