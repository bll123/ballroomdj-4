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
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "playlist.h"
#include "rating.h"
#include "songfilter.h"
#include "songlist.h"
#include "sortopt.h"
#include "status.h"
#include "ui.h"
#include "uifavorite.h"
#include "uilevel.h"
#include "uirating.h"
#include "uisongfilter.h"
#include "uistatus.h"
#include "uiutils.h"

enum {
  UISF_LABEL_SORTBY,
  UISF_LABEL_SEARCH,
  UISF_LABEL_GENRE,
  UISF_LABEL_DANCE,
  UISF_LABEL_DANCE_RATING,
  UISF_LABEL_DANCE_LEVEL,
  UISF_LABEL_STATUS,
  UISF_LABEL_FAVORITE,
  UISF_LABEL_PLAY_STATUS,
  UISF_LABEL_MAX,
};

typedef struct uisongfilter {
  rating_t          *ratings;
  level_t           *levels;
  status_t          *status;
  sortopt_t         *sortopt;
  UIWidget          *parentwin;
  nlist_t           *options;
  UICallback        *applycb;
  UICallback        *danceselcb;
  songfilter_t      *songfilter;
  UIWidget          filterDialog;
  UICallback        filtercb;
  UIWidget          playlistdisp;
  uidropdown_t      *playlistfilter;
  uidropdown_t      *sortbyfilter;
  uidropdown_t      *dancefilter;
  uidropdown_t      *genrefilter;
  uientry_t         *searchentry;
  uirating_t        *uirating;
  uilevel_t         *uilevel;
  uistatus_t        *uistatus;
  uifavorite_t      *uifavorite;
  uiswitch_t        *playstatusswitch;
  UIWidget          labels [UISF_LABEL_MAX];
  songfilterpb_t    dfltpbflag;
  int               danceIdx;
  bool              showplaylist : 1;
} uisongfilter_t;

/* song filter handling */
static void uisfDisableWidgets (uisongfilter_t *uisf);
static void uisfEnableWidgets (uisongfilter_t *uisf);
static void uisfCreateDialog (uisongfilter_t *uisf);
static bool uisfResponseHandler (void *udata, long responseid);
static void uisfUpdate (uisongfilter_t *uisf);
static void uisfPlaylistSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisfSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisfGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisfDanceSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisfInitDisplay (uisongfilter_t *uisf);
static void uisfPlaylistSelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfCreatePlaylistList (uisongfilter_t *uisf);
static void uisfSortBySelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfCreateSortByList (uisongfilter_t *uisf);
static void uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfCreateGenreList (uisongfilter_t *uisf);
static void uisfUpdateFilterDialogDisplay (uisongfilter_t *uisf);

uisongfilter_t *
uisfInit (UIWidget *windowp, nlist_t *options, songfilterpb_t pbflag)
{
  uisongfilter_t *uisf;

  uisf = malloc (sizeof (uisongfilter_t));

  uisf->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uisf->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uisf->status = bdjvarsdfGet (BDJVDF_STATUS);
  uisf->sortopt = sortoptAlloc ();
  uisf->applycb = NULL;
  uisf->danceselcb = NULL;
  uisf->parentwin = windowp;
  uisf->options = options;
  uisf->songfilter = songfilterAlloc ();
  songfilterSetSort (uisf->songfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));
  uiutilsUIWidgetInit (&uisf->filterDialog);
  uisf->dfltpbflag = pbflag;
  uisf->playlistfilter = uiDropDownInit ();
  uisf->sortbyfilter = uiDropDownInit ();
  uisf->genrefilter = uiDropDownInit ();
  uisf->dancefilter = uiDropDownInit ();
  uiutilsUIWidgetInit (&uisf->playlistdisp);
  uisf->searchentry = uiEntryInit (30, 100);
  uisf->uirating = NULL;
  uisf->uilevel = NULL;
  uisf->uistatus = NULL;
  uisf->uifavorite = NULL;
  uisf->playstatusswitch = NULL;
  uisf->danceIdx = -1;
  uisf->showplaylist = false;
  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiutilsUIWidgetInit (&uisf->labels [i]);
  }

  return uisf;
}

