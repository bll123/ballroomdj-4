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
#include "bdjstring.h"
#include "bdj4ui.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "songfilter.h"
#include "uisongsel.h"
#include "ui.h"
#include "uilevel.h"
#include "uirating.h"
#include "uiutils.h"

/* song filter handling */
static void uisongselCreateFilterDialog (uisongsel_t *uisongsel);
static bool uisongselFilterResponseHandler (void *udata, int responseid);
static void uisongselFilterUpdate (uisongsel_t *uisongsel);
static void uisongselSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselDanceSelectSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

bool
uisongselFilterDialog (void *udata)
{
  uisongsel_t * uisongsel = udata;
  int         x, y;

  logProcBegin (LOG_PROC, "uisongselFilterDialog");

  uisongselCreateFilterDialog (uisongsel);

  uisongselInitFilterDisplay (uisongsel);
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    uiSwitchSetValue (uisongsel->playstatusswitch, uisongsel->dfltpbflag);
  }
  uiWidgetShowAll (&uisongsel->filterDialog);

  x = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y);
  uiWindowMove (&uisongsel->filterDialog, x, y);
  logProcEnd (LOG_PROC, "uisongselFilterDialog", "");
  return UICB_CONT;
}

void
uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGet (&uisongsel->dancesel, path);

  uiDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);
  uisongselFilterDanceProcess (uisongsel, idx);
  return;
}

/* internal routines */

static void
uisongselCreateFilterDialog (uisongsel_t *uisongsel)
{
  GtkWidget     *widget;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uiwidgetp;
  UIWidget      sg;
  int           max;
  int           len;

  logProcBegin (LOG_PROC, "uisongselCreateFilterDialog");

  uiCreateSizeGroupHoriz (&sg);

  uiutilsUICallbackIntInit (&uisongsel->filtercb,
      uisongselFilterResponseHandler, uisongsel);
  uiCreateDialog (&uisongsel->filterDialog, uisongsel->windowp,
      &uisongsel->filtercb,
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
  uiDialogPackInDialog (&uisongsel->filterDialog, &vbox);

  /* sort-by : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the method to sort the song selection display */
  uiCreateColonLabel (&uiwidget, _("Sort by"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  widget = uiComboboxCreate (uisongsel->filterDialog.widget,
      "", uisongselSortBySelectHandler, &uisongsel->sortbysel, uisongsel);
  uisongselCreateSortByList (uisongsel);
  uiBoxPackStartUW (&hbox, widget);

  /* search : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: filter the song selection with a search for text */
  uiCreateColonLabel (&uiwidget, _("Search"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiEntryCreate (uisongsel->searchentry);
  uiwidgetp = uiEntryGetUIWidget (uisongsel->searchentry);
  uiWidgetAlignHorizStart (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);

  /* genre */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_GENRE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the genre displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Genre"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);

    widget = uiComboboxCreate (uisongsel->filterDialog.widget,
        "", uisongselGenreSelectHandler,
        &uisongsel->filtergenresel, uisongsel);
    uisongselCreateGenreList (uisongsel);
    uiBoxPackStartUW (&hbox, widget);
  }

  /* dance : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the dance displayed in the song selection */
  uiCreateColonLabel (&uiwidget, _("Dance"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  widget = uiComboboxCreate (uisongsel->filterDialog.widget,
      "", uisongselDanceSelectSignal,
      &uisongsel->filterdancesel, uisongsel);
  /* CONTEXT: a filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->filterdancesel, _("All Dances"));
  uiBoxPackStartUW (&hbox, widget);

  /* rating : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: a filter: select the dance rating displayed in the song selection */
  uiCreateColonLabel (&uiwidget, _("Dance Rating"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uisongsel->uirating = uiratingSpinboxCreate (&hbox, true);

  /* level */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_DANCELEVEL)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the dance level displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Dance Level"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);

    uisongsel->uilevel = uilevelSpinboxCreate (&hbox, true);
  }

  /* status */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUS)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the status displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Status"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);

    uiSpinboxTextCreate (&uisongsel->filterstatussel, uisongsel);
    max = statusGetMaxWidth (uisongsel->status);
    /* CONTEXT: a filter: all statuses are displayed in the song selection */
    len = istrlen (_("Any Status"));
    if (len > max) {
      max = len;
    }
    uiSpinboxTextSet (&uisongsel->filterstatussel, -1,
        statusGetCount (uisongsel->status),
        max, NULL, NULL, uisongselStatusGet);
    uiBoxPackStart (&hbox, uiSpinboxGetUIWidget (&uisongsel->filterstatussel));
  }

  /* favorite */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_FAVORITE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: select the 'favorite' displayed in the song selection */
    uiCreateColonLabel (&uiwidget, _("Favorite"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);

    uiSpinboxTextCreate (&uisongsel->filterfavoritesel, uisongsel);
    uiSpinboxTextSet (&uisongsel->filterfavoritesel, 0,
        SONG_FAVORITE_MAX, 1, NULL, NULL, uisongselFavoriteGet);
    uiBoxPackStart (&hbox, uiSpinboxGetUIWidget (&uisongsel->filterfavoritesel));
  }

  /* status playable */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    uiCreateHorizBox (&hbox);
    uiBoxPackStart (&vbox, &hbox);

    /* CONTEXT: a filter: the song status is marked as playable */
    uiCreateColonLabel (&uiwidget, _("Playable Status"));
    uiBoxPackStart (&hbox, &uiwidget);
    uiSizeGroupAdd (&sg, &uiwidget);

    uisongsel->playstatusswitch = uiCreateSwitch (uisongsel->dfltpbflag);
    uiBoxPackStart (&hbox, uiSwitchGetUIWidget (uisongsel->playstatusswitch));
  }

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);

  logProcEnd (LOG_PROC, "uisongselCreateFilterDialog", "");
}

static bool
uisongselFilterResponseHandler (void *udata, int responseid)
{
  uisongsel_t   *uisongsel = udata;
  int           x, y;

  logProcBegin (LOG_PROC, "uisongselFilterResponseHandler");

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      uiWindowGetPosition (&uisongsel->filterDialog, &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);
      uiutilsUIWidgetInit (&uisongsel->filterDialog);
      break;
    }
    case RESPONSE_CLOSE: {
      uiWindowGetPosition (&uisongsel->filterDialog, &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      uiWidgetHide (&uisongsel->filterDialog);
      break;
    }
    case RESPONSE_APPLY: {
      break;
    }
    case RESPONSE_RESET: {
      songfilterReset (uisongsel->songfilter);
      uisongsel->danceIdx = -1;
      uiDropDownSelectionSetNum (&uisongsel->dancesel, uisongsel->danceIdx);
      uiDropDownSelectionSetNum (&uisongsel->filterdancesel, uisongsel->danceIdx);
      uisongselInitFilterDisplay (uisongsel);
      if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
        uiSwitchSetValue (uisongsel->playstatusswitch, uisongsel->dfltpbflag);
      }
      break;
    }
  }

  if (responseid != RESPONSE_DELETE_WIN) {
    uisongselFilterUpdate (uisongsel);
  }
  return UICB_CONT;
}

