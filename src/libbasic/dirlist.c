/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

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

#include "bdj4.h"
#include "bdjstring.h"
#include "dirlist.h"
#include "fileop.h"
#include "mdebug.h"
#include "nodiscard.h"
#include "slist.h"
#include "osdir.h"
#include "osutils.h"
#include "pathinfo.h"
#include "queue.h"

BDJ_NODISCARD
slist_t *
dirlistBasicDirList (const char *dirname, const char *extension)
{
  dirhandle_t   *dh;
  char          *fname;
  slist_t       *fileList;
  pathinfo_t    *pi;
  char          temp [BDJ4_PATH_MAX];
  char          *p;
  char          *end;


  if (! fileopIsDirectory (dirname)) {
    return NULL;
  }

  snprintf (temp, sizeof (temp), "basic-dir-%s", dirname);
  p = temp;
  end = temp + sizeof (temp);
  if (extension != NULL) {
    p = stpecpy (p, end, "-");
    p = stpecpy (p, end, extension);
  }
  fileList = slistAlloc (temp, LIST_UNORDERED, NULL);
  dh = osDirOpen (dirname);
  while ((fname = osDirIterate (dh)) != NULL) {
    snprintf (temp, sizeof (temp), "%s/%s", dirname, fname);
    if (fileopIsDirectory (temp)) {
      mdfree (fname);
      continue;
    }

    if (extension != NULL) {
      pi = pathInfo (fname);
      if (pathInfoExtCheck (pi, extension) == false) {
        pathInfoFree (pi);
        mdfree (fname);
        continue;
      }
      pathInfoFree (pi);
    }

    /* used to use g_filename_to_utf8 (utf8-mac to utf8), */
    /* but this seems to not be necessary. */
    /* 2025-11-14 both homebrew and pkgsrc fail to do any conversion */
    /*    using g_filename_to_utf8 (utf8-mac is unknown) */
    slistSetStr (fileList, fname, NULL);
    mdfree (fname);
  }
  osDirClose (dh);

  return fileList;
}

BDJ_NODISCARD
slist_t *
dirlistRecursiveDirList (const char *dirname, int flags)
{
  dirhandle_t   *dh;
  char          *fname;
  slist_t       *fileList;
  queue_t       *dirQueue;
  char          temp [BDJ4_PATH_MAX];
  char          *p;
  size_t        dirnamelen;

  if (! fileopIsDirectory (dirname)) {
    return NULL;
  }

  dirnamelen = strlen (dirname);
  snprintf (temp, sizeof (temp), "rec-dir-%s", dirname);
  fileList = slistAlloc (temp, LIST_UNORDERED, NULL);
  dirQueue = queueAlloc ("dir-q", NULL);

  queuePush (dirQueue, mdstrdup (dirname));
  while (queueGetCount (dirQueue) > 0) {
    char  *dir;

    dir = queuePop (dirQueue);

    dh = osDirOpen (dir);
    while ((fname = osDirIterate (dh)) != NULL) {
      if (strcmp (fname, ".") == 0 ||
          strcmp (fname, "..") == 0) {
        mdfree (fname);
        continue;
      }

      snprintf (temp, sizeof (temp), "%s/%s", dir, fname);
      if ((flags & DIRLIST_LINKS) == DIRLIST_LINKS &&
          osIsLink (temp)) {
        if ((flags & DIRLIST_FILES) == DIRLIST_FILES) {
          p = temp + dirnamelen + 1;
          slistSetStr (fileList, temp, p);
        }
      } else if (fileopIsDirectory (temp)) {
        queuePush (dirQueue, mdstrdup (temp));
        if ((flags & DIRLIST_DIRS) == DIRLIST_DIRS) {
          p = temp + dirnamelen + 1;
          slistSetStr (fileList, temp, p);
        }
      } else if (fileopFileExists (temp)) {
        if ((flags & DIRLIST_FILES) == DIRLIST_FILES) {
          p = temp + dirnamelen + 1;
          slistSetStr (fileList, temp, p);
        }
      }
      mdfree (fname);
    }

    osDirClose (dh);
    mdfree (dir);
  }
  queueFree (dirQueue);

  /* need it sorted so that the relative filename can be retrieved */
  slistSort (fileList);
  return fileList;
}

