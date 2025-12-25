/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <ctype.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "pathinfo.h"
#include "sysvars.h"

static int32_t globalcount = 0;

void
audiosrcutilMakeTempName (const char *ffn, char *tempnm, size_t maxlen)
{
  char        tnm [BDJ4_PATH_MAX];
  size_t      idx;
  pathinfo_t  *pi;

  pi = pathInfo (ffn);

  idx = 0;
  for (const char *p = pi->filename;
      *p && idx < maxlen && idx < pi->flen; ++p) {
    if ((isascii (*p) && isalnum (*p)) ||
        *p == '.' || *p == '-' || *p == '_') {
      tnm [idx++] = *p;
    }
  }
  tnm [idx] = '\0';
  pathInfoFree (pi);

  /* the profile index so we don't stomp on other bdj instances   */
  /* the global count so we don't stomp on ourselves              */
  snprintf (tempnm, maxlen, "tmp/%02" PRId64 "-%03" PRId32 "-%s",
      sysvarsGetNum (SVL_PROFILE_IDX), globalcount, tnm);
  ++globalcount;
}

bool
audiosrcutilPreCacheFile (const char *fn)
{
  ssize_t   fsz;
  int       frc;
  char      *buff;
  FILE      *fh;

  /* read the entire file in order to get it into the operating system's */
  /* filesystem cache */
  fsz = fileopSize (fn);
  if (fsz <= 0) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file size 0: %s", fn);
    return false;
  }
  buff = mdmalloc (fsz);
  fh = fileopOpen (fn, "rb");
  frc = fread (buff, fsz, 1, fh);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
  if (frc != 1) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file read failed: %s", fn);
    return false;
  }
  return true;
}