void
uisongselFilterUpdate (uisongsel_t *uisongsel)
{
  const char    *searchstr;
  int           idx;
  int           nval;


  /* search : always active */
  searchstr = uiEntryGetValue (uisongsel->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating : always active */
  idx = uiratingGetValue (uisongsel->uirating);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_RATING);
  }

  /* dance level */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_DANCELEVEL)) {
    idx = uilevelGetValue (uisongsel->uilevel);
    if (idx >= 0) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW, idx);
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW);
      songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH);
    }
  }

  /* status */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUS)) {
    idx = uiSpinboxTextGetValue (&uisongsel->filterstatussel);
    if (idx >= 0) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_STATUS, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_STATUS);
    }
  }

  /* favorite */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_FAVORITE)) {
    idx = uiSpinboxTextGetValue (&uisongsel->filterfavoritesel);
    if (idx != SONG_FAVORITE_NONE) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_FAVORITE, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_FAVORITE);
    }
  }

  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    nval = uiSwitchGetValue (uisongsel->playstatusswitch);
  } else {
    nval = uisongsel->dfltpbflag;
  }
  if (nval) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_STATUS_PLAYABLE, nval);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_STATUS_PLAYABLE);
  }

  uisongselApplySongFilter (uisongsel);
  logProcEnd (LOG_PROC, "uisongselFilterResponseHandler", "");
}

static void
uisongselSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGet (&uisongsel->sortbysel, path);
  uisongselSortBySelect (uisongsel, idx);
}

static void
uisongselGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGet (&uisongsel->filtergenresel, path);
  uisongselGenreSelect (uisongsel, idx);
}

static void
uisongselDanceSelectSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGet (&uisongsel->filterdancesel, path);
  uiDropDownSelectionSetNum (&uisongsel->dancesel, idx);
  uisongselDanceSelect (uisongsel, idx);
}