void
uisfFree (uisongfilter_t *uisf)
{
  if (uisf != NULL) {
    uiDialogDestroy (&uisf->filterDialog);
    uiDropDownFree (uisf->playlistfilter);
    uiDropDownFree (uisf->sortbyfilter);
    uiEntryFree (uisf->searchentry);
    uiDropDownFree (uisf->dancefilter);
    uiDropDownFree (uisf->genrefilter);
    uiratingFree (uisf->uirating);
    uilevelFree (uisf->uilevel);
    uistatusFree (uisf->uistatus);
    uifavoriteFree (uisf->uifavorite);
    uiSwitchFree (uisf->playstatusswitch);
    sortoptFree (uisf->sortopt);
    songfilterFree (uisf->songfilter);
    free (uisf);
  }
}

void
uisfSetApplyCallback (uisongfilter_t *uisf, UICallback *applycb)
{
  if (uisf == NULL) {
    return;
  }
  uisf->applycb = applycb;
}

void
uisfSetDanceSelectCallback (uisongfilter_t *uisf, UICallback *danceselcb)
{
  if (uisf == NULL) {
    return;
  }
  uisf->danceselcb = danceselcb;
}

void
uisfShowPlaylistDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  uisf->showplaylist = true;
  uisfUpdateFilterDialogDisplay (uisf);
}

void
uisfHidePlaylistDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  uisf->showplaylist = false;
  uisfUpdateFilterDialogDisplay (uisf);
}

bool
uisfPlaylistInUse (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return false;
  }

  return songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST);
}

bool
uisfDialog (void *udata)
{
  uisongfilter_t *uisf = udata;
  int         x, y;

  logProcBegin (LOG_PROC, "uisfDialog");

  uisfCreateDialog (uisf);
  uisfInitDisplay (uisf);
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    uiSwitchSetValue (uisf->playstatusswitch, uisf->dfltpbflag);
  }
  uiWidgetShowAll (&uisf->filterDialog);

  uisfUpdateFilterDialogDisplay (uisf);

  x = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_Y);
  uiWindowMove (&uisf->filterDialog, x, y);
  logProcEnd (LOG_PROC, "uisfDialog", "");
  return UICB_CONT;
}

void
uisfSetPlaylist (uisongfilter_t *uisf, char *slname)
{
  if (uisf == NULL) {
    return;
  }

  uiDropDownSelectionSetStr (uisf->playlistfilter, slname);
  songfilterSetData (uisf->songfilter, SONG_FILTER_PLAYLIST, slname);
}

void
uisfClearPlaylist (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  songfilterOff (uisf->songfilter, SONG_FILTER_PLAYLIST);
}

void
uisfSetDanceIdx (uisongfilter_t *uisf, int danceIdx)
{
  if (uisf == NULL) {
    return;
  }

  uisf->danceIdx = danceIdx;
  uiDropDownSelectionSetNum (uisf->dancefilter, danceIdx);

  if (danceIdx >= 0) {
    ilist_t   *danceList;

    danceList = ilistAlloc ("songfilter-dance", LIST_ORDERED);
    /* any value will do; only interested in the dance index at this point */
    ilistSetNum (danceList, danceIdx, 0, 0);
    songfilterSetData (uisf->songfilter, SONG_FILTER_DANCE, danceList);
    songfilterSetNum (uisf->songfilter, SONG_FILTER_DANCE_IDX, danceIdx);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_DANCE);
    songfilterClear (uisf->songfilter, SONG_FILTER_DANCE_IDX);
  }
}

songfilter_t *
uisfGetSongFilter (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return NULL;
  }
  return uisf->songfilter;
}

/* internal routines */

