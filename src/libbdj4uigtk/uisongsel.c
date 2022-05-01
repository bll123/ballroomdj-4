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
#include "bdj4intl.h"
#include "bdj4playerui.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "genre.h"
#include "log.h"
#include "pathbld.h"
#include "songfilter.h"
#include "uimusicq.h"
#include "uisongsel.h"
#include "uiutils.h"

static void uisongselSongfilterSetDance (uisongsel_t *uisongsel, ssize_t idx);

uisongsel_t *
uisongselInit (conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options,
    songfilterpb_t pbflag, dispselsel_t dispselType)
{
  uisongsel_t   *uisongsel;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uisongselInit");

  uisongsel = malloc (sizeof (uisongsel_t));
  assert (uisongsel != NULL);

  uisongsel->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uisongsel->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uisongsel->status = bdjvarsdfGet (BDJVDF_STATUS);
  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->musicdb = musicdb;
  uisongsel->dispselType = dispselType;
  uisongsel->filterDisplaySel = NULL;
  uisongsel->options = options;
  uisongsel->idxStart = 0;
  uisongsel->danceIdx = -1;
  uisongsel->dfilterCount = (double) dbCount (musicdb);
  uisongsel->dfltpbflag = pbflag;
  uiutilsDropDownInit (&uisongsel->dancesel);
  uiutilsDropDownInit (&uisongsel->sortbysel);
  uiutilsEntryInit (&uisongsel->searchentry, 30, 100);
  uiutilsDropDownInit (&uisongsel->filtergenresel);
  uiutilsDropDownInit (&uisongsel->filterdancesel);
  uiutilsSpinboxTextInit (&uisongsel->filterratingsel);
  uiutilsSpinboxTextInit (&uisongsel->filterlevelsel);
  uiutilsSpinboxTextInit (&uisongsel->filterstatussel);
  uiutilsSpinboxTextInit (&uisongsel->filterfavoritesel);
  uisongsel->songfilter = songfilterAlloc ();
  songfilterSetSort (uisongsel->songfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
  uisongsel->sortopt = sortoptAlloc ();

  pathbldMakePath (tbuff, sizeof (tbuff),
      "ds-songfilter", ".txt", PATHBLD_MP_USEIDX);
  uisongsel->filterDisplayDf = datafileAllocParse ("uisongsel-filter",
      DFTYPE_KEY_VAL, tbuff, filterdisplaydfkeys, FILTER_DISP_MAX, DATAFILE_NO_LOOKUP);
  uisongsel->filterDisplaySel = datafileGetList (uisongsel->filterDisplayDf);

  uisongselUIInit (uisongsel);

  logProcEnd (LOG_PROC, "uisongselInit", "");
  return uisongsel;
}

void
uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb)
{
  uisongsel->musicdb = musicdb;
}

void
uisongselFree (uisongsel_t *uisongsel)
{
  logProcBegin (LOG_PROC, "uisongselFree");

  if (uisongsel != NULL) {
    if (uisongsel->songfilter != NULL) {
      songfilterFree (uisongsel->songfilter);
    }
    if (uisongsel->filterDisplayDf != NULL) {
      datafileFree (uisongsel->filterDisplayDf);
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
    uisongselUIFree (uisongsel);
    free (uisongsel);
  }

  logProcEnd (LOG_PROC, "uisongselFree", "");
}

void
uisongselMainLoop (uisongsel_t *uisongsel)
{
  return;
}

int
uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uisongsel_t    *uisongsel = udata;


  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_DATABASE_UPDATE: {
          /* re-filter the display */
          uisongselFilterDanceProcess (uisongsel, uisongsel->danceIdx);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  return 0;
}

void
uisongselFilterDanceProcess (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselFilterDanceProcess");

  uisongselSongfilterSetDance (uisongsel, idx);
  uiutilsDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);
  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
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
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  ssize_t insloc;
  char    tbuff [MAXPATHLEN];

  insloc = bdjoptGetNum (OPT_P_INSERT_LOCATION);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zd%c%d", mqidx,
      MSG_ARGS_RS, insloc, MSG_ARGS_RS, dbidx);
  connSendMessage (uisongsel->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
}

void
uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  song_t    *song;

  song = dbGetByIdx (uisongsel->musicdb, (ssize_t) dbidx);
  songChangeFavorite (song);
// ### TODO song data must be saved to the database.
  logMsg (LOG_DBG, LOG_SONGSEL, "favorite changed");
  uisongselPopulateData (uisongsel);
}

