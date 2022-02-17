#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
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
#include "rating.h"
#include "sortopt.h"
#include "status.h"

void
bdjvarsdfloadInit (void)
{
  char      fn [MAXPATHLEN];

    /* dance types must be loaded before dance */
  pathbldMakePath (fn, sizeof (fn), "", "dancetypes", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_DANCE_TYPES, dnctypesAlloc (fn));

    /* the database load depends on dances */
    /* playlist loads depend on dances */
    /* sequence loads depend on dances */
  pathbldMakePath (fn, sizeof (fn), "", "dances", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_DANCES, danceAlloc (fn));

    /* the database load depends on ratings, levels, genres and status */
    /* playlist loads depend on ratings and levels */
  pathbldMakePath (fn, sizeof (fn), "", "ratings", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_RATINGS, ratingAlloc (fn));
  pathbldMakePath (fn, sizeof (fn), "", "genres", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_GENRES, genreAlloc (fn));
  pathbldMakePath (fn, sizeof (fn), "", "levels", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_LEVELS, levelAlloc (fn));

    /* status is an optional configuration file   */
    /* the database load depends on it if present */
  pathbldMakePath (fn, sizeof (fn), "", "status", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_STATUS, statusAlloc (fn));

  pathbldMakePath (fn, sizeof (fn), "", "sortopt", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_SORT_OPT, sortoptAlloc (fn));
  pathbldMakePath (fn, sizeof (fn), "", "autoselection", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_AUTO_SEL, autoselAlloc (fn));
}

void
bdjvarsdfloadCleanup (void)
{
  autoselFree (bdjvarsdfGet (BDJVDF_AUTO_SEL));
  sortoptFree (bdjvarsdfGet (BDJVDF_SORT_OPT));
  statusFree (bdjvarsdfGet (BDJVDF_STATUS));
  levelFree (bdjvarsdfGet (BDJVDF_LEVELS));
  genreFree (bdjvarsdfGet (BDJVDF_GENRES));
  ratingFree (bdjvarsdfGet (BDJVDF_RATINGS));
  danceFree (bdjvarsdfGet (BDJVDF_DANCES));
  dnctypesFree (bdjvarsdfGet (BDJVDF_DANCE_TYPES));
}
