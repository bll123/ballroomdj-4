/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "asconf.h"
#include "audioadjust.h"
#include "autosel.h"
#include "bdj4.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "dance.h"
#include "dance.h"
#include "datafile.h"
#include "dispsel.h"
#include "dnctypes.h"
#include "genre.h"
#include "level.h"
#include "pathbld.h"
#include "rating.h"
#include "songfav.h"
#include "status.h"

static const char *dfloaddbg [] = {
  [BDJVDF_AUTO_SEL] = "autosel",
  [BDJVDF_DANCES] = "dances",
  [BDJVDF_DANCE_TYPES] = "dancetypes",
  [BDJVDF_GENRES] = "genres",
  [BDJVDF_LEVELS] = "levels",
  [BDJVDF_RATINGS] = "ratings",
  [BDJVDF_STATUS] = "status",
  [BDJVDF_FAVORITES] = "favorites",
  [BDJVDF_AUDIO_ADJUST] = "audioadjust",
  [BDJVDF_AUDIOSRC_CONF] = "audiosrc",
};

static_assert (sizeof (dfloaddbg) / sizeof (const char *) == BDJVDF_MAX,
    "missing bdjvdf entry");

int
bdjvarsdfloadInit (void)
{
  int   rc;

  /* dance types must be loaded before dance */
  bdjvarsdfSet (BDJVDF_DANCE_TYPES, dnctypesAlloc ());

  /* the database load depends on dances */
  /* playlist loads depend on dances */
  /* sequence loads depend on dances */
  bdjvarsdfSet (BDJVDF_DANCES, danceAlloc (NULL));

  /* the database load depends on ratings, levels and status */
  /* playlist loads depend on ratings, levels and status */
  bdjvarsdfSet (BDJVDF_RATINGS, ratingAlloc ());
  bdjvarsdfSet (BDJVDF_LEVELS, levelAlloc ());
  bdjvarsdfSet (BDJVDF_STATUS, statusAlloc ());

  /* the database load depends on genres and favorites */
  bdjvarsdfSet (BDJVDF_GENRES, genreAlloc ());
  bdjvarsdfSet (BDJVDF_FAVORITES, songFavoriteAlloc ());

  /* auto selection numbers are used by dancesel.c and autosel.c */
  bdjvarsdfSet (BDJVDF_AUTO_SEL, autoselAlloc ());

  /* audio adjust is used by audioadjust.c */
  bdjvarsdfSet (BDJVDF_AUDIO_ADJUST, aaAlloc ());

  /* audio-src-conf is used by configui, */
  /*   audiosrc, uiimppl, bdj4server, bdj4starter */
  bdjvarsdfSet (BDJVDF_AUDIOSRC_CONF, asconfAlloc ());

  rc = 0;
  for (int i = BDJVDF_AUTO_SEL; i < BDJVDF_MAX; ++i) {
    if (bdjvarsdfGet (i) == NULL) {
      /* a missing audio adjust file does not constitute a failure */
      if (i == BDJVDF_AUDIO_ADJUST) {
        continue;
      }
      fprintf (stderr, "Unable to load datafile %d %s\n", i, dfloaddbg [i]);
      rc = -1;
      break;
    }
  }

  return rc;
}

void
bdjvarsdfloadCleanup (void)
{
  asconfFree (bdjvarsdfGet (BDJVDF_AUDIOSRC_CONF));
  aaFree (bdjvarsdfGet (BDJVDF_AUDIO_ADJUST));
  autoselFree (bdjvarsdfGet (BDJVDF_AUTO_SEL));
  songFavoriteFree (bdjvarsdfGet (BDJVDF_FAVORITES));
  genreFree (bdjvarsdfGet (BDJVDF_GENRES));
  statusFree (bdjvarsdfGet (BDJVDF_STATUS));
  levelFree (bdjvarsdfGet (BDJVDF_LEVELS));
  ratingFree (bdjvarsdfGet (BDJVDF_RATINGS));
  danceFree (bdjvarsdfGet (BDJVDF_DANCES));
  dnctypesFree (bdjvarsdfGet (BDJVDF_DANCE_TYPES));
}
