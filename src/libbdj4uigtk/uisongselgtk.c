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

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjopt.h"
#include "bdj4playerui.h"
#include "conn.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "songfilter.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  RESPONSE_NONE,
  RESPONSE_RESET,
};

enum {
  SONGSEL_COL_ELLIPSIZE,
  SONGSEL_COL_FONT,
  SONGSEL_COL_IDX,
  SONGSEL_COL_SORTIDX,
  SONGSEL_COL_DBIDX,
  SONGSEL_COL_FAV_COLOR,
  SONGSEL_COL_MAX,
};

#define STORE_ROWS     60

typedef struct {
  GtkWidget           *parentwin;
  GtkWidget           *vbox;
  GtkWidget           *songselTree;
  GtkWidget           *songselScrollbar;
  GtkEventController  *scrollController;
  GtkTreeViewColumn   *favColumn;
  GtkWidget           *filterDialog;
  GtkWidget           *statusPlayable;
  GtkWidget           *queueButton;
  GtkWidget           *selectButton;
  /* other data */
  int               lastTreeSize;
  double            lastRowHeight;
  int               maxRows;
} uisongselgtk_t;

static void uisongselInitializeStore (uisongsel_t *uisongsel);
static void uisongselCreateRows (uisongsel_t *uisongsel);

static void uisongselQueueProcessSignal (GtkButton *b, gpointer udata);
static void uisongselFilterDialog (GtkButton *b, gpointer udata);
static void uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static gboolean uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata);
static void uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);
static gboolean uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event,
    gpointer udata);
static void uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer user_data);
static void uisongselCreateFilterDialog (uisongsel_t *uisongsel);
static void uisongselFilterResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata);

static void uisongselSortBySelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

static void uisongselGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

static void uisongselDanceSelectSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;

  uiw = malloc (sizeof (uisongselgtk_t));
  uiw->parentwin = NULL;
  uiw->vbox = NULL;
  uiw->songselTree = NULL;
  uiw->songselScrollbar = NULL;
  uiw->scrollController = NULL;
  uiw->favColumn = NULL;
  uiw->filterDialog = NULL;
  uiw->statusPlayable = NULL;
  uiw->queueButton = NULL;
  uiw->selectButton = NULL;
  uiw->lastTreeSize = 0;
  uiw->lastRowHeight = 0.0;
  uiw->maxRows = 0;
  uisongsel->uiWidgetData = uiw;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  if (uisongsel->uiWidgetData != NULL) {
    free (uisongsel->uiWidgetData);
    uisongsel->uiWidgetData = NULL;
  }
}

