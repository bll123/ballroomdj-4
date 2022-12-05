/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "instutil.h"
#include "osprocess.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"

void
instutilCreateShortcut (const char *name, const char *maindir,
    const char *target, int profilenum)
{
  char        path [MAXPATHLEN];
  char        buff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        tmp [40];
  const char  *targv [10];
  int         targc = 0;

  if (profilenum < 0) {
    profilenum = 0;
  }

  if (isWindows ()) {
    if (! chdir ("install")) {
      targv [targc++] = ".\\winshortcut.bat";
      snprintf (path, sizeof (path), "\"%s%s.lnk\"",
          "%USERPROFILE%\\Desktop\\", name);
      targv [targc++] = path;
      pathbldMakePath (buff, sizeof (buff),
          "bdj4", ".exe", PATHBLD_MP_EXECDIR);
      pathWinPath (buff, sizeof (buff));
      targv [targc++] = buff;
      strlcpy (tbuff, target, sizeof (tbuff));
      pathWinPath (tbuff, sizeof (tbuff));
      targv [targc++] = tbuff;
      snprintf (tmp, sizeof (tmp), "%d", profilenum);
      targv [targc++] = tmp;
      targv [targc++] = NULL;
      osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
      (void) ! chdir (maindir);
    }
  }

  if (isLinux ()) {
    snprintf (buff, sizeof (buff),
        "./install/linuxshortcut.sh '%s' '%s' '%s' %d",
        name, maindir, target, profilenum);
    (void) ! system (buff);
  }
}

