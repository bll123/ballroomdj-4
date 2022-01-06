#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bdjvarsdfload.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "dance.h"
#include "autosel.h"
#include "dnctypes.h"
#include "genre.h"
#include "level.h"
#include "rating.h"
#include "sortopt.h"
#include "portability.h"
#include "datautil.h"
#include "dance.h"

void
bdjvarsdfloadInit (void)
{
  char      fn [MAXPATHLEN];

    /* dance types must be loaded before dance */
  datautilMakePath (fn, sizeof (fn), "", "dancetypes", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_DANCE_TYPES] = dnctypesAlloc (fn);

    /* the database load depends on dances */
    /* playlist loads depend on dances */
  datautilMakePath (fn, sizeof (fn), "", "dances", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_DANCES] = danceAlloc (fn);

    /* the database load depends on ratings, levels and genres */
    /* playlist loads depend on ratings and levels */
  datautilMakePath (fn, sizeof (fn), "", "ratings", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_RATINGS] = ratingAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "genres", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_GENRES] = genreAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "levels", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_LEVELS] = levelAlloc (fn);

  datautilMakePath (fn, sizeof (fn), "", "sortopt", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_SORT_OPT] = sortoptAlloc (fn);
  datautilMakePath (fn, sizeof (fn), "", "autoselection", ".txt", DATAUTIL_MP_NONE);
  bdjvarsdf [BDJVDF_AUTO_SEL] = autoselAlloc (fn);
}

void
bdjvarsdfloadCleanup (void)
{
  autoselFree (bdjvarsdf [BDJVDF_AUTO_SEL]);
  sortoptFree (bdjvarsdf [BDJVDF_SORT_OPT]);
  levelFree (bdjvarsdf [BDJVDF_LEVELS]);
  genreFree (bdjvarsdf [BDJVDF_GENRES]);
  ratingFree (bdjvarsdf [BDJVDF_RATINGS]);
  danceFree (bdjvarsdf [BDJVDF_DANCES]);
  dnctypesFree (bdjvarsdf [BDJVDF_DANCE_TYPES]);
}