GtkWidget *
uisongselBuildUI (uisongsel_t *uisongsel, GtkWidget *parentwin)
{
  uisongselgtk_t    *uiw;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *widget;
  GtkAdjustment     *adjustment;
  slist_t           *sellist;
  char              tbuff [200];

  logProcBegin (LOG_PROC, "uisongselBuildUI");

  uiw = uisongsel->uiWidgetData;
  uiw->parentwin = parentwin;

  uiw->vbox = uiutilsCreateVertBox ();
  gtk_widget_set_hexpand (uiw->vbox, TRUE);
  gtk_widget_set_vexpand (uiw->vbox, TRUE);

  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_hexpand (hbox, TRUE);
  uiutilsBoxPackStart (uiw->vbox, hbox);

  if (uisongsel->dispselType == DISP_SEL_SONGSEL) {
    /* CONTEXT: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    widget = uiutilsCreateButton (tbuff, NULL,
        uisongselQueueProcessSignal, uisongsel);
    uiutilsBoxPackStart (hbox, widget);
    uiw->selectButton = widget;
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_REQUEST ||
      uisongsel->dispselType == DISP_SEL_MM) {
    if (uisongsel->dispselType == DISP_SEL_REQUEST) {
      /* CONTEXT: queue a song to be played */
      strlcpy (tbuff, _("Queue"), sizeof (tbuff));
    }
    if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
        uisongsel->dispselType == DISP_SEL_MM) {
      /* CONTEXT: play the selected song */
      strlcpy (tbuff, _("Play"), sizeof (tbuff));
    }
    widget = uiutilsCreateButton (tbuff, NULL,
        uisongselQueueProcessSignal, uisongsel);
    uiutilsBoxPackStart (hbox, widget);

    /* only set the queue button for songsel and mm */
    /* if the queue button matches, the current song will be cleared */
    /* and the song will be queued to musicq B (the hidden queue) */
    if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
        uisongsel->dispselType == DISP_SEL_MM) {
      uiw->queueButton = widget;
    }
  }

  widget = uiutilsComboboxCreate (parentwin,
      "", uisongselFilterDanceSignal,
      &uisongsel->dancesel, uisongsel);
  /* CONTEXT: filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->dancesel, _("All Dances"));
  uiutilsBoxPackEnd (hbox, widget);

  /* CONTEXT: a button that starts the filters (narrowing down song selections) dialog */
  widget = uiutilsCreateButton (_("Filters"), NULL,
      uisongselFilterDialog, uisongsel);
  uiutilsBoxPackEnd (hbox, widget);

  hbox = uiutilsCreateHorizBox ();
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uiw->vbox), hbox, TRUE, TRUE, 0);

  adjustment = gtk_adjustment_new (0.0, 0.0, uisongsel->dfilterCount, 1.0, 10.0, 10.0);
  uiw->songselScrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  assert (uiw->songselScrollbar != NULL);
  gtk_widget_set_vexpand (uiw->songselScrollbar, TRUE);
  uiutilsSetCss (uiw->songselScrollbar,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  uiutilsBoxPackEnd (hbox, uiw->songselScrollbar);
  g_signal_connect (uiw->songselScrollbar, "change-value",
      G_CALLBACK (uisongselScroll), uisongsel);

  vbox = uiutilsCreateVertBox ();
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
  uiutilsBoxPackStart (vbox, widget);

  uiw->songselTree = uiutilsCreateTreeView ();
  assert (uiw->songselTree != NULL);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiw->songselTree), TRUE);
  gtk_widget_set_halign (uiw->songselTree, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (uiw->songselTree, TRUE);
  gtk_widget_set_vexpand (uiw->songselTree, TRUE);

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uiw->songselTree));
  gtk_adjustment_set_upper (adjustment, uisongsel->dfilterCount);
  uiw->scrollController =
      gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uiw->songselTree, GDK_SCROLL_MASK);
  gtk_container_add (GTK_CONTAINER (widget), uiw->songselTree);
  g_signal_connect (uiw->songselTree, "row-activated",
      G_CALLBACK (uisongselCheckFavChgSignal), uisongsel);
  g_signal_connect (uiw->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  uiw->favColumn = uiutilsAddDisplayColumns (
      uiw->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uiw->maxRows = STORE_ROWS;
  uisongselPopulateData (uisongsel);

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: initial");
  uisongselPopulateData (uisongsel);

  uiutilsDropDownSelectionSetNum (&uisongsel->dancesel, -1);

  g_signal_connect (uiw->songselTree, "size-allocate",
      G_CALLBACK (uisongselProcessTreeSize), uisongsel);

  logProcEnd (LOG_PROC, "uisongselBuildUI", "");
  return uiw->vbox;
}

void
uisongselClearData (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkTreeModel        * model = NULL;

  logProcBegin (LOG_PROC, "uisongselClearData");

  uiw = uisongsel->uiWidgetData;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));
  gtk_list_store_clear (GTK_LIST_STORE (model));
  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkTreeModel        * model = NULL;
  GtkTreeIter         iter;
  GtkAdjustment       *adjustment;
  ssize_t             idx;
  int                 count;
  song_t              * song;
  songfavoriteinfo_t  * favorite;
  char                * color;
  char                tmp [40];
  char                tbuff [100];
  dbidx_t             dbidx;
  char                * listingFont;
  slist_t             *sellist;

  logProcBegin (LOG_PROC, "uisongselPopulateData");
  uiw = uisongsel->uiWidgetData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
  gtk_adjustment_set_upper (adjustment, uisongsel->dfilterCount);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));

  count = 0;
  idx = uisongsel->idxStart;
  while (count < uiw->maxRows) {
    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      song = dbGetByIdx (uisongsel->musicdb, dbidx);
      if (song != NULL) {
        favorite = songGetFavoriteData (song);
        color = favorite->color;
        if (strcmp (color, "") == 0) {
          uiutilsGetForegroundColor (uiw->songselTree, tmp, sizeof (tmp));
          color = tmp;
        }

        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            SONGSEL_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
            SONGSEL_COL_FONT, listingFont,
            SONGSEL_COL_IDX, idx,
            SONGSEL_COL_SORTIDX, idx,
            SONGSEL_COL_DBIDX, dbidx,
            SONGSEL_COL_FAV_COLOR, color,
            -1);

        sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
        uiutilsSetDisplayColumns (
            GTK_LIST_STORE (model), &iter, sellist,
            song, SONGSEL_COL_MAX);
      }
    }

    ++idx;
    ++count;
  }
  logProcEnd (LOG_PROC, "uisongselPopulateData", "");
}

