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
#include "log.h"
#include "nlist.h"
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

GtkWidget *
uisongselActivate (uisongsel_t *uisongsel, GtkWidget *parentwin)
{
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *widget;
  GtkAdjustment     *adjustment;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselActivate");

  uisongsel->parentwin = parentwin;

  uisongsel->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uisongsel->vbox != NULL);
  gtk_widget_set_hexpand (uisongsel->vbox, TRUE);
  gtk_widget_set_vexpand (uisongsel->vbox, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uisongsel->vbox), hbox,
      FALSE, FALSE, 0);

  /* CONTEXT: queue a song to be played */
  widget = uiutilsCreateButton (_("Queue"), NULL,
      uisongselQueueProcessSignal, uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  widget = uiutilsComboboxCreate (parentwin,
      "", uisongselFilterDanceSignal,
      &uisongsel->dancesel, uisongsel);
  /* CONTEXT: filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->dancesel, _("All Dances"));
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: a button that starts the filters (narrowing down song selections) dialog */
  widget = uiutilsCreateButton (_("Filters"), NULL,
      uisongselFilterDialog, uisongsel);
  gtk_box_pack_end (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uisongsel->vbox), hbox,
      TRUE, TRUE, 0);

  adjustment = gtk_adjustment_new (0.0, 0.0, uisongsel->dfilterCount, 1.0, 10.0, 10.0);
  uisongsel->songselScrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  assert (uisongsel->songselScrollbar != NULL);
  gtk_widget_set_vexpand (uisongsel->songselScrollbar, TRUE);
  uiutilsSetCss (uisongsel->songselScrollbar,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  gtk_box_pack_end (GTK_BOX (hbox), uisongsel->songselScrollbar,
      FALSE, FALSE, 0);
  g_signal_connect (uisongsel->songselScrollbar, "change-value",
      G_CALLBACK (uisongselScroll), uisongsel);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (vbox != NULL);
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
  gtk_box_pack_start (GTK_BOX (vbox), widget,
      FALSE, FALSE, 0);

  uisongsel->songselTree = uiutilsCreateTreeView ();
  assert (uisongsel->songselTree != NULL);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uisongsel->songselTree), TRUE);
  gtk_widget_set_halign (uisongsel->songselTree, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (uisongsel->songselTree, TRUE);
  gtk_widget_set_vexpand (uisongsel->songselTree, TRUE);

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uisongsel->songselTree));
  gtk_adjustment_set_upper (adjustment, uisongsel->dfilterCount);
  uisongsel->scrollController =
      gtk_event_controller_scroll_new (uisongsel->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uisongsel->songselTree, GDK_SCROLL_MASK);
  gtk_container_add (GTK_CONTAINER (widget), uisongsel->songselTree);
  g_signal_connect (uisongsel->songselTree, "row-activated",
      G_CALLBACK (uisongselCheckFavChgSignal), uisongsel);
  g_signal_connect (uisongsel->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uisongsel->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, DISP_SEL_REQUEST);
  uisongsel->favColumn = uiutilsAddDisplayColumns (
      uisongsel->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uisongsel->maxRows = STORE_ROWS;
  uisongselPopulateData (uisongsel);

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: initial");
  uisongselPopulateData (uisongsel);

  uiutilsDropDownSelectionSetNum (&uisongsel->dancesel, -1);

  g_signal_connect (uisongsel->songselTree, "size-allocate",
      G_CALLBACK (uisongselProcessTreeSize), uisongsel);

  logProcEnd (LOG_PROC, "uisongselActivate", "");
  return uisongsel->vbox;
}