static void
uisfInitDisplay (uisongfilter_t *uisf)
{
  char        *sortby;

  logProcBegin (LOG_PROC, "uisfInitFilterDisplay");

  /* this is run when the filter dialog is first started, */
  /* and after a reset */
  /* all items need to be set, as after a reset, they need to be updated */
  /* sort-by and dance are important, the others can be reset */

  uiDropDownSelectionSetNum (uisf->playlistfilter, -1);

  sortby = songfilterGetSort (uisf->songfilter);
  uiDropDownSelectionSetStr (uisf->sortbyfilter, sortby);
  uiDropDownSelectionSetNum (uisf->dancefilter, uisf->danceIdx);
  uiDropDownSelectionSetNum (uisf->genrefilter, -1);
  uiEntrySetValue (uisf->searchentry, "");
  uiratingSetValue (uisf->uirating, -1);
  uilevelSetValue (uisf->uilevel, -1);
  uistatusSetValue (uisf->uistatus, -1);
  uifavoriteSetValue (uisf->uifavorite, 0);
  logProcEnd (LOG_PROC, "uisfInitFilterDisplay", "");
}

#if 0
static char *
uisfFavoriteGet (void *udata, int idx)
{
  uisongfilter_t      *uisf = udata;
  songfavoriteinfo_t  *favorite;

  logProcBegin (LOG_PROC, "uisfFavoriteGet");

  favorite = songGetFavorite (idx);
  uisfSetFavoriteForeground (uisf, favorite->color);
  logProcEnd (LOG_PROC, "uisfFavoriteGet", "");
  return favorite->dispStr;
}
#endif

#if 0
static void
uisfSetFavoriteForeground (uisongfilter_t *uisf, char *color)
{
  char                tmp [40];

  if (strcmp (color, "") != 0) {
    uiSpinboxSetColor (uisf->favoritefilter, color);
  } else {
    uiGetForegroundColor (&uisf->filterDialog, tmp, sizeof (tmp));
    uiSpinboxSetColor (uisf->favoritefilter, tmp);
  }
}
#endif

static void
uisfPlaylistSelect (uisongfilter_t *uisf, ssize_t idx)
{
  char  *str;

  logProcBegin (LOG_PROC, "uisfPlaylistSelect");
  if (idx >= 0) {
    str = uiDropDownGetString (uisf->playlistfilter);
    songfilterSetData (uisf->songfilter, SONG_FILTER_PLAYLIST, str);
    uisfDisableWidgets (uisf);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_PLAYLIST);
    uisfEnableWidgets (uisf);
  }
  logProcEnd (LOG_PROC, "uisfPlaylistSelect", "");
}

static void
uisfCreatePlaylistList (uisongfilter_t *uisf)
{
  slist_t           *pllist;

  logProcBegin (LOG_PROC, "uisfCreatePlaylistList");

  /* at this time, only song lists are supported */
  pllist = playlistGetPlaylistList (PL_LIST_SONGLIST);
  /* what text is best to use for 'no selection'? */
  uiDropDownSetList (uisf->playlistfilter, pllist, "");
  slistFree (pllist);
  logProcEnd (LOG_PROC, "uisfCreatePlaylistList", "");
}

static void
uisfSortBySelect (uisongfilter_t *uisf, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisfSortBySelect");
  if (idx >= 0) {
    songfilterSetSort (uisf->songfilter,
        uiDropDownGetString (uisf->sortbyfilter));
    nlistSetStr (uisf->options, SONGSEL_SORT_BY,
        uiDropDownGetString (uisf->sortbyfilter));
  }
  logProcEnd (LOG_PROC, "uisfSortBySelect", "");
}

static void
uisfCreateSortByList (uisongfilter_t *uisf)
{
  slist_t           *sortoptlist;

  logProcBegin (LOG_PROC, "uisfCreateSortByList");

  sortoptlist = sortoptGetList (uisf->sortopt);
  uiDropDownSetList (uisf->sortbyfilter, sortoptlist, NULL);
  logProcEnd (LOG_PROC, "uisfCreateSortByList", "");
}

static void
uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisfGenreSelect");
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_GENRE, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_GENRE);
    }
  }
  logProcEnd (LOG_PROC, "uisfGenreSelect", "");
}

