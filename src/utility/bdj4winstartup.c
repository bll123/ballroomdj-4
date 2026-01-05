/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "instutil.h"
#include "osenv.h"
#include "osprocess.h"
#include "osutils.h"
#include "pathutil.h"

int
main (int argc, char *argv [])
{
  char        home [BDJ4_PATH_MAX];
  char        tbuff [BDJ4_PATH_MAX];
  char        instdir [BDJ4_PATH_MAX];
  FILE        *fh;
  const char  *targv [10];
  int         targc = 0;
  int         flags;
  int         rc;

  osGetEnv ("USERPROFILE", home, sizeof (home));
  pathNormalizePath (home, sizeof (home));
  snprintf (tbuff, sizeof (tbuff), "%s/AppData/Roaming/BDJ4/%s%s",
      home, INST_PATH_FN, BDJ4_CONFIG_EXT);
  if (! fileopFileExists (tbuff)) {
    return 0;
  }

  fh = fileopOpen (tbuff, "r");
  fgets (instdir, sizeof (instdir), fh);
  fclose (fh);
  stringTrim (instdir);

  snprintf (tbuff, sizeof (tbuff), "%s%s/bin/bdj4.exe",
      home, instdir + strlen (WINUSERPROFILE));
  if (! fileopFileExists (tbuff)) {
    return 0;
  }

  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4cleantmp";
  targv [targc++] = NULL;

  flags = OS_PROC_NONE;
  /* if detach is used, a command window gets opened */
  flags |= OS_PROC_NOWINDOW;
  rc = osProcessStart (targv, flags, NULL, NULL);

  return 0;
}
