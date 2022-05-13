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

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdj4playerui.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "songfilter.h"
#include "uisongsel.h"
#include "ui.h"
#include "uiutils.h"

enum {
  RESPONSE_NONE,
  RESPONSE_RESET,
};

/* song filter handling */
static void uisongselCreateFilterDialog (uisongsel_t *uisongsel);
static void uisongselFilterResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata);
static void uisongselFilterUpdate (uisongsel_t *uisongsel);
static void uisongselSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselDanceSelectSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

void
uisongselFilterDialog (void *udata)
{
  uisongsel_t * uisongsel = udata;
  gint        x, y;

  logProcBegin (LOG_PROC, "uisongselFilterDialog");

  if (uisongsel->filterDialog == NULL) {
    uisongselCreateFilterDialog (uisongsel);
  }

  uisongselInitFilterDisplay (uisongsel);
  if (uisongsel->statusPlayable != NULL) {
    uiSwitchSetValue (uisongsel->statusPlayable, uisongsel->dfltpbflag);
  }
  uiWidgetShowAllW (uisongsel->filterDialog);

  x = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y);
  uiWindowMove (uisongsel->filterDialog, x, y);
  logProcEnd (LOG_PROC, "uisongselFilterDialog", "");
}

void
uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiDropDownSelectionGet (&uisongsel->dancesel, path);
  if (uisongsel->filterDialog != NULL) {
    uiDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);
  }
  uisongselFilterDanceProcess (uisongsel, idx);
  return;
}

/* internal routines */

static void
uisongselCreateFilterDialog (uisongsel_t *uisongsel)
{
  GtkWidget     *content;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  UIWidget      sg;
  int           max;
  int           len;

  logProcBegin (LOG_PROC, "uisongselCreateFilterDialog");

  uiCreateSizeGroupHoriz (&sg);

  uisongsel->filterDialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: title for the filter dialog */
      _("Filter Songs"),
      GTK_WINDOW (uisongsel->window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: filter dialog: closes the dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      /* CONTEXT: filter dialog: resets the selections */
      _("Reset"),
      RESPONSE_RESET,
      /* CONTEXT: filter dialog: applies the selections */
      _("Apply"),
      GTK_RESPONSE_APPLY,
      NULL
      );

  content = gtk_dialog_get_content_area (GTK_DIALOG (uisongsel->filterDialog));
  uiWidgetSetAllMarginsW (content, uiBaseMarginSz * 2);

  vbox = uiCreateVertBoxWW ();
  uiBoxPackInWindowWW (content, vbox);

  /* sort-by : always available */
  hbox = uiCreateHorizBoxWW ();
  uiBoxPackStartWW (vbox, hbox);

  /* CONTEXT: a filter: select the method to sort the song selection display */
  widget = uiCreateColonLabelW (_("Sort by"));
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  widget = uiComboboxCreate (uisongsel->filterDialog,
      "", uisongselSortBySelectHandler, &uisongsel->sortbysel, uisongsel);
  uisongselCreateSortByList (uisongsel);
  uiBoxPackStartWW (hbox, widget);

  /* search : always available */
  hbox = uiCreateHorizBoxWW ();
  uiBoxPackStartWW (vbox, hbox);

  /* CONTEXT: a filter: filter the song selection with a search for text */
  widget = uiCreateColonLabelW (_("Search"));
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  widget = uiEntryCreate (&uisongsel->searchentry);
  uiWidgetAlignHorizStartW (widget);
  uiBoxPackStartWW (hbox, widget);

  /* genre */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_GENRE)) {
    hbox = uiCreateHorizBoxWW ();
    uiBoxPackStartWW (vbox, hbox);

    /* CONTEXT: a filter: select the genre displayed in the song selection */
    widget = uiCreateColonLabelW (_("Genre"));
    uiBoxPackStartWW (hbox, widget);
    uiSizeGroupAdd (&sg, widget);

    widget = uiComboboxCreate (uisongsel->filterDialog,
        "", uisongselGenreSelectHandler,
        &uisongsel->filtergenresel, uisongsel);
    uisongselCreateGenreList (uisongsel);
    uiBoxPackStartWW (hbox, widget);
  }

  /* dance : always available */
  hbox = uiCreateHorizBoxWW ();
  uiBoxPackStartWW (vbox, hbox);

  /* CONTEXT: a filter: select the dance displayed in the song selection */
  widget = uiCreateColonLabelW (_("Dance"));
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  widget = uiComboboxCreate (uisongsel->filterDialog,
      "", uisongselDanceSelectSignal,
      &uisongsel->filterdancesel, uisongsel);
  /* CONTEXT: a filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->filterdancesel, _("All Dances"));
  uiBoxPackStartWW (hbox, widget);

  /* rating : always available */
  hbox = uiCreateHorizBoxWW ();
  uiBoxPackStartWW (vbox, hbox);

  /* CONTEXT: a filter: select the dance rating displayed in the song selection */
  widget = uiCreateColonLabelW (_("Dance Rating"));
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAdd (&sg, widget);

  widget = uiSpinboxTextCreate (&uisongsel->filterratingsel, uisongsel);
  max = ratingGetMaxWidth (uisongsel->ratings);
  /* CONTEXT: a filter: all dance ratings will be listed */
  len = istrlen (_("All Ratings"));
  if (len > max) {
    max = len;
  }
  uiSpinboxTextSet (&uisongsel->filterratingsel, -1,
      ratingGetCount (uisongsel->ratings),
      max, NULL, NULL, uisongselRatingGet);
  uiBoxPackStartWW (hbox, widget);

  /* level */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_DANCELEVEL)) {
    hbox = uiCreateHorizBoxWW ();
    uiBoxPackStartWW (vbox, hbox);

    /* CONTEXT: a filter: select the dance level displayed in the song selection */
    widget = uiCreateColonLabelW (_("Dance Level"));
    uiBoxPackStartWW (hbox, widget);
    uiSizeGroupAdd (&sg, widget);

    widget = uiSpinboxTextCreate (&uisongsel->filterlevelsel, uisongsel);
    max = levelGetMaxWidth (uisongsel->levels);
    /* CONTEXT: a filter: all dance levels will be listed */
    len = istrlen (_("All Levels"));
    if (len > max) {
      max = len;
    }
    uiSpinboxTextSet (&uisongsel->filterlevelsel, -1,
        levelGetCount (uisongsel->levels),
        max, NULL, NULL, uisongselLevelGet);
    uiBoxPackStartWW (hbox, widget);
  }

  /* status */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUS)) {
    hbox = uiCreateHorizBoxWW ();
    uiBoxPackStartWW (vbox, hbox);

    /* CONTEXT: a filter: select the status displayed in the song selection */
    widget = uiCreateColonLabelW (_("Status"));
    uiBoxPackStartWW (hbox, widget);
    uiSizeGroupAdd (&sg, widget);

    widget = uiSpinboxTextCreate (&uisongsel->filterstatussel, uisongsel);
    max = statusGetMaxWidth (uisongsel->status);
    /* CONTEXT: a filter: all statuses are displayed in the song selection */
    len = istrlen (_("Any Status"));
    if (len > max) {
      max = len;
    }
    uiSpinboxTextSet (&uisongsel->filterstatussel, -1,
        statusGetCount (uisongsel->status),
        max, NULL, NULL, uisongselStatusGet);
    uiBoxPackStartWW (hbox, widget);
  }

  /* favorite */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_FAVORITE)) {
    hbox = uiCreateHorizBoxWW ();
    uiBoxPackStartWW (vbox, hbox);

    /* CONTEXT: a filter: select the 'favorite' displayed in the song selection */
    widget = uiCreateColonLabelW (_("Favorite"));
    uiBoxPackStartWW (hbox, widget);
    uiSizeGroupAdd (&sg, widget);

    widget = uiSpinboxTextCreate (&uisongsel->filterfavoritesel, uisongsel);
    uiSpinboxTextSet (&uisongsel->filterfavoritesel, 0,
        SONG_FAVORITE_MAX, 1, NULL, NULL, uisongselFavoriteGet);
    uiBoxPackStartWW (hbox, widget);
  }

  /* status playable */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_STATUSPLAYABLE)) {
    hbox = uiCreateHorizBoxWW ();
    uiBoxPackStartWW (vbox, hbox);

    /* CONTEXT: a filter: the song status is marked as playable */
    widget = uiCreateColonLabelW (_("Playable Status"));
    uiBoxPackStartWW (hbox, widget);
    uiSizeGroupAdd (&sg, widget);

    widget = uiCreateSwitch (uisongsel->dfltpbflag);
    uiBoxPackStartWW (hbox, widget);
    uisongsel->statusPlayable = widget;
  }

  /* the dialog doesn't have any space above the buttons */
  hbox = uiCreateHorizBoxWW ();
  uiBoxPackStartWW (vbox, hbox);

  widget = uiCreateLabelW (" ");
  uiBoxPackStartWW (hbox, widget);

  g_signal_connect (uisongsel->filterDialog, "response",
      G_CALLBACK (uisongselFilterResponseHandler), uisongsel);
  logProcEnd (LOG_PROC, "uisongselCreateFilterDialog", "");
}

