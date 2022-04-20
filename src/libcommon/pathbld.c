#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdjstring.h"
#include "pathbld.h"
#include "sysvars.h"

char *
pathbldMakePath (char *buff, size_t buffsz,
    const char *base, const char *extension, int flags)
{
  char      profpath [50];
  char      *dirprefix = "";


  *profpath = '\0';

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
  if ((flags & PATHBLD_MP_LOCALEDIR) == PATHBLD_MP_LOCALEDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4LOCALEDIR);
  }
  if ((flags & PATHBLD_MP_IMGDIR) == PATHBLD_MP_IMGDIR) {
    if ((flags & PATHBLD_MP_RELATIVE) == PATHBLD_MP_RELATIVE) {
      dirprefix = "img";
    } else {
      dirprefix = sysvarsGetStr (SV_BDJ4IMGDIR);
    }
  }

  if ((flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX) {
    if ((flags & PATHBLD_MP_TMPDIR) == PATHBLD_MP_TMPDIR) {
      /* if the tmp dir is being used, there is no pre-made directory */
      /* use a prefix */
      snprintf (profpath, sizeof (profpath), "t%02zd-", sysvarsGetNum (SVL_BDJIDX));
    } else {
      snprintf (profpath, sizeof (profpath), "profile%02zd/", sysvarsGetNum (SVL_BDJIDX));
    }
  }
  if ((flags & PATHBLD_MP_HOSTNAME) == PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s/%s%s%s", dirprefix,
        sysvarsGetStr (SV_HOSTNAME), profpath, base, extension);
  }
  if ((flags & PATHBLD_MP_HOSTNAME) != PATHBLD_MP_HOSTNAME) {
    snprintf (buff, buffsz, "%s/%s%s%s", dirprefix,
        profpath, base, extension);
  }
  stringTrimChar (buff, '/');

  return buff;
}