static void
uisfCreateGenreList (uisongfilter_t *uisf)
{
  slist_t   *genrelist;
  genre_t   *genre;

  logProcBegin (LOG_PROC, "uisfCreateGenreList");

  genre = bdjvarsdfGet (BDJVDF_GENRES);
  genrelist = genreGetList (genre);
  uiDropDownSetNumList (uisf->genrefilter, genrelist,
      /* CONTEXT: a filter: all genres are displayed in the song selection */
      _("All Genres"));
  logProcEnd (LOG_PROC, "uisfCreateGenreList", "");
}

static void
uisfCreateDialog (uisongfilter_t *uisf)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uiwidgetp;
  UIWidget      sg;  // labels
  UIWidget      sgA; // playlist, title, search
  UIWidget      sgB; // drop-downs, not including title
  UIWidget      sgC; // spinboxes, not including favorite

  logProcBegin (LOG_PROC, "uisfCreateDialog");

  if (uiutilsUIWidgetSet (&uisf->filterDialog)) {
    return;
  }

  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgA);
  uiCreateSizeGroupHoriz (&sgB);
  uiCreateSizeGroupHoriz (&sgC);

  uiutilsUICallbackLongInit (&uisf->filtercb,
      uisfResponseHandler, uisf);
  uiCreateDialog (&uisf->filterDialog, uisf->parentwin,
      &uisf->filtercb,
      /* CONTEXT: title for the filter dialog */
      _("Filter Songs"),
      /* CONTEXT: filter dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: filter dialog: resets the selections */
      _("Reset"),
      RESPONSE_RESET,
      /* CONTEXT: filter dialog: applies the selections */
      _("Apply"),
      RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiDialogPackInDialog (&uisf->filterDialog, &vbox);

  /* playlist : only available for the music manager */
  /* in this case, the entire hbox will be shown/hidden */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);
  uiutilsUIWidgetCopy (&uisf->playlistdisp, &hbox);

  /* CONTEXT: a filter: select a playlist to work with (music manager) */
  uiCreateColonLabel (&uiwidget, _("Playlist"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiwidgetp = uiComboboxCreate (&uisf->filterDialog, "",
      uisfPlaylistSelectHandler, uisf->playlistfilter, uisf);
  uisfCreatePlaylistList (uisf);
  uiBoxPackStart (&hbox, uiwidgetp);
  /* looks bad if added to the size group */

  /* sort-by : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the method to sort the song selection display */
  uiCreateColonLabel (&uiwidget, _("Sort by"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_SORTBY], &uiwidget);

  uiwidgetp = uiComboboxCreate (&uisf->filterDialog, "",
      uisfSortBySelectHandler, uisf->sortbyfilter, uisf);
  uisfCreateSortByList (uisf);
  uiBoxPackStart (&hbox, uiwidgetp);
  /* looks bad if added to the size group */

  /* search : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: filter the song selection with a search for text */
  uiCreateColonLabel (&uiwidget, _("Search"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_SEARCH], &uiwidget);

  uiEntryCreate (uisf->searchentry);
  uiwidgetp = uiEntryGetUIWidget (uisf->searchentry);
  uiWidgetAlignHorizStart (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgA, uiwidgetp);

  /* genre */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the genre displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Genre"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);
    uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_GENRE], &uiwidget);

    uiwidgetp = uiComboboxCreate (&uisf->filterDialog, "",
        uisfGenreSelectHandler, uisf->genrefilter, uisf);
    uisfCreateGenreList (uisf);
    uiBoxPackStart (&hbox, uiwidgetp);
    /* looks bad if added to the size group */
 }

  /* dance : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the dance displayed in the song selection */
  uiCreateColonLabel (&uiwidget, _("Dance"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_DANCE], &uiwidget);

  uiwidgetp = uiComboboxCreate (&uisf->filterDialog,
      "", uisfDanceSelectHandler,
      uisf->dancefilter, uisf);
  /* CONTEXT: a filter: all dances are selected */
  uiutilsCreateDanceList (uisf->dancefilter, _("All Dances"));
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgB, uiwidgetp);

  /* rating : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the dance rating displayed in the song selection */
  uiCreateColonLabel (&uiwidget, _("Dance Rating"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);
  uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_DANCE_RATING], &uiwidget);

  uisf->uirating = uiratingSpinboxCreate (&hbox, true);
  uiratingSizeGroupAdd (uisf->uirating, &sgC);

  /* level */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCELEVEL)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the dance level displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Dance Level"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);
    uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_DANCE_LEVEL], &uiwidget);

    uisf->uilevel = uilevelSpinboxCreate (&hbox, true);
    uilevelSizeGroupAdd (uisf->uilevel, &sgC);
  }

  /* status */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the status displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Status"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);
    uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_STATUS], &uiwidget);

    uisf->uistatus = uistatusSpinboxCreate (&hbox, true);
    uistatusSizeGroupAdd (uisf->uistatus, &sgC);
  }

  /* favorite */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_FAVORITE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the 'favorite' displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Favorite"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);
    uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_FAVORITE], &uiwidget);

    uisf->uifavorite = uifavoriteSpinboxCreate (&hbox);
  }

  /* status playable */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: the song status is marked as playable */
    uiCreateColonLabel (&uiwidget, _("Playable Status"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);
    uiutilsUIWidgetCopy (&uisf->labels [UISF_LABEL_PLAY_STATUS], &uiwidget);

    uisf->playstatusswitch = uiCreateSwitch (uisf->dfltpbflag);
    uiBoxPackStart (&hbox, uiSwitchGetUIWidget (uisf->playstatusswitch));
  }

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);

  logProcEnd (LOG_PROC, "uisfCreateDialog", "");
}

