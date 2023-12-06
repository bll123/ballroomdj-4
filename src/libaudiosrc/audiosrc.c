/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pathbld.h"
#include "sysvars.h"

#define AUDIOSRC_FILE       "file://"
#define AUDIOSRC_YOUTUBE    "https://www.youtube.com/"
#define AUDIOSRC_YOUTUBE_S  "https://youtu.be/"
#define AUDIOSRC_YOUTUBE_M  "https://m.youtube.com/"
enum {
  AUDIOSRC_FILE_LEN = strlen (AUDIOSRC_FILE),
  AUDIOSRC_YOUTUBE_LEN = strlen (AUDIOSRC_YOUTUBE),
  AUDIOSRC_YOUTUBE_S_LEN = strlen (AUDIOSRC_YOUTUBE_S),
  AUDIOSRC_YOUTUBE_M_LEN = strlen (AUDIOSRC_YOUTUBE_M),
};

int
audiosrcGetType (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;

  if (fileopIsAbsolutePath (nm)) {
    type = AUDIOSRC_TYPE_FILE;
  } else if (strncmp (nm, AUDIOSRC_FILE, AUDIOSRC_FILE_LEN) == 0) {
    type = AUDIOSRC_TYPE_FILE;
  } else if (strncmp (nm, AUDIOSRC_YOUTUBE, AUDIOSRC_YOUTUBE_LEN) == 0) {
    type = AUDIOSRC_TYPE_YOUTUBE;
  } else if (strncmp (nm, AUDIOSRC_YOUTUBE_S, AUDIOSRC_YOUTUBE_S_LEN) == 0) {
    type = AUDIOSRC_TYPE_YOUTUBE;
  } else if (strncmp (nm, AUDIOSRC_YOUTUBE_M, AUDIOSRC_YOUTUBE_M_LEN) == 0) {
    type = AUDIOSRC_TYPE_YOUTUBE;
  }

  return type;
}

bool
audiosrcExists (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    rc = false;


  if (nm == NULL) {
    return false;
  }

  type = audiosrcGetType (nm);

  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfileExists (nm);
  }

  return rc;
}

bool
audiosrcRemove (const char *nm)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    rc = false;

  type = audiosrcGetType (nm);

  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfileRemove (nm);
  }

  return rc;
}

bool
audiosrcPrep (const char *sfname, char *tempnm, size_t sz)
{
  int     type = AUDIOSRC_TYPE_FILE;
  bool    rc;

  type = audiosrcGetType (sfname);

  if (type == AUDIOSRC_TYPE_FILE) {
    rc = audiosrcfilePrep (sfname, tempnm, sz);
  } else {
    /* in most cases, just return the same URI */
    strlcpy (tempnm, sfname, sz);
    rc = true;
  }

  return rc;
}

void
audiosrcPrepClean (const char *sfname, const char *tempnm)
{
  int   type = AUDIOSRC_TYPE_FILE;

  type = audiosrcGetType (sfname);

  if (type == AUDIOSRC_TYPE_FILE) {
    audiosrcfilePrepClean (tempnm);
  }
}

void
audiosrcFullPath (const char *sfname, char *fullpath, size_t sz)
{
  int     type = AUDIOSRC_TYPE_FILE;

  type = audiosrcGetType (sfname);

  *fullpath = '\0';
  if (type == AUDIOSRC_TYPE_FILE) {
    audiosrcfileFullPath (sfname, fullpath, sz);
  } else {
    strlcpy (fullpath, sfname, sz);
  }
}




