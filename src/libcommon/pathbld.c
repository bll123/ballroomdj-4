/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"

char *
pathbldMakePath (char *buff, size_t buffsz,
    const char *base, const char *extension, int flags)
{
  char      tbuff [BDJ4_PATH_MAX];
  char      profpath [50];
  char      dstamp [40];
  char      *dirprefix = "";


  *profpath = '\0';
  *dstamp = '\0';

  dirprefix = ".";   /* default to current directory if no path specified */
  if ((flags & PATHBLD_MP_DREL_DATA) == PATHBLD_MP_DREL_DATA) {
    /* relative */
    dirprefix = sysvarsGetStr (SV_BDJ4_DREL_DATA);
  }
  if ((flags & PATHBLD_MP_DIR_LOCK) == PATHBLD_MP_DIR_LOCK) {
    int     uid;

    uid = sysvarsGetNum (SVL_USER_ID);
    if (isLinux () && uid != SVC_USER_ID_NONE) {
      snprintf (tbuff, sizeof (tbuff), "/var/run/user/%d/bdj4", uid);
      dirprefix = tbuff;
    }
    if (! isLinux () || uid == SVC_USER_ID_NONE) {
      dirprefix = sysvarsGetStr (SV_BDJ4_DREL_TMP);
    }
  }

  /* relative prefixes */
  if ((flags & PATHBLD_MP_DREL_TMP) == PATHBLD_MP_DREL_TMP) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DREL_TMP);
  }
  if ((flags & PATHBLD_MP_DREL_HTTP) == PATHBLD_MP_DREL_HTTP) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DREL_HTTP);
  }
  if ((flags & PATHBLD_MP_DREL_IMG) == PATHBLD_MP_DREL_IMG) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DREL_IMG);
  }
  if ((flags & PATHBLD_MP_DREL_TEST_TMPL) == PATHBLD_MP_DREL_TEST_TMPL) {
    dirprefix = "test-templates";
  }

  /* absolute prefixes */
  if ((flags & PATHBLD_MP_DIR_DATATOP) == PATHBLD_MP_DIR_DATATOP) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_DATATOP);
  }
  if ((flags & PATHBLD_MP_DIR_EXEC) == PATHBLD_MP_DIR_EXEC) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_EXEC);
  }
  if ((flags & PATHBLD_MP_DIR_MAIN) == PATHBLD_MP_DIR_MAIN) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_MAIN);
  }
  if ((flags & PATHBLD_MP_DIR_TEMPLATE) == PATHBLD_MP_DIR_TEMPLATE) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_TEMPLATE);
  }
  if ((flags & PATHBLD_MP_DIR_LOCALE) == PATHBLD_MP_DIR_LOCALE) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_LOCALE);
  }
  if ((flags & PATHBLD_MP_DIR_CONFIG) == PATHBLD_MP_DIR_CONFIG) {
    dirprefix = sysvarsGetStr (SV_DIR_CONFIG);
  }
  if ((flags & PATHBLD_MP_DIR_CACHE) == PATHBLD_MP_DIR_CACHE) {
    dirprefix = sysvarsGetStr (SV_DIR_CACHE);
  }
  if ((flags & PATHBLD_MP_DIR_IMG) == PATHBLD_MP_DIR_IMG) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_IMG);
  }
  if ((flags & PATHBLD_MP_DIR_INST) == PATHBLD_MP_DIR_INST) {
    dirprefix = sysvarsGetStr (SV_BDJ4_DIR_INST);
  }
  if ((flags & PATHBLD_MP_DIR_ICON) == PATHBLD_MP_DIR_ICON) {
    dirprefix = sysvarsGetStr (SV_PATH_ICONDIR);
  }
  if ((flags & PATHBLD_MP_DSTAMP) == PATHBLD_MP_DSTAMP) {
    tmutilDstamp (dstamp, sizeof (dstamp));
  }

  if ((flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX) {
    if ((flags & PATHBLD_MP_DIR_LOCK) == PATHBLD_MP_DIR_LOCK ||
        (flags & PATHBLD_MP_DREL_TMP) == PATHBLD_MP_DREL_TMP) {
      /* if the lock/tmp dir is being used, there is no prefix directory */
      /* use a filename prefix */
      snprintf (profpath, sizeof (profpath), "l%02d-%02d-",
          (int) sysvarsGetNum (SVL_ALTIDX), (int) sysvarsGetNum (SVL_PROFILE_IDX));
    } else {
      snprintf (profpath, sizeof (profpath), "profile%02" PRId64 "/",
          sysvarsGetNum (SVL_PROFILE_IDX));
    }
  }
  if ((flags & PATHBLD_MP_HOSTNAME) == PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s/%s%s%s%s", dirprefix,
        sysvarsGetStr (SV_HOSTNAME), profpath, base, dstamp, extension);
  }
  if ((flags & PATHBLD_MP_HOSTNAME) != PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s%s%s%s", dirprefix,
        profpath, base, dstamp, extension);
  }
  stringTrimChar (buff, '/');

  return buff;
}