void
uisongselSetFavoriteForeground (uisongsel_t *uisongsel, char *color)
{
  uisongselgtk_t      *uiw;
  char                tbuff [100];
  char                tmp [40];

  uiw = uisongsel->uiWidgetData;

  if (strcmp (color, "") != 0) {
    snprintf (tbuff, sizeof (tbuff),
        "spinbutton { color: %s; } ", color);
    uiutilsSetCss (uisongsel->filterfavoritesel.spinbox, tbuff);
  } else {
    uiutilsGetForegroundColor (uiw->songselTree, tmp, sizeof (tmp));
    snprintf (tbuff, sizeof (tbuff),
        "spinbutton { color: %s; } ", tmp);
    uiutilsSetCss (uisongsel->filterfavoritesel.spinbox, tbuff);
  }
}

/* internal routines */

static void
uisongselInitializeStore (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkListStore      *store = NULL;
  GType             *songselstoretypes;
  int               colcount = 0;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

  uiw = uisongsel->uiWidgetData;
  songselstoretypes = malloc (sizeof (GType) * SONGSEL_COL_MAX);
  colcount = 0;
  /* attributes ellipsize/align/font*/
  songselstoretypes [colcount++] = G_TYPE_INT;
  songselstoretypes [colcount++] = G_TYPE_STRING;
  /* internal idx/sortidx/dbidx/fav color */
  songselstoretypes [colcount++] = G_TYPE_ULONG,
  songselstoretypes [colcount++] = G_TYPE_ULONG,
  songselstoretypes [colcount++] = G_TYPE_ULONG,
  songselstoretypes [colcount++] = G_TYPE_STRING,

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  songselstoretypes = uiutilsAddDisplayTypes (songselstoretypes, sellist, &colcount);
  store = gtk_list_store_newv (colcount, songselstoretypes);
  assert (store != NULL);
  free (songselstoretypes);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->songselTree), GTK_TREE_MODEL (store));
  logProcEnd (LOG_PROC, "uisongselInitializeStore", "");
}

static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkTreeModel      *model = NULL;
  GtkTreeIter       iter;

  logProcBegin (LOG_PROC, "uisongselCreateRows");

  uiw = uisongsel->uiWidgetData;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));
  /* enough pre-allocated rows are needed so that if the windows is */
  /* maximized and the font size is not large, enough rows are available */
  /* to be displayed */
  for (int i = 0; i < STORE_ROWS; ++i) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  }

  /* initial song filter process */
  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  logProcEnd (LOG_PROC, "uisongselCreateRows", "");
}

static void
uisongselQueueProcessSignal (GtkButton *b, gpointer udata)
{
  uisongselgtk_t    * uiw;
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  int               count;
  gulong            dbidx;
  musicqidx_t       mqidx = MUSICQ_CURRENT;

  logProcBegin (LOG_PROC, "uisongselQueueProcessSignal");

  uiw = uisongsel->uiWidgetData;
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uiw->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselQueueProcess", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  if (GTK_WIDGET (b) == uiw->queueButton) {
    /* clear any playing song */
    connSendMessage (uisongsel->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
    /* and queue to the hidden music queue */
    mqidx = MUSICQ_B;
  }
  if (GTK_WIDGET (b) == uiw->selectButton) {
    mqidx = MUSICQ_A;
  }
  uisongselQueueProcess (uisongsel, dbidx, mqidx);

  logProcEnd (LOG_PROC, "uisongselQueueProcessSignal", "");
  return;
}

