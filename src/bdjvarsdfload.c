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
#include "status.h"

int
bdjvarsdfloadInit (void)
{
  char  fn [MAXPATHLEN];
  int   rc;

  /* dance types must be loaded before dance */
  pathbldMakePath (fn, sizeof (fn), "", "dancetypes", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_DANCE_TYPES, dnctypesAlloc (fn));

  /* the database load depends on dances */
  /* playlist loads depend on dances */
  /* sequence loads depend on dances */
  bdjvarsdfSet (BDJVDF_DANCES, danceAlloc ());

  /* the database load depends on ratings, levels and status*/
  /* playlist loads depend on ratings, levels and status */
  bdjvarsdfSet (BDJVDF_RATINGS, ratingAlloc ());
  bdjvarsdfSet (BDJVDF_LEVELS, levelAlloc ());
  bdjvarsdfSet (BDJVDF_STATUS, statusAlloc ());

  /* the database load depends on genres */
  bdjvarsdfSet (BDJVDF_GENRES, genreAlloc ());

  pathbldMakePath (fn, sizeof (fn), "", "autoselection", ".txt", PATHBLD_MP_NONE);
  bdjvarsdfSet (BDJVDF_AUTO_SEL, autoselAlloc (fn));

  rc = 0;
  for (int i = BDJVDF_AUTO_SEL; i < BDJVDF_MAX; ++i) {
    if (bdjvarsdfGet (i) == NULL) {
      fprintf (stderr, "Unable to load datafile %d\n", i);
      rc = -1;
      break;
    }
  }

  return rc;
}

void
bdjvarsdfloadCleanup (void)
{
  autoselFree (bdjvarsdfGet (BDJVDF_AUTO_SEL));
  statusFree (bdjvarsdfGet (BDJVDF_STATUS));
  levelFree (bdjvarsdfGet (BDJVDF_LEVELS));
  genreFree (bdjvarsdfGet (BDJVDF_GENRES));
  ratingFree (bdjvarsdfGet (BDJVDF_RATINGS));
  danceFree (bdjvarsdfGet (BDJVDF_DANCES));
  dnctypesFree (bdjvarsdfGet (BDJVDF_DANCE_TYPES));
}
