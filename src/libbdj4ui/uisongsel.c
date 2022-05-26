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
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "genre.h"
#include "log.h"
#include "songfilter.h"
#include "uimusicq.h"
#include "uisongsel.h"
#include "ui.h"

static void uisongselSongfilterSetDance (uisongsel_t *uisongsel, ssize_t idx);

uisongsel_t *
uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options,
    songfilterpb_t pbflag, dispselsel_t dispselType)
{
  uisongsel_t   *uisongsel;

  logProcBegin (LOG_PROC, "uisongselInit");

  uisongsel = malloc (sizeof (uisongsel_t));
  assert (uisongsel != NULL);

  uisongsel->tag = tag;
  uisongsel->windowp = NULL;
  uisongsel->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uisongsel->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uisongsel->status = bdjvarsdfGet (BDJVDF_STATUS);
  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->musicdb = musicdb;
  uisongsel->dispselType = dispselType;
  uiutilsUIWidgetInit (&uisongsel->filterDialog);
  uisongsel->options = options;
  uisongsel->idxStart = 0;
  uisongsel->oldIdxStart = 0;
  uisongsel->danceIdx = -1;
  uisongsel->dfilterCount = (double) dbCount (musicdb);
  uisongsel->dfltpbflag = pbflag;
  uisongsel->filterApplied = mstime ();
  uisongsel->playstatusswitch = NULL;
  uiDropDownInit (&uisongsel->dancesel);
  uiDropDownInit (&uisongsel->sortbysel);
  uisongsel->searchentry = uiEntryInit (30, 100);
  uiDropDownInit (&uisongsel->filtergenresel);
  uiDropDownInit (&uisongsel->filterdancesel);
  uisongsel->uirating = NULL;
  uisongsel->uilevel = NULL;
  uisongsel->filterstatussel = uiSpinboxTextInit ();
  uisongsel->filterfavoritesel = uiSpinboxTextInit ();
  uisongsel->songfilter = NULL;
  uisongsel->sortopt = sortoptAlloc ();

  uisongselUIInit (uisongsel);

  logProcEnd (LOG_PROC, "uisongselInit", "");
  return uisongsel;
}

inline void
uisongselInitializeSongFilter (uisongsel_t *uisongsel,
    songfilter_t *songfilter)
{
  uisongsel->songfilter = songfilter;
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
    uiDropDownFree (&uisongsel->dancesel);
    uiDropDownFree (&uisongsel->sortbysel);
    uiEntryFree (uisongsel->searchentry);
    uiDropDownFree (&uisongsel->filterdancesel);
    uiDropDownFree (&uisongsel->filtergenresel);
    uiratingFree (uisongsel->uirating);
    uilevelFree (uisongsel->uilevel);
    uiSpinboxTextFree (uisongsel->filterstatussel);
    uiSpinboxTextFree (uisongsel->filterfavoritesel);
    sortoptFree (uisongsel->sortopt);
    uisongselUIFree (uisongsel);
    uiDialogDestroy (&uisongsel->filterDialog);
    uiutilsUIWidgetInit (&uisongsel->filterDialog);
    uiSwitchFree (uisongsel->playstatusswitch);
    free (uisongsel);
  }

  logProcEnd (LOG_PROC, "uisongselFree", "");
}

void
uisongselMainLoop (uisongsel_t *uisongsel)
{
  if (songfilterIsChanged (uisongsel->songfilter, uisongsel->filterApplied)) {
    uisongselApplySongFilter (uisongsel);
  }
  return;
}

int
uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uisongsel_t   *uisongsel = udata;
  bool          disp = false;

  /* if a message is added that has arguments, then the args var */
  /* needs to be strdup'd (see uimusicq.c) */

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_DATABASE_UPDATE: {
          /* re-filter the display */
          uisongselApplySongFilter (uisongsel);
          disp = true;
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

  if (disp) {
    logMsg (LOG_DBG, LOG_MSGS, "uisongsel (%d): got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        uisongsel->dispselType, routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  return 0;
}

void
uisongselFilterDanceProcess (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselFilterDanceProcess");

  uisongselSongfilterSetDance (uisongsel, idx);
  uisongselApplySongFilter (uisongsel);
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
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_DANCE_IDX, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_DANCE);
    songfilterClear (uisongsel->songfilter, SONG_FILTER_DANCE_IDX);
  }
  logProcEnd (LOG_PROC, "uisongselSongfilterSetDance", "");
}

void
uisongselDanceSelect (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselDanceSelect");
  uisongselSongfilterSetDance (uisongsel, idx);
  logProcEnd (LOG_PROC, "uisongselDanceSelect", "");
}

void
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  ssize_t insloc;
  char    tbuff [MAXPATHLEN];

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    insloc = 99;
  } else {
    insloc = bdjoptGetNum (OPT_P_INSERT_LOCATION);
  }
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

/* song filter */

void
uisongselApplySongFilter (uisongsel_t *uisongsel)
{
  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;
  uisongsel->filterApplied = mstime ();
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
  uiDropDownSelectionSetStr (&uisongsel->sortbysel, sortby);

  idx = uisongsel->danceIdx;
  uiDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);

  uiDropDownSelectionSetNum (&uisongsel->filtergenresel, -1);
  uiEntrySetValue (uisongsel->searchentry, "");
  uiratingSetValue (uisongsel->uirating, -1);
  uilevelSetValue (uisongsel->uilevel, -1);
  uiSpinboxTextSetValue (uisongsel->filterstatussel, -1);
  uiSpinboxTextSetValue (uisongsel->filterfavoritesel, 0);
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
  uiDropDownSetList (&uisongsel->sortbysel, sortoptlist, NULL);
  logProcEnd (LOG_PROC, "uisongselCreateSortByList", "");
}

void
uisongselGenreSelect (uisongsel_t *uisongsel, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisongselGenreSelect");
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_GENRE)) {
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
  uiDropDownSetNumList (&uisongsel->filtergenresel, genrelist,
      /* CONTEXT: a filter: all genres are displayed in the song selection */
      _("All Genres"));
  logProcEnd (LOG_PROC, "uisongselCreateGenreList", "");
}