static void
uisongselFilterDialog (GtkButton *b, gpointer udata)
{
  uisongselgtk_t      * uiw;
  uisongsel_t * uisongsel = udata;
  gint        x, y;

  logProcBegin (LOG_PROC, "uisongselFilterDialog");

  uiw = uisongsel->uiWidgetData;
  if (uiw->filterDialog == NULL) {
    uisongselCreateFilterDialog (uisongsel);
  }

  uisongselInitFilterDisplay (uisongsel);
  if (uiw->statusPlayable != NULL) {
    uiutilsSwitchSetValue (uiw->statusPlayable, uisongsel->dfltpbflag);
  }
  gtk_widget_show_all (uiw->filterDialog);

  x = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (uiw->filterDialog), x, y);
  }
  logProcEnd (LOG_PROC, "uisongselFilterDialog", "");
}

static void
uisongselFilterDanceSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiutilsDropDownSelectionGet (&uisongsel->dancesel, path);
  uisongselFilterDanceProcess (uisongsel, idx);
  return;
}

static gboolean
uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata)
{
  uisongselgtk_t      * uiw;
  uisongsel_t   *uisongsel = udata;
  double        start;

  logProcBegin (LOG_PROC, "uisongselScroll");

  uiw = uisongsel->uiWidgetData;
  if (value < 0 || value > uisongsel->dfilterCount) {
    logProcEnd (LOG_PROC, "uisongselScroll", "bad-value");
    return TRUE;
  }

  start = floor (value);
  if (start > uisongsel->dfilterCount - uiw->maxRows) {
    start = uisongsel->dfilterCount - uiw->maxRows;
  }
  uisongsel->idxStart = (ssize_t) start;

  logMsg (LOG_DBG, LOG_SONGSEL, "populate: scroll");
  uisongselPopulateData (uisongsel);
  logProcEnd (LOG_PROC, "uisongselScroll", "");
  return FALSE;
}

static void
uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uisongselgtk_t      * uiw;
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  int               count;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  gulong            dbidx;


  logProcBegin (LOG_PROC, "uisongselCheckFavChgSignal");

  uiw = uisongsel->uiWidgetData;
  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "not-fav-col");
    return;
  }

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uiw->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  uisongselChangeFavorite (uisongsel, dbidx);

  logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "");
}

