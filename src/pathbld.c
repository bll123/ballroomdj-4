#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pathbld.h"
#include "sysvars.h"

char *
pathbldMakePath (char *buff, size_t buffsz, const char *subpath,
    const char *base, const char *extension, int flags)
{
  char      suffix [30];
  char      *subpathsep;
  char      *dirprefix = "data";

  *suffix = '\0';
  subpathsep = "";
  if (*subpath) {
    subpathsep = "/";
  }

  if ((flags & PATHBLD_MP_TMPDIR) == PATHBLD_MP_TMPDIR) {
    dirprefix = "tmp";
  }
  if ((flags & PATHBLD_MP_EXECDIR) == PATHBLD_MP_EXECDIR) {
    dirprefix = sysvars [SV_BDJ4EXECDIR];
  }
  if ((flags & PATHBLD_MP_MAINDIR) == PATHBLD_MP_MAINDIR) {
    dirprefix = sysvars [SV_BDJ4MAINDIR];
  }

  if ((flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX) {
    if (lsysvars [SVL_BDJIDX] != 0L) {
      snprintf (suffix, sizeof (suffix), "-%zd", lsysvars [SVL_BDJIDX]);
    }
  }
  if ((flags & PATHBLD_MP_HOSTNAME) == PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s/%s%s%s%s%s", dirprefix,
        sysvars [SV_HOSTNAME], subpath, subpathsep, base, suffix, extension);
  }
  if ((flags & PATHBLD_MP_HOSTNAME) != PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s%s%s%s%s", dirprefix,
        subpath, subpathsep, base, suffix, extension);
  }

  return buff;
}


