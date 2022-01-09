#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "datautil.h"
#include "sysvars.h"

char *
datautilMakePath (char *buff, size_t buffsz, const char *subpath,
    const char *base, const char *extension, int flags)
{
  char      suffix [30];
  char      *subpathsep;

  *suffix = '\0';
  subpathsep = "";
  if (*subpath) {
    subpathsep = "/";
  }

  if ((flags & DATAUTIL_MP_USEIDX) == DATAUTIL_MP_USEIDX) {
    if (lsysvars [SVL_BDJIDX] != 0L) {
      snprintf (suffix, sizeof (suffix), "-%ld", lsysvars [SVL_BDJIDX]);
    }
  }
  if ((flags & DATAUTIL_MP_HOSTNAME) == DATAUTIL_MP_HOSTNAME) {
    snprintf (buff, buffsz, "data/%s/%s%s%s%s%s", sysvars [SV_HOSTNAME],
        subpath, subpathsep, base, suffix, extension);
  }
  if ((flags & DATAUTIL_MP_HOSTNAME) != DATAUTIL_MP_HOSTNAME) {
    snprintf (buff, buffsz, "data/%s%s%s%s%s",
        subpath, subpathsep, base, suffix, extension);
  }

  return buff;
}