static gboolean
uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event, gpointer udata)
{
  uisongselgtk_t  *uiw;
  uisongsel_t     *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselScrollEvent");

  uiw = uisongsel->uiWidgetData;

  /* i'd like to have a way to turn off smooth scrolling for the application */
  if (event->direction == GDK_SCROLL_SMOOTH) {
    double dx, dy;

    gdk_event_get_scroll_deltas ((GdkEvent *) event, &dx, &dy);
    if (dy < 0.0) {
      --uisongsel->idxStart;
    }
    if (dy > 0.0) {
      ++uisongsel->idxStart;
    }
  }
  if (event->direction == GDK_SCROLL_DOWN) {
    ++uisongsel->idxStart;
  }
  if (event->direction == GDK_SCROLL_UP) {
    --uisongsel->idxStart;
  }
  /* using g_signal_emit_by_name causes memory corruption */
  uisongselScroll (GTK_RANGE (uiw->songselScrollbar), 0,
      (double) uisongsel->idxStart, uisongsel);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
  return TRUE;
}

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uisongselgtk_t  *uiw;
  uisongsel_t   *uisongsel = udata;
  GtkAdjustment *adjustment;
  double        ps;

  logProcBegin (LOG_PROC, "uisongselProcessTreeSize");

  uiw = uisongsel->uiWidgetData;

  if (allocation->height != uiw->lastTreeSize) {
    if (allocation->height < 200) {
      logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "small-alloc-height");
      return;
    }

    /* the step increment is useless */
    /* the page-size and upper can be used to determine */
    /* how many rows can be displayed */
    adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uiw->songselTree));
    ps = gtk_adjustment_get_page_size (adjustment);


    if (uiw->lastRowHeight == 0.0) {
      double      u, hpr;

      u = gtk_adjustment_get_upper (adjustment);
      hpr = u / STORE_ROWS;
      /* save the original step increment for use in calculations later */
      /* the current step increment has been adjusted for the current */
      /* number of rows that are displayed */
      uiw->lastRowHeight = hpr;
    }

    uiw->maxRows = (int) (ps / uiw->lastRowHeight);
    ++uiw->maxRows;
    logMsg (LOG_DBG, LOG_IMPORTANT, "max-rows:%d", uiw->maxRows);

    /* force a redraw */
    gtk_adjustment_set_value (adjustment, 0.0);
    /* using g_signal_emit_by_name causes memory corruption */
    uisongselScroll (GTK_RANGE (uiw->songselScrollbar), 0, 0.0, uisongsel);

    adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
    /* the step increment does not work correctly with smooth scrolling */
    /* and it appears there's no easy way to turn smooth scrolling off */
    gtk_adjustment_set_step_increment (adjustment, 4.0);
    gtk_adjustment_set_page_increment (adjustment, (double) uiw->maxRows);
    gtk_adjustment_set_page_size (adjustment, (double) uiw->maxRows);

    uiw->lastTreeSize = allocation->height;

    logMsg (LOG_DBG, LOG_SONGSEL, "populate: tree size change");
    uisongselPopulateData (uisongsel);

    /* using g_signal_emit_by_name causes memory corruption */
    uisongselScroll (GTK_RANGE (uiw->songselScrollbar), 0,
        (double) uisongsel->idxStart, uisongsel);
  }
  logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "");
}