static bool
uisfResponseHandler (void *udata, long responseid)
{
  uisongfilter_t   *uisf = udata;
  int           x, y;

  logProcBegin (LOG_PROC, "uisfResponseHandler");

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      uiWindowGetPosition (&uisf->filterDialog, &x, &y);
      nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_Y, y);
      uiutilsUIWidgetInit (&uisf->filterDialog);
      break;
    }
    case RESPONSE_CLOSE: {
      uiWindowGetPosition (&uisf->filterDialog, &x, &y);
      nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_Y, y);
      uiWidgetHide (&uisf->filterDialog);
      break;
    }
    case RESPONSE_APPLY: {
      break;
    }
    case RESPONSE_RESET: {
      songfilterReset (uisf->songfilter);
      uisf->danceIdx = -1;
      uiDropDownSelectionSetNum (uisf->dancefilter, uisf->danceIdx);
      if (uisf->danceselcb != NULL) {
        uiutilsCallbackLongHandler (uisf->danceselcb, uisf->danceIdx);
      }
      uisfInitDisplay (uisf);
      if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
        uiSwitchSetValue (uisf->playstatusswitch, uisf->dfltpbflag);
      }
      break;
    }
  }

  if (responseid != RESPONSE_DELETE_WIN && responseid != RESPONSE_CLOSE) {
    uisfUpdate (uisf);
  }
  return UICB_CONT;
}

void
uisfUpdate (uisongfilter_t *uisf)
{
  const char    *searchstr;
  int           idx;
  int           nval;


  /* playlist : if active, no other filters are used */
  if (uisf->showplaylist) {
    /* turn playlist back on */
    songfilterOn (uisf->songfilter, SONG_FILTER_PLAYLIST);

    if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
      uisfDisableWidgets (uisf);

      /* if the playlist filter is on, then apply and return */
      if (uisf->applycb != NULL) {
        uiutilsCallbackHandler (uisf->applycb);
      }
      return;
    } else {
      uisfEnableWidgets (uisf);
    }
  }

  /* search : always active */
  searchstr = uiEntryGetValue (uisf->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisf->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating : always active */
  idx = uiratingGetValue (uisf->uirating);
  if (idx >= 0) {
    songfilterSetNum (uisf->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_RATING);
  }

  /* dance level */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCELEVEL)) {
    idx = uilevelGetValue (uisf->uilevel);
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_LEVEL_LOW, idx);
      songfilterSetNum (uisf->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_LEVEL_LOW);
      songfilterClear (uisf->songfilter, SONG_FILTER_LEVEL_HIGH);
    }
  }

  /* status */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS)) {
    idx = uistatusGetValue (uisf->uistatus);
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_STATUS);
    }
  }

  /* favorite */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_FAVORITE)) {
    idx = uifavoriteGetValue (uisf->uifavorite);
    if (idx != SONG_FAVORITE_NONE) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_FAVORITE, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_FAVORITE);
    }
  }

  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    nval = uiSwitchGetValue (uisf->playstatusswitch);
  } else {
    nval = uisf->dfltpbflag;
  }
  if (nval) {
    songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE, nval);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE);
  }

  if (uisf->applycb != NULL) {
    uiutilsCallbackHandler (uisf->applycb);
  }
  logProcEnd (LOG_PROC, "uisfResponseHandler", "");
}