void
uisongselApplySongFilter (uisongsel_t *uisongsel)
{
  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;
  uisongselClearData (uisongsel);
  uisongselPopulateData (uisongsel);
}

void
uisongselInitFilterDisplay (uisongsel_t *uisongsel)
{
  nlistidx_t  idx;
  char        *sortby;

  logProcBegin (LOG_PROC, "uisongselInitFilterDisplay");

  /* this is run when the filter dialog is first started, */
  /* and after a reset */
  /* all items need to be set, as after a reset, they need to be updated */
  /* sort-by and dance are important, the others can be reset */

  sortby = songfilterGetSort (uisongsel->songfilter);
  uiutilsDropDownSelectionSetStr (&uisongsel->sortbysel, sortby);

  idx = uisongsel->danceIdx;
  uiutilsDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);

  uiutilsDropDownSelectionSetNum (&uisongsel->filtergenresel, -1);
  uiutilsEntrySetValue (&uisongsel->searchentry, "");
  uiutilsSpinboxTextSetValue (&uisongsel->filterratingsel, -1);
  uiutilsSpinboxTextSetValue (&uisongsel->filterlevelsel, -1);
  uiutilsSpinboxTextSetValue (&uisongsel->filterstatussel, -1);
  uiutilsSpinboxTextSetValue (&uisongsel->filterfavoritesel, 0);
  logProcEnd (LOG_PROC, "uisongselInitFilterDisplay", "");
}

char *
uisongselRatingGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselRatingGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselRatingGet", "all");
    /* CONTEXT: a filter: all dance ratings are displayed in the song selection */
    return _("All Ratings");
  }
  logProcEnd (LOG_PROC, "uisongselRatingGet", "");
  return ratingGetRating (uisongsel->ratings, idx);
}

char *
uisongselLevelGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselLevelGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselLevelGet", "all");
    /* CONTEXT: a filter: all dance levels are displayed in the song selection */
    return _("All Levels");
  }
  logProcEnd (LOG_PROC, "uisongselLevelGet", "");
  return levelGetLevel (uisongsel->levels, idx);
}

char *
uisongselStatusGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselStatusGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselStatusGet", "any");
    /* CONTEXT: a filter: all statuses are displayed in the song selection */
    return _("Any Status");
  }
  logProcEnd (LOG_PROC, "uisongselStatusGet", "");
  return statusGetStatus (uisongsel->status, idx);
}

char *
uisongselFavoriteGet (void *udata, int idx)
{
  uisongsel_t         *uisongsel = udata;
  songfavoriteinfo_t  *favorite;

  logProcBegin (LOG_PROC, "uisongselFavoriteGet");

  favorite = songGetFavorite (idx);
  uisongselSetFavoriteForeground (uisongsel, favorite->color);
  logProcEnd (LOG_PROC, "uisongselFavoriteGet", "");
  return favorite->dispStr;
}


void
uisongselSortBySelect (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselSortBySelect");
  if (idx >= 0) {
    songfilterSetSort (uisongsel->songfilter,
        uisongsel->sortbysel.strSelection);
    nlistSetStr (uisongsel->options, SONGSEL_SORT_BY,
        uisongsel->sortbysel.strSelection);
  }
  logProcEnd (LOG_PROC, "uisongselSortBySelect", "");
}

void
uisongselCreateSortByList (uisongsel_t *uisongsel)
{
  slist_t           *sortoptlist;

  logProcBegin (LOG_PROC, "uisongselCreateSortByList");

  sortoptlist = sortoptGetList (uisongsel->sortopt);
  uiutilsDropDownSetList (&uisongsel->sortbysel, sortoptlist, NULL);
  logProcEnd (LOG_PROC, "uisongselCreateSortByList", "");
}

void
uisongselGenreSelect (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselGenreSelect");
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_GENRE)) {
    if (idx >= 0) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_GENRE, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_GENRE);
    }
  }
  logProcEnd (LOG_PROC, "uisongselGenreSelect", "");
}

void
uisongselCreateGenreList (uisongsel_t *uisongsel)
{
  slist_t   *genrelist;
  genre_t   *genre;

  logProcBegin (LOG_PROC, "uisongselCreateGenreList");

  genre = bdjvarsdfGet (BDJVDF_GENRES);
  genrelist = genreGetList (genre);
  uiutilsDropDownSetNumList (&uisongsel->filtergenresel, genrelist,
      /* CONTEXT: a filter: all genres are displayed in the song selection */
      _("All Genres"));
  logProcEnd (LOG_PROC, "uisongselCreateGenreList", "");
}