void
uisongselClearData (uisongsel_t *uisongsel)
{
  GtkTreeModel        * model = NULL;

  logProcBegin (LOG_PROC, "uisongselClearData");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));
  gtk_list_store_clear (GTK_LIST_STORE (model));
  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
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
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisongsel->songselScrollbar));
  gtk_adjustment_set_upper (adjustment, uisongsel->dfilterCount);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));

  count = 0;
  idx = uisongsel->idxStart;
  while (count < uisongsel->maxRows) {
    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      song = dbGetByIdx (dbidx);
      if (song != NULL) {
        favorite = songGetFavoriteData (song);
        color = favorite->color;
        if (strcmp (color, "") == 0) {
          uiutilsGetForegroundColor (uisongsel->songselTree, tmp, sizeof (tmp));
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

        sellist = dispselGetList (uisongsel->dispsel, DISP_SEL_REQUEST);
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

/* internal routines */

static void
uisongselInitializeStore (uisongsel_t *uisongsel)
{
  GtkListStore      *store = NULL;
  GType             *songselstoretypes;
  int               colcount = 0;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

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

  sellist = dispselGetList (uisongsel->dispsel, DISP_SEL_REQUEST);
  songselstoretypes = uiutilsAddDisplayTypes (songselstoretypes, sellist, &colcount);
  store = gtk_list_store_newv (colcount, songselstoretypes);
  assert (store != NULL);
  free (songselstoretypes);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uisongsel->songselTree), GTK_TREE_MODEL (store));
  uisongsel->createRowProcessFlag = true;
  uisongsel->createRowFlag = true;
  logProcEnd (LOG_PROC, "uisongselInitializeStore", "");
}

static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  GtkTreeModel      *model = NULL;
  GtkTreeIter       iter;

  logProcBegin (LOG_PROC, "uisongselCreateRows");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));
  /* enough pre-allocated rows are needed so that if the windows is */
  /* maximized and the font size is not large, enough rows are available */
  /* to be displayed */
  for (int i = 0; i < STORE_ROWS; ++i) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  }

  /* initial song filter process */
  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  logProcEnd (LOG_PROC, "uisongselCreateRows", "");
}

static void
uisongselQueueProcessSignal (GtkButton *b, gpointer udata)
{
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  int               count;
  gulong            dbidx;

  logProcBegin (LOG_PROC, "uisongselQueueProcessSignal");

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselQueueProcess", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  uisongselQueueProcess (uisongsel, dbidx);

  logProcEnd (LOG_PROC, "uisongselQueueProcessSignal", "");
  return;
}

static void
uisongselFilterDialog (GtkButton *b, gpointer udata)
{
  uisongsel_t * uisongsel = udata;
  gint        x, y;

  logProcBegin (LOG_PROC, "uisongselFilterDialog");

  if (uisongsel->filterDialog == NULL) {
    uisongselCreateFilterDialog (uisongsel);
  }

  uisongselInitFilterDisplay (uisongsel);
  gtk_widget_show_all (uisongsel->filterDialog);

  x = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (uisongsel->filterDialog), x, y);
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
  uisongsel_t   *uisongsel = udata;
  double        start;

  logProcBegin (LOG_PROC, "uisongselScroll");

  if (value < 0 || value > uisongsel->dfilterCount) {
    logProcEnd (LOG_PROC, "uisongselScroll", "bad-value");
    return TRUE;
  }

  start = floor (value);
  if (start > uisongsel->dfilterCount - uisongsel->maxRows) {
    start = uisongsel->dfilterCount - uisongsel->maxRows;
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
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  int               count;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  gulong            dbidx;


  logProcBegin (LOG_PROC, "uisongselCheckFavChgSignal");

  if (column != uisongsel->favColumn) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "not-fav-col");
    return;
  }

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
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
  uisongsel_t   *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselScrollEvent");

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
  g_signal_emit_by_name (GTK_RANGE (uisongsel->songselScrollbar),
      "change-value", NULL, (double) uisongsel->idxStart, uisongsel, NULL);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
  return TRUE;
}

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uisongsel_t   *uisongsel = udata;
  GtkAdjustment *adjustment;
  double        ps;

  logProcBegin (LOG_PROC, "uisongselProcessTreeSize");

  if (allocation->height != uisongsel->lastTreeSize) {
    if (allocation->height < 200) {
      logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "small-alloc-height");
      return;
    }

    /* the step increment is useless */
    /* the page-size and upper can be used to determine */
    /* how many rows can be displayed */
    adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uisongsel->songselTree));
    ps = gtk_adjustment_get_page_size (adjustment);


    if (uisongsel->lastRowHeight == 0.0) {
      double      u, hpr;

      u = gtk_adjustment_get_upper (adjustment);
      hpr = u / STORE_ROWS;
      /* save the original step increment for use in calculations later */
      /* the current step increment has been adjusted for the current */
      /* number of rows that are displayed */
      uisongsel->lastRowHeight = hpr;
    }

    uisongsel->maxRows = (int) (ps / uisongsel->lastRowHeight);
    ++uisongsel->maxRows;
    logMsg (LOG_DBG, LOG_IMPORTANT, "max-rows:%d", uisongsel->maxRows);

    /* force a redraw */
    gtk_adjustment_set_value (adjustment, 0.0);
    g_signal_emit_by_name (GTK_RANGE (uisongsel->songselScrollbar),
        "change-value", NULL, 0.0, uisongsel, NULL);

    adjustment = gtk_range_get_adjustment (GTK_RANGE (uisongsel->songselScrollbar));
    /* the step increment does not work correctly with smooth scrolling */
    /* and it appears there's no easy way to turn smooth scrolling off */
    gtk_adjustment_set_step_increment (adjustment, 4.0);
    gtk_adjustment_set_page_increment (adjustment, (double) uisongsel->maxRows);
    gtk_adjustment_set_page_size (adjustment, (double) uisongsel->maxRows);

    uisongsel->lastTreeSize = allocation->height;

    logMsg (LOG_DBG, LOG_SONGSEL, "populate: tree size change");
    uisongselPopulateData (uisongsel);

    g_signal_emit_by_name (GTK_RANGE (uisongsel->songselScrollbar),
        "change-value", NULL, (double) uisongsel->idxStart, uisongsel, NULL);
  }
  logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "");
}