/* internal routines */

static void
uisfDisableWidgets (uisongfilter_t *uisf)
{
  if (! uiutilsUIWidgetSet (&uisf->filterDialog)) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetDisable (&uisf->labels [i]);
  }
  uiDropDownDisable (uisf->sortbyfilter);
  uiEntryDisable (uisf->searchentry);
  uiDropDownDisable (uisf->dancefilter);
  uiDropDownDisable (uisf->genrefilter);
  uiratingDisable (uisf->uirating);
  uilevelDisable (uisf->uilevel);
  uistatusDisable (uisf->uistatus);
  uifavoriteDisable (uisf->uifavorite);
  uiSwitchDisable (uisf->playstatusswitch);
}

static void
uisfEnableWidgets (uisongfilter_t *uisf)
{
  if (! uiutilsUIWidgetSet (&uisf->filterDialog)) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetEnable (&uisf->labels [i]);
  }
  uiDropDownEnable (uisf->sortbyfilter);
  uiEntryEnable (uisf->searchentry);
  uiDropDownEnable (uisf->dancefilter);
  uiDropDownEnable (uisf->genrefilter);
  uiratingEnable (uisf->uirating);
  uilevelEnable (uisf->uilevel);
  uistatusEnable (uisf->uistatus);
  uifavoriteEnable (uisf->uifavorite);
  uiSwitchEnable (uisf->playstatusswitch);
}

static void
uisfPlaylistSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongfilter_t *uisf = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGetW (uisf->playlistfilter, path);
  uisfPlaylistSelect (uisf, idx);
}

static void
uisfSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongfilter_t *uisf = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGetW (uisf->sortbyfilter, path);
  uisfSortBySelect (uisf, idx);
}

static void
uisfGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongfilter_t *uisf = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGetW (uisf->genrefilter, path);
  uisfGenreSelect (uisf, idx);
}

static void
uisfDanceSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongfilter_t  *uisf = udata;
  long            danceIdx;

  danceIdx = uiDropDownSelectionGetW (uisf->dancefilter, path);
  uisf->danceIdx = danceIdx;
  uisfSetDanceIdx (uisf, danceIdx);
  if (uisf->danceselcb != NULL) {
    uiutilsCallbackLongHandler (uisf->danceselcb, danceIdx);
  }
}

static void
uisfUpdateFilterDialogDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  if (uisf->showplaylist) {
    songfilterOn (uisf->songfilter, SONG_FILTER_PLAYLIST);
    if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
      uisfDisableWidgets (uisf);
    } else {
      uisfEnableWidgets (uisf);
    }

    if (! uiutilsUIWidgetSet (&uisf->filterDialog)) {
      return;
    }

    uiWidgetShowAll (&uisf->playlistdisp);
  }

  if (! uisf->showplaylist) {
    songfilterOff (uisf->songfilter, SONG_FILTER_PLAYLIST);
    uisfEnableWidgets (uisf);

    if (! uiutilsUIWidgetSet (&uisf->filterDialog)) {
      return;
    }

    uiWidgetHide (&uisf->playlistdisp);
  }
}

