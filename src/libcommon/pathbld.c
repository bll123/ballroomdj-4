#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "bdjstring.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"

char *
pathbldMakePath (char *buff, size_t buffsz,
    const char *base, const char *extension, int flags)
{
  char      profpath [50];
  char      dstamp [40];
  char      *dirprefix = "";


  *profpath = '\0';
  *dstamp = '\0';

  dirprefix = ".";   /* default to current directory if no path specified */
  if ((flags & PATHBLD_MP_DATA) == PATHBLD_MP_DATA) {
    dirprefix = sysvarsGetStr (SV_BDJ4DATADIR);
  }
  if ((flags & PATHBLD_MP_DATATOPDIR) == PATHBLD_MP_DATATOPDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4DATATOPDIR);
  }
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
  if ((flags & PATHBLD_MP_CONFIGDIR) == PATHBLD_MP_CONFIGDIR) {
    dirprefix = sysvarsGetStr (SV_CONFIG_DIR);
  }
  if ((flags & PATHBLD_MP_IMGDIR) == PATHBLD_MP_IMGDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4IMGDIR);
  }
  if ((flags & PATHBLD_MP_INSTDIR) == PATHBLD_MP_INSTDIR) {
    dirprefix = sysvarsGetStr (SV_BDJ4INSTDIR);
  }
  if ((flags & PATHBLD_MP_DSTAMP) == PATHBLD_MP_DSTAMP) {
    tmutilDstamp (dstamp, sizeof (dstamp));
  }

  if ((flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX) {
    if ((flags & PATHBLD_MP_TMPDIR) == PATHBLD_MP_TMPDIR) {
      /* if the tmp dir is being used, there is no prefix directory */
      /* use a filename prefix */
      snprintf (profpath, sizeof (profpath), "l%02"PRId64"-", sysvarsGetNum (SVL_BDJIDX));
    } else {
      snprintf (profpath, sizeof (profpath), "profile%02"PRId64"/", sysvarsGetNum (SVL_BDJIDX));
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


