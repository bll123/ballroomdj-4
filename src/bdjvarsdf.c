#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdjvarsdf.h"
#include "datafile.h"
#include "dncspeeds.h"
#include "dnctypes.h"
#include "genre.h"
#include "level.h"
#include "rating.h"
#include "sortopt.h"
#include "portability.h"
#include "datautil.h"

datafile_t *bdjvarsdf [BDJVDF_MAX];

void
bdjvarsdfInit (void)
{
  char      fn [MAXPATHLEN];

  datautilMakePath (fn, sizeof (fn), "", "dancespeeds", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_DANCE_SPEEDS] = dncspeedsAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "dancetypes", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_DANCE_TYPES] = dnctypesAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "ratings", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_RATINGS] = ratingAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "genres", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_GENRES] = genreAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "levels", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_LEVELS] = levelAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "sortopt", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_SORT_OPT] = sortoptAlloc (fn);
}

void
bdjvarsdfCleanup (void)
{
  dncspeedsFree (bdjvarsdf [BDJVDF_DANCE_SPEEDS]);
  dnctypesFree (bdjvarsdf [BDJVDF_DANCE_TYPES]);
  ratingFree (bdjvarsdf [BDJVDF_RATINGS]);
  genreFree (bdjvarsdf [BDJVDF_GENRES]);
  levelFree (bdjvarsdf [BDJVDF_LEVELS]);
  sortoptFree (bdjvarsdf [BDJVDF_SORT_OPT]);
}
