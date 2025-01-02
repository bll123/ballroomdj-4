/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <stdarg.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"

#if _lib_symlink    // both linux and macos

int
osCreateLink (const char *target, const char *linkpath)
{
  int rc;

  rc = symlink (target, linkpath);
  return rc;
}

bool
osIsLink (const char *path)
{
  struct stat statbuf;
  int         rc;

  rc = lstat (path, &statbuf);
  if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFLNK) {
    rc = -1;
  }
  return (rc == 0);
}

#endif

