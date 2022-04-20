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

#include "bdj4.h"
#include "bdj4playerui.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "log.h"
#include "uimusicq.h"
#include "uisongsel.h"
#include "uiutils.h"

static void uisongselSongfilterSetDance (uisongsel_t *uisongsel, ssize_t idx);

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

void
uisongselFilterDanceProcess (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselFilterDanceProcess");

  uisongselSongfilterSetDance (uisongsel, idx);
  uiutilsDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);
  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  uisongsel->idxStart = 0;
  uisongselClearData (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: filter by dance");
  uisongselPopulateData (uisongsel);
  logProcEnd (LOG_PROC, "uisongselFilterDanceProcess", "");
}

/* internal routines */

static void
uisongselSongfilterSetDance (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselSongfilterSetDance");

  uisongsel->danceIdx = idx;
  if (idx >= 0) {
    ilist_t   *danceList;

    danceList = ilistAlloc ("songsel-filter-dance", LIST_ORDERED);
    /* any value will do; only interested in the dance index at this point */
    ilistSetNum (danceList, idx, 0, 0);
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_DANCE, danceList);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_DANCE);
  }
  logProcEnd (LOG_PROC, "uisongselSongfilterSetDance", "");
}

void
uisongselDanceSelect (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselDanceSelect");
  uisongsel->danceIdx = idx;
  uisongselSongfilterSetDance (uisongsel, idx);
  logProcEnd (LOG_PROC, "uisongselDanceSelect", "");
}

void
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  ssize_t insloc;
  char    tbuff [MAXPATHLEN];

  insloc = bdjoptGetNum (OPT_P_INSERT_LOCATION);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zd%c%d", MUSICQ_CURRENT,
      MSG_ARGS_RS, insloc, MSG_ARGS_RS, dbidx);
  connSendMessage (uisongsel->conn, ROUTE_MAIN,
      MSG_MUSICQ_INSERT, tbuff);
}

void
uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  song_t    *song;

  song = dbGetByIdx ((ssize_t) dbidx);
  songChangeFavorite (song);
// ### TODO song data must be saved to the database.
  logMsg (LOG_DBG, LOG_SONGSEL, "favorite changed");
  uisongselPopulateData (uisongsel);
}