static void
uisongselCreateFilterDialog (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;
  GtkWidget     *content;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  GtkSizeGroup  *sgA;
  int           max;
  int           len;

  logProcBegin (LOG_PROC, "uisongselCreateFilterDialog");

  uiw = uisongsel->uiWidgetData;

  sgA = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  uiw->filterDialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: title for the filter dialog */
      _("Filter Songs"),
      GTK_WINDOW (uiw->parentwin),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: action button for the filter dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      /* CONTEXT: action button for the filter dialog */
      _("Reset"),
      RESPONSE_RESET,
      /* CONTEXT: action button for the filter dialog */
      _("Apply"),
      GTK_RESPONSE_APPLY,
      NULL
      );

  content = gtk_dialog_get_content_area (GTK_DIALOG (uiw->filterDialog));
  uiutilsWidgetSetAllMargins (content, uiutilsBaseMarginSz * 2);

  vbox = uiutilsCreateVertBox ();
  gtk_widget_set_hexpand (vbox, FALSE);
  gtk_widget_set_vexpand (vbox, FALSE);
  gtk_container_add (GTK_CONTAINER (content), vbox);

  /* sort-by : always available */
  hbox = uiutilsCreateHorizBox ();
  uiutilsBoxPackStart (vbox, hbox);

  /* CONTEXT: a filter: select the method to sort the song selection display */
  widget = uiutilsCreateColonLabel (_("Sort by"));
  uiutilsBoxPackStart (hbox, widget);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uiw->filterDialog,
      "", uisongselSortBySelectHandler, &uisongsel->sortbysel, uisongsel);
  uisongselCreateSortByList (uisongsel);
  uiutilsBoxPackStart (hbox, widget);

  /* search : always available */
  hbox = uiutilsCreateHorizBox ();
  uiutilsBoxPackStart (vbox, hbox);

  /* CONTEXT: a filter: filter the song selection with a search for text */
  widget = uiutilsCreateColonLabel (_("Search"));
  uiutilsBoxPackStart (hbox, widget);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsEntryCreate (&uisongsel->searchentry);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (widget, TRUE);
  uiutilsBoxPackStart (hbox, widget);

  /* genre */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_GENRE)) {
    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (vbox, hbox);

    /* CONTEXT: a filter: select the genre displayed in the song selection */
    widget = uiutilsCreateColonLabel (_("Genre"));
    uiutilsBoxPackStart (hbox, widget);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsComboboxCreate (uiw->filterDialog,
        "", uisongselGenreSelectHandler,
        &uisongsel->filtergenresel, uisongsel);
    uisongselCreateGenreList (uisongsel);
    uiutilsBoxPackStart (hbox, widget);
  }

  /* dance : always available */
  hbox = uiutilsCreateHorizBox ();
  uiutilsBoxPackStart (vbox, hbox);

  /* CONTEXT: a filter: select the dance displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Dance"));
  uiutilsBoxPackStart (hbox, widget);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uiw->filterDialog,
      "", uisongselDanceSelectSignal,
      &uisongsel->filterdancesel, uisongsel);
  /* CONTEXT: a filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->filterdancesel, _("All Dances"));
  uiutilsBoxPackStart (hbox, widget);

  /* rating : always available */
  hbox = uiutilsCreateHorizBox ();
  uiutilsBoxPackStart (vbox, hbox);

  /* CONTEXT: a filter: select the dance rating displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Dance Rating"));
  uiutilsBoxPackStart (hbox, widget);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterratingsel, uisongsel);
  max = ratingGetMaxWidth (uisongsel->ratings);
  /* CONTEXT: a filter: all dance ratings will be listed */
  len = istrlen (_("All Ratings"));
  if (len > max) {
    max = len;
  }
  uiutilsSpinboxTextSet (&uisongsel->filterratingsel, -1,
      ratingGetCount (uisongsel->ratings),
      max, NULL, uisongselRatingGet);
  uiutilsBoxPackStart (hbox, widget);

  /* level */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_DANCELEVEL)) {
    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (vbox, hbox);

    /* CONTEXT: a filter: select the dance level displayed in the song selection */
    widget = uiutilsCreateColonLabel (_("Dance Level"));
    uiutilsBoxPackStart (hbox, widget);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsSpinboxTextCreate (&uisongsel->filterlevelsel, uisongsel);
    max = levelGetMaxWidth (uisongsel->levels);
    /* CONTEXT: a filter: all dance levels will be listed */
    len = istrlen (_("All Levels"));
    if (len > max) {
      max = len;
    }
    uiutilsSpinboxTextSet (&uisongsel->filterlevelsel, -1,
        levelGetCount (uisongsel->levels),
        max, NULL, uisongselLevelGet);
    uiutilsBoxPackStart (hbox, widget);
  }

  /* status */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_STATUS)) {
    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (vbox, hbox);

    /* CONTEXT: a filter: select the status displayed in the song selection */
    widget = uiutilsCreateColonLabel (_("Status"));
    uiutilsBoxPackStart (hbox, widget);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsSpinboxTextCreate (&uisongsel->filterstatussel, uisongsel);
    max = statusGetMaxWidth (uisongsel->status);
    /* CONTEXT: a filter: all statuses are displayed in the song selection */
    len = istrlen (_("Any Status"));
    if (len > max) {
      max = len;
    }
    uiutilsSpinboxTextSet (&uisongsel->filterstatussel, -1,
        statusGetCount (uisongsel->status),
        max, NULL, uisongselStatusGet);
    uiutilsBoxPackStart (hbox, widget);
  }

  /* favorite */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_FAVORITE)) {
    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (vbox, hbox);

    /* CONTEXT: a filter: select the 'favorite' displayed in the song selection */
    widget = uiutilsCreateColonLabel (_("Favorite"));
    uiutilsBoxPackStart (hbox, widget);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsSpinboxTextCreate (&uisongsel->filterfavoritesel, uisongsel);
    uiutilsSpinboxTextSet (&uisongsel->filterfavoritesel, 0,
        SONG_FAVORITE_MAX, 1, NULL, uisongselFavoriteGet);
    uiutilsBoxPackStart (hbox, widget);
  }

  /* status playable */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_STATUSPLAYABLE)) {
    hbox = uiutilsCreateHorizBox ();
    uiutilsBoxPackStart (vbox, hbox);

    /* CONTEXT: a filter: have a status that are marked as playable */
    widget = uiutilsCreateColonLabel (_("Playable Status"));
    uiutilsBoxPackStart (hbox, widget);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsCreateSwitch (uisongsel->dfltpbflag);
    uiutilsBoxPackStart (hbox, widget);
    uiw->statusPlayable = widget;
  }

  /* the dialog doesn't have any space above the buttons */
  hbox = uiutilsCreateHorizBox ();
  uiutilsBoxPackStart (vbox, hbox);

  widget = uiutilsCreateLabel (" ");
  uiutilsBoxPackStart (hbox, widget);

  g_signal_connect (uiw->filterDialog, "response",
      G_CALLBACK (uisongselFilterResponseHandler), uisongsel);
  logProcEnd (LOG_PROC, "uisongselCreateFilterDialog", "");
}