static void
uisongselCreateFilterDialog (uisongsel_t *uisongsel)
{
  GtkWidget     *content;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *widget;
  GtkSizeGroup  *sgA;
  int           max;
  int           len;

  logProcBegin (LOG_PROC, "uisongselCreateFilterDialog");


  sgA = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  uisongsel->filterDialog = gtk_dialog_new_with_buttons (
      /* CONTEXT: title for the filter dialog */
      _("Filter Songs"),
      GTK_WINDOW (uisongsel->parentwin),
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

  content = gtk_dialog_get_content_area (GTK_DIALOG (uisongsel->filterDialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (vbox != NULL);
  gtk_widget_set_hexpand (vbox, FALSE);
  gtk_widget_set_vexpand (vbox, FALSE);
  gtk_container_add (GTK_CONTAINER (content), vbox);

  /* sort-by */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the method to sort the song selection display */
  widget = uiutilsCreateColonLabel (_("Sort by"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselSortBySelectHandler, &uisongsel->sortbysel, uisongsel);
  uisongselCreateSortByList (uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* search */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: filter the song selection with a search for text */
  widget = uiutilsCreateColonLabel (_("Search"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsEntryCreate (&uisongsel->searchentry);
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the genre displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Genre"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselGenreSelectHandler,
      &uisongsel->filtergenresel, uisongsel);
  uisongselCreateGenreList (uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* dance */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the dance displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Dance"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselDanceSelectSignal,
      &uisongsel->filterdancesel, uisongsel);
  /* CONTEXT: a filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->filterdancesel, _("All Dances"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* rating */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the dance rating displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Dance Rating"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
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
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* level */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the dance level displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Dance Level"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
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
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* status */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the status displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Status"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
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
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* favorite */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* CONTEXT: a filter: select the 'favorite' displayed in the song selection */
  widget = uiutilsCreateColonLabel (_("Favorite"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterfavoritesel, uisongsel);
  uiutilsSpinboxTextSet (&uisongsel->filterfavoritesel, 0,
      SONG_FAVORITE_MAX, 1, NULL, uisongselFavoriteGet);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* the dialog doesn't have any space above the buttons */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel ("");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  g_signal_connect (uisongsel->filterDialog, "response",
      G_CALLBACK (uisongselFilterResponseHandler), uisongsel);
  logProcEnd (LOG_PROC, "uisongselCreateFilterDialog", "");
}

static void
uisongselFilterResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  uisongsel_t   *uisongsel = udata;
  const char    *searchstr;
  int           idx;
  gint          x, y;

  logProcBegin (LOG_PROC, "uisongselFilterResponseHandler");

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      gtk_window_get_position (GTK_WINDOW (uisongsel->filterDialog), &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      songfilterReset (uisongsel->songfilter);
      uisongsel->filterDialog = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      gtk_window_get_position (GTK_WINDOW (uisongsel->filterDialog), &x, &y);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_X, x);
      nlistSetNum (uisongsel->options, SONGSEL_FILTER_POSITION_Y, y);

      songfilterReset (uisongsel->songfilter);
      gtk_widget_hide (uisongsel->filterDialog);
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
      break;
    }
  }

  searchstr = uiutilsEntryGetValue (&uisongsel->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_SEARCH);
  }

  idx = uiutilsSpinboxTextGetValue (&uisongsel->filterratingsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_RATING);
  }

  idx = uiutilsSpinboxTextGetValue (&uisongsel->filterlevelsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW, idx);
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW);
    songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH);
  }

  idx = uiutilsSpinboxTextGetValue (&uisongsel->filterstatussel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_STATUS, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_STATUS);
  }

  idx = uiutilsSpinboxTextGetValue (&uisongsel->filterfavoritesel);
  if (idx != SONG_FAVORITE_NONE) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_FAVORITE, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_FAVORITE);
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