static void
uisongselFilterResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  uisongsel_t   *uisongsel = udata;
  gint          x, y;

  logProcBegin (LOG_PROC, "uisongselFilterResponseHandler");

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      uiWindowGetPosition (uisongsel->filterDialog, &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      uisongsel->filterDialog = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      gtk_window_get_position (GTK_WINDOW (uisongsel->filterDialog), &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      uiWidgetHideW (uisongsel->filterDialog);
      break;
    }
    case GTK_RESPONSE_APPLY: {
      break;
    }
    case RESPONSE_RESET: {
      songfilterReset (uisongsel->songfilter);
      uisongsel->danceIdx = -1;
      uiDropDownSelectionSetNum (&uisongsel->dancesel, uisongsel->danceIdx);
      uiDropDownSelectionSetNum (&uisongsel->filterdancesel, uisongsel->danceIdx);
      uisongselInitFilterDisplay (uisongsel);
      if (uisongsel->statusPlayable != NULL) {
        uiSwitchSetValue (uisongsel->statusPlayable, uisongsel->dfltpbflag);
      }
      break;
    }
  }

  uisongselFilterUpdate (uisongsel);
}

void
uisongselFilterUpdate (uisongsel_t *uisongsel)
{
  const char    *searchstr;
  int           idx;
  int           nval;


  /* search : always active */
  searchstr = uiEntryGetValue (&uisongsel->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating : always active */
  idx = uiSpinboxTextGetValue (&uisongsel->filterratingsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_RATING);
  }

  /* dance level */
  if (songfilterCheckSelection (uisongsel->songfilter, FILTER_DISP_DANCELEVEL)) {
    idx = uiSpinboxTextGetValue (&uisongsel->filterlevelsel);
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
    nval = gtk_switch_get_active (GTK_SWITCH (uisongsel->statusPlayable));
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

