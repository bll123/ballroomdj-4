#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4playerui.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "uisongsel.h"
#include "uiutils.h"

uisongsel_t *
uisongselInit (progstate_t *progstate, conn_t *conn, dispsel_t *dispsel,
    nlist_t *options, songfilterpb_t filterFlags)
{
  uisongsel_t    *uisongsel;

  logProcBegin (LOG_PROC, "uisongselInit");

  uisongsel = malloc (sizeof (uisongsel_t));
  assert (uisongsel != NULL);

  uisongsel->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uisongsel->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uisongsel->status = bdjvarsdfGet (BDJVDF_STATUS);
  uisongsel->progstate = progstate;
  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->maxRows = 0;
  uisongsel->idxStart = 0;
  uisongsel->createRowProcessFlag = false;
  uisongsel->createRowFlag = false;
  uisongsel->dfilterCount = (double) dbCount ();
  uisongsel->lastTreeSize = 0;
  uisongsel->lastRowHeight = 0.0;
  uisongsel->danceIdx = -1;
  uisongsel->options = options;
  uisongsel->favColumn = NULL;
  uiutilsDropDownInit (&uisongsel->dancesel);
  uiutilsDropDownInit (&uisongsel->sortbysel);
  uiutilsEntryInit (&uisongsel->searchentry, 30, 100);
  uiutilsDropDownInit (&uisongsel->filtergenresel);
  uiutilsDropDownInit (&uisongsel->filterdancesel);
  uiutilsSpinboxTextInit (&uisongsel->filterratingsel);
  uiutilsSpinboxTextInit (&uisongsel->filterlevelsel);
  uiutilsSpinboxTextInit (&uisongsel->filterstatussel);
  uiutilsSpinboxTextInit (&uisongsel->filterfavoritesel);
  uisongsel->songfilter = songfilterAlloc (filterFlags);
  songfilterSetSort (uisongsel->songfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
  uisongsel->sortopt = sortoptAlloc ();

  // ### FIX load last option/etc settings from datafile.

  uisongsel->filterDialog = NULL;

  logProcEnd (LOG_PROC, "uisongselInit", "");
  return uisongsel;
}

void
uisongselFree (uisongsel_t *uisongsel)
{

  logProcBegin (LOG_PROC, "uisongselFree");

  if (uisongsel != NULL) {
    if (uisongsel->songfilter != NULL) {
      songfilterFree (uisongsel->songfilter);
    }
    uiutilsDropDownFree (&uisongsel->dancesel);
    uiutilsDropDownFree (&uisongsel->sortbysel);
    uiutilsEntryFree (&uisongsel->searchentry);
    uiutilsDropDownFree (&uisongsel->filterdancesel);
    uiutilsDropDownFree (&uisongsel->filtergenresel);
    uiutilsSpinboxTextFree (&uisongsel->filterratingsel);
    uiutilsSpinboxTextFree (&uisongsel->filterlevelsel);
    uiutilsSpinboxTextFree (&uisongsel->filterstatussel);
    uiutilsSpinboxTextFree (&uisongsel->filterfavoritesel);
    sortoptFree (uisongsel->sortopt);
    free (uisongsel);
  }
  logProcEnd (LOG_PROC, "uisongselFree", "");
}

void
uisongselMainLoop (uisongsel_t *uisongsel)
{
  return;
}

