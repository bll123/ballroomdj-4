#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "autosel.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "dance.h"
#include "dance.h"
#include "datafile.h"
#include "pathbld.h"
#include "dnctypes.h"
#include "genre.h"
#include "level.h"
#include "portability.h"
#include "rating.h"
#include "sortopt.h"
#include "status.h"

void
bdjvarsdfloadInit (void)
{
  char      fn [MAXPATHLEN];

    /* dance types must be loaded before dance */
  pathbldMakePath (fn, sizeof (fn), "", "dancetypes", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_DANCE_TYPES] = dnctypesAlloc (fn);

    /* the database load depends on dances */
    /* playlist loads depend on dances */
    /* sequence loads depend on dances */
  pathbldMakePath (fn, sizeof (fn), "", "dances", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_DANCES] = danceAlloc (fn);

    /* the database load depends on ratings, levels, genres and status */
    /* playlist loads depend on ratings and levels */
  pathbldMakePath (fn, sizeof (fn), "", "ratings", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_RATINGS] = ratingAlloc (fn);
  pathbldMakePath (fn, sizeof (fn), "", "genres", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_GENRES] = genreAlloc (fn);
  pathbldMakePath (fn, sizeof (fn), "", "levels", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_LEVELS] = levelAlloc (fn);

    /* status is an optional configuration file   */
    /* the database load depends on it if present */
  pathbldMakePath (fn, sizeof (fn), "", "status", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_STATUS] = statusAlloc (fn);

  pathbldMakePath (fn, sizeof (fn), "", "sortopt", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_SORT_OPT] = sortoptAlloc (fn);
  pathbldMakePath (fn, sizeof (fn), "", "autoselection", ".txt", PATHBLD_MP_NONE);
  bdjvarsdf [BDJVDF_AUTO_SEL] = autoselAlloc (fn);
}

void
bdjvarsdfloadCleanup (void)
{
  autoselFree (bdjvarsdf [BDJVDF_AUTO_SEL]);
  sortoptFree (bdjvarsdf [BDJVDF_SORT_OPT]);
  statusFree (bdjvarsdf [BDJVDF_STATUS]);
  levelFree (bdjvarsdf [BDJVDF_LEVELS]);
  genreFree (bdjvarsdf [BDJVDF_GENRES]);
  ratingFree (bdjvarsdf [BDJVDF_RATINGS]);
  danceFree (bdjvarsdf [BDJVDF_DANCES]);
  dnctypesFree (bdjvarsdf [BDJVDF_DANCE_TYPES]);
}
