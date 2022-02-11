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
  char      *dirprefix;

  *suffix = '\0';
  subpathsep = "";
  if (*subpath) {
    subpathsep = "/";
  }

  dirprefix = sysvarsGetStr (SV_BDJ4DATADIR);
  if ((flags & PATHBLD_MP_TMPDIR) == PATHBLD_MP_TMPDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4TMPDIR);
  }
  if ((flags & PATHBLD_MP_HTTPDIR) == PATHBLD_MP_HTTPDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4HTTPDIR);
  }
  if ((flags & PATHBLD_MP_EXECDIR) == PATHBLD_MP_EXECDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4EXECDIR);
  }
  if ((flags & PATHBLD_MP_MAINDIR) == PATHBLD_MP_MAINDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4MAINDIR);
  }
  if ((flags & PATHBLD_MP_TEMPLATEDIR) == PATHBLD_MP_TEMPLATEDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4TEMPLATEDIR);
  }
  if ((flags & PATHBLD_MP_IMGDIR) == PATHBLD_MP_IMGDIR) {
    if ((flags & PATHBLD_MP_RELATIVE) == PATHBLD_MP_RELATIVE) {
      dirprefix = "img";
    } else {
      dirprefix = sysvarsGetStr (SV_BDJ4IMGDIR);
    }
  }

  if ((flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX) {
    if (sysvarsGetNum (SVL_BDJIDX) != 0L) {
      snprintf (suffix, sizeof (suffix), "-%zd", sysvarsGetNum (SVL_BDJIDX));
    }
  }
  if ((flags & PATHBLD_MP_HOSTNAME) == PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s/%s%s%s%s%s", dirprefix,
        sysvarsGetStr (SV_HOSTNAME), subpath, subpathsep, base, suffix, extension);
  }
  if ((flags & PATHBLD_MP_HOSTNAME) != PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s%s%s%s%s", dirprefix,
        subpath, subpathsep, base, suffix, extension);
  }

  return buff;
}