static void
uisongselFilterResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  uisongselgtk_t  *uiw;
  uisongsel_t   *uisongsel = udata;
  const char    *searchstr;
  int           idx;
  int           nval;
  gint          x, y;

  logProcBegin (LOG_PROC, "uisongselFilterResponseHandler");

  uiw = uisongsel->uiWidgetData;

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      gtk_window_get_position (GTK_WINDOW (uiw->filterDialog), &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      songfilterReset (uisongsel->songfilter);
      uiw->filterDialog = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      gtk_window_get_position (GTK_WINDOW (uiw->filterDialog), &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      songfilterReset (uisongsel->songfilter);
      gtk_widget_hide (uiw->filterDialog);
      break;
    }
    case GTK_RESPONSE_APPLY: {
      uiutilsDropDownSelectionSetNum (&uisongsel->dancesel, uisongsel->danceIdx);
      break;
    }
    case RESPONSE_RESET: {
      songfilterReset (uisongsel->songfilter);
      uisongsel->danceIdx = -1;
      uiutilsDropDownSelectionSetNum (&uisongsel->dancesel, -1);
      uisongselInitFilterDisplay (uisongsel);
      if (uiw->statusPlayable != NULL) {
        uiutilsSwitchSetValue (uiw->statusPlayable, uisongsel->dfltpbflag);
      }
      break;
    }
  }

  /* search : always active */
  searchstr = uiutilsEntryGetValue (&uisongsel->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating : always active */
  idx = uiutilsSpinboxTextGetValue (&uisongsel->filterratingsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_RATING);
  }

  /* dance level */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_DANCELEVEL)) {
    idx = uiutilsSpinboxTextGetValue (&uisongsel->filterlevelsel);
    if (idx >= 0) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW, idx);
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW);
      songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH);
    }
  }

  /* status */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_STATUS)) {
    idx = uiutilsSpinboxTextGetValue (&uisongsel->filterstatussel);
    if (idx >= 0) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_STATUS, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_STATUS);
    }
  }

  /* favorite */
  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_FAVORITE)) {
    idx = uiutilsSpinboxTextGetValue (&uisongsel->filterfavoritesel);
    if (idx != SONG_FAVORITE_NONE) {
      songfilterSetNum (uisongsel->songfilter, SONG_FILTER_FAVORITE, idx);
    } else {
      songfilterClear (uisongsel->songfilter, SONG_FILTER_FAVORITE);
    }
  }

  if (nlistGetNum (uisongsel->filterDisplaySel, FILTER_DISP_STATUSPLAYABLE)) {
    nval = gtk_switch_get_active (GTK_SWITCH (uiw->statusPlayable));
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

  idx = uiutilsDropDownSelectionGet (&uisongsel->sortbysel, path);
  uisongselSortBySelect (uisongsel, idx);
}

static void
uisongselGenreSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiutilsDropDownSelectionGet (&uisongsel->filtergenresel, path);
  uisongselGenreSelect (uisongsel, idx);
}

static void
uisongselDanceSelectSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  idx = uiutilsDropDownSelectionGet (&uisongsel->filterdancesel, path);
  uisongselDanceSelect (uisongsel, idx);
}

