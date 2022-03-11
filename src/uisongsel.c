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
#include "conn.h"
#include "dance.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "musicq.h"
#include "pathbld.h"
#include "progstate.h"
#include "rating.h"
#include "songfilter.h"
#include "sortopt.h"
#include "status.h"
#include "tagdef.h"
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
  SONGSEL_COL_DANCE,
  SONGSEL_COL_TITLE,
  SONGSEL_COL_ARTIST,
  SONGSEL_COL_FAV_COLOR,
  SONGSEL_COL_FAVORITE,
  SONGSEL_COL_MAX,
};

#define STORE_ROWS     60

static void uisongselInitializeStore (uisongsel_t *uisongsel);
static void uisongselCreateRows (uisongsel_t *uisongsel);
static void uisongselClearData (uisongsel_t *uisongsel);
static void uisongselPopulateData (uisongsel_t *uisongsel);
static void uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer user_data);
static gboolean uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata);
static void uisongselRowSelected (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);
static gboolean uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event,
    gpointer udata);
static void uisongselQueueProcess (GtkButton *b, gpointer udata);
static void uisongselFilterDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselFilterDialog (GtkButton *b, gpointer udata);
static void uisongselCreateFilterDialog (uisongsel_t *uisongsel);

static void uisongselSortBySelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselCreateSortByList (uisongsel_t *uisongsel);

static void uisongselGenreSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselCreateGenreList (uisongsel_t *uisongsel);

static void uisongselDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselSongfilterSetDance (uisongsel_t *uisongsel, ssize_t idx);

static char * uisongselRatingGet (void *udata, int idx);
static char * uisongselLevelGet (void *udata, int idx);
static char * uisongselStatusGet (void *udata, int idx);
static char * uisongselFavoriteGet (void *udata, int idx);

static void uisongselFilterResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata);
static void uisongselInitFilterDisplay (uisongsel_t *uisongsel);

uisongsel_t *
uisongselInit (progstate_t *progstate, conn_t *conn, nlist_t *options)
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
  uisongsel->maxRows = 0;
  uisongsel->idxStart = 0;
  uisongsel->createRowProcessFlag = false;
  uisongsel->createRowFlag = false;
  uisongsel->dfilterCount = (double) dbCount ();
  uisongsel->lastTreeSize = 0;
  uisongsel->lastRowHeight = 0.0;
  uisongsel->danceIdx = -1;
  uisongsel->options = options;
  uiutilsDropDownInit (&uisongsel->dancesel);
  uiutilsDropDownInit (&uisongsel->sortbysel);
  uiutilsEntryInit (&uisongsel->searchentry, 30, 100);
  uiutilsDropDownInit (&uisongsel->filtergenresel);
  uiutilsDropDownInit (&uisongsel->filterdancesel);
  uiutilsSpinboxInit (&uisongsel->filterratingsel);
  uiutilsSpinboxInit (&uisongsel->filterlevelsel);
  uiutilsSpinboxInit (&uisongsel->filterstatussel);
  uiutilsSpinboxInit (&uisongsel->filterfavoritesel);
  uisongsel->songfilter = songfilterAlloc (SONG_FILTER_FOR_PLAYBACK);
  songfilterSetSort (uisongsel->songfilter,
      nlistGetStr (options, SONGSEL_SORT_BY));

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
    uiutilsSpinboxFree (&uisongsel->filterratingsel);
    uiutilsSpinboxFree (&uisongsel->filterlevelsel);
    uiutilsSpinboxFree (&uisongsel->filterstatussel);
    uiutilsSpinboxFree (&uisongsel->filterfavoritesel);
    free (uisongsel);
  }
  logProcEnd (LOG_PROC, "uisongselFree", "");
}

GtkWidget *
uisongselActivate (uisongsel_t *uisongsel, GtkWidget *parentwin)
{
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *widget;
  GtkTreeSelection  *sel;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkAdjustment     *adjustment;

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

  widget = uiutilsCreateButton (_("Queue"), NULL,
      uisongselQueueProcess, uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  widget = uiutilsComboboxCreate (parentwin,
      "", uisongselFilterDanceProcess,
      &uisongsel->dancesel, uisongsel);
  uiutilsCreateDanceList (&uisongsel->dancesel, _("All Dances"));
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

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

  uisongsel->songselTree = gtk_tree_view_new ();
  assert (uisongsel->songselTree != NULL);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (uisongsel->songselTree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uisongsel->songselTree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uisongsel->songselTree), TRUE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_margin_start (uisongsel->songselTree, 4);
  gtk_widget_set_margin_end (uisongsel->songselTree, 4);
  gtk_widget_set_margin_top (uisongsel->songselTree, 2);
  gtk_widget_set_margin_bottom (uisongsel->songselTree, 4);
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
      G_CALLBACK (uisongselRowSelected), uisongsel);
  g_signal_connect (uisongsel->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uisongsel->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_DANCE,
      "font", SONGSEL_COL_FONT,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, _("Dance"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_TITLE,
      "font", SONGSEL_COL_FONT,
      "ellipsize", SONGSEL_COL_ELLIPSIZE,
      NULL);
  /* if set to autosize, the column disappears on a resize */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_fixed_width (column, 400);
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Title"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_ARTIST,
      "font", SONGSEL_COL_FONT,
      "ellipsize", SONGSEL_COL_ELLIPSIZE,
      NULL);
  gtk_tree_view_column_set_fixed_width (column, 250);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_title (column, _("Artist"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 0.5, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer,
      "markup", SONGSEL_COL_FAVORITE,
      "font", SONGSEL_COL_FONT,
      "foreground", SONGSEL_COL_FAV_COLOR,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, "\xE2\x98\x85");
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);
  uisongsel->favColumn = column;

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
uisongselMainLoop (uisongsel_t *uisongsel)
{
  return;
}

/* internal routines */

static void
uisongselInitializeStore (uisongsel_t *uisongsel)
{
  GtkListStore      *store = NULL;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

  store = gtk_list_store_new (SONGSEL_COL_MAX,
      /* attributes */
      G_TYPE_INT, G_TYPE_STRING,
      /* internal */
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG,
      /* display */
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

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

static void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  GtkTreeModel        * model = NULL;
  GtkTreeIter         iter;
  GtkAdjustment       *adjustment;
  ssize_t             idx;
  int                 count;
  song_t              * song;
  dance_t             * dances;
  ilistidx_t          danceIdx;
  char                * danceStr;
  songfavoriteinfo_t  * favorite;
  char                * color;
  char                tmp [40];
  char                tbuff [100];
  dbidx_t             dbidx;
  char                * listingFont;

  logProcBegin (LOG_PROC, "uisongselPopulateData");
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisongsel->songselScrollbar));
  gtk_adjustment_set_upper (adjustment, uisongsel->dfilterCount);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));

  count = 0;
  idx = uisongsel->idxStart;
  while (count < uisongsel->maxRows) {
    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      song = dbGetByIdx (dbidx);
      if (song != NULL) {
        danceIdx = songGetNum (song, TAG_DANCE);
        danceStr = danceGetStr (dances, danceIdx, DANCE_DANCE);
        favorite = songGetFavoriteData (song);
        color = favorite->color;
        if (strcmp (color, "") == 0) {
          GtkStyleContext   * context;
          GdkRGBA           gcolor;

          context = gtk_widget_get_style_context (uisongsel->songselTree);
          gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &gcolor);
          color = gdk_rgba_to_string (&gcolor);
          snprintf (tmp, sizeof (tmp), "#%02x%02x%02x",
              (int) round (gcolor.red * 255.0),
              (int) round (gcolor.green * 255.0),
              (int) round (gcolor.blue * 255.0));
          color = tmp;
        }
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            SONGSEL_COL_ELLIPSIZE, 1,
            SONGSEL_COL_FONT, listingFont,
            SONGSEL_COL_IDX, idx,
            SONGSEL_COL_SORTIDX, idx,
            SONGSEL_COL_DBIDX, dbidx,
            SONGSEL_COL_DANCE, danceStr,
            SONGSEL_COL_ARTIST, songGetStr (song, TAG_ARTIST),
            SONGSEL_COL_TITLE, songGetStr (song, TAG_TITLE),
            SONGSEL_COL_FAV_COLOR, color,
            SONGSEL_COL_FAVORITE, favorite->spanStr,
            -1);
      }
    }

    ++idx;
    ++count;
  }
  logProcEnd (LOG_PROC, "uisongselPopulateData", "");
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
uisongselRowSelected (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  int               count;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  gulong            dbidx;
  song_t            * song;

  logProcBegin (LOG_PROC, "uisongselRowSelected");

  if (column != uisongsel->favColumn) {
    logProcEnd (LOG_PROC, "uisongselRowSelected", "not-fav-col");
    return;
  }

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselRowSelected", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);
  song = dbGetByIdx ((ssize_t) dbidx);
  songChangeFavorite (song);
  // ## TODO song data must be saved to the database.
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: favorite changed");
  uisongselPopulateData (uisongsel);
  logProcEnd (LOG_PROC, "uisongselRowSelected", "");
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
uisongselQueueProcess (GtkButton *b, gpointer udata)
{
  uisongsel_t       * uisongsel = udata;
  GtkTreeSelection  * sel;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  int               count;
  gulong            dbidx;
  char              tbuff [MAXPATHLEN];
  ssize_t           insloc;

  logProcBegin (LOG_PROC, "uisongselQueueProcess");

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselQueueProcess", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  insloc = bdjoptGetNum (OPT_P_INSERT_LOCATION);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zd%c%ld", MUSICQ_CURRENT,
      MSG_ARGS_RS, insloc, MSG_ARGS_RS, dbidx);
  connSendMessage (uisongsel->conn, ROUTE_MAIN,
      MSG_MUSICQ_INSERT, tbuff);

  logProcEnd (LOG_PROC, "uisongselQueueProcess", "");
  return;
}

static void
uisongselFilterDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  logProcBegin (LOG_PROC, "uisongselFilterDanceProcess");

  idx = uiutilsDropDownSelectionGet (&uisongsel->dancesel, path);
  uisongselSongfilterSetDance (uisongsel, idx);
  uiutilsDropDownSelectionSetNum (&uisongsel->filterdancesel, idx);
  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  uisongsel->idxStart = 0;
  uisongselClearData (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: filter by dance");
  uisongselPopulateData (uisongsel);
  logProcEnd (LOG_PROC, "uisongselFilterDanceProcess", "");
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
      _("Filter Songs"),
      GTK_WINDOW (uisongsel->parentwin),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      _("Close"),
      GTK_RESPONSE_CLOSE,
      _("Reset"),
      RESPONSE_RESET,
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

  widget = uiutilsCreateColonLabel (_("Sort by"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselSortBySelect, &uisongsel->sortbysel, uisongsel);
  uisongselCreateSortByList (uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* search */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

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

  widget = uiutilsCreateColonLabel (_("Genre"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselGenreSelect,
      &uisongsel->filtergenresel, uisongsel);
  uisongselCreateGenreList (uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* dance */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Dance"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsComboboxCreate (uisongsel->filterDialog,
      "", uisongselDanceSelect,
      &uisongsel->filterdancesel, uisongsel);
  uiutilsCreateDanceList (&uisongsel->filterdancesel, _("All Dances"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* rating */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Dance Rating"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterratingsel, uisongsel);
  max = ratingGetMaxWidth (uisongsel->ratings);
  len = istrlen (_("All Ratings"));
  if (len > max) {
    max = len;
  }
  uiutilsSpinboxSet (&uisongsel->filterratingsel, -1,
      ratingGetCount (uisongsel->ratings),
      max, uisongselRatingGet);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* level */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Dance Level"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterlevelsel, uisongsel);
  max = levelGetMaxWidth (uisongsel->levels);
  len = istrlen (_("All Levels"));
  if (len > max) {
    max = len;
  }
  uiutilsSpinboxSet (&uisongsel->filterlevelsel, -1,
      levelGetCount (uisongsel->levels),
      max, uisongselLevelGet);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* status */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Status"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterstatussel, uisongsel);
  max = statusGetMaxWidth (uisongsel->status);
  len = istrlen (_("Any Status"));
  if (len > max) {
    max = len;
  }
  uiutilsSpinboxSet (&uisongsel->filterstatussel, -1,
      statusGetCount (uisongsel->status),
      max, uisongselStatusGet);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* favorite */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateColonLabel (_("Favorite"));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  widget = uiutilsSpinboxTextCreate (&uisongsel->filterfavoritesel, uisongsel);
  uiutilsSpinboxSet (&uisongsel->filterfavoritesel, 0,
      SONG_FAVORITE_MAX, 3, uisongselFavoriteGet);
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
uisongselSortBySelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  logProcBegin (LOG_PROC, "uisongselSortBySelect");

  idx = uiutilsDropDownSelectionGet (&uisongsel->sortbysel, path);
  if (idx >= 0) {
    songfilterSetSort (uisongsel->songfilter,
        uisongsel->sortbysel.strSelection);
    nlistSetStr (uisongsel->options, SONGSEL_SORT_BY,
        uisongsel->sortbysel.strSelection);
  }
  logProcEnd (LOG_PROC, "uisongselSortBySelect", "");
}

static void
uisongselCreateSortByList (uisongsel_t *uisongsel)
{
  slist_t           *sortoptlist;
  sortopt_t         *sortopt;

  logProcBegin (LOG_PROC, "uisongselCreateSortByList");

  sortopt = bdjvarsdfGet (BDJVDF_SORT_OPT);
  sortoptlist = sortoptGetList (sortopt);
  uiutilsDropDownSetList (&uisongsel->sortbysel, sortoptlist, NULL);
  logProcEnd (LOG_PROC, "uisongselCreateSortByList", "");
}

static void
uisongselGenreSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  logProcBegin (LOG_PROC, "uisongselGenreSelect");

  idx = uiutilsDropDownSelectionGet (&uisongsel->filtergenresel, path);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_GENRE, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_GENRE);
  }
  logProcEnd (LOG_PROC, "uisongselGenreSelect", "");
}

static void
uisongselCreateGenreList (uisongsel_t *uisongsel)
{
  slist_t   *genrelist;
  genre_t   *genre;

  logProcBegin (LOG_PROC, "uisongselCreateGenreList");

  genre = bdjvarsdfGet (BDJVDF_GENRES);
  genrelist = genreGetList (genre);
  uiutilsDropDownSetNumList (&uisongsel->filtergenresel, genrelist,
      _("All Genres"));
  logProcEnd (LOG_PROC, "uisongselCreateGenreList", "");
}


static void
uisongselDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;

  logProcBegin (LOG_PROC, "uisongselDanceSelect");

  idx = uiutilsDropDownSelectionGet (&uisongsel->filterdancesel, path);
  uisongsel->danceIdx = idx;
  uisongselSongfilterSetDance (uisongsel, idx);
  logProcEnd (LOG_PROC, "uisongselDanceSelect", "");
}

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

static char *
uisongselRatingGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselRatingGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselRatingGet", "all");
    return _("All Ratings");
  }
  logProcEnd (LOG_PROC, "uisongselRatingGet", "");
  return ratingGetRating (uisongsel->ratings, idx);
}

static char *
uisongselLevelGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselLevelGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselLevelGet", "all");
    return _("All Levels");
  }
  logProcEnd (LOG_PROC, "uisongselLevelGet", "");
  return levelGetLevel (uisongsel->levels, idx);
}

static char *
uisongselStatusGet (void *udata, int idx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin (LOG_PROC, "uisongselStatusGet");

  if (idx == -1) {
    logProcEnd (LOG_PROC, "uisongselStatusGet", "any");
    return _("Any Status");
  }
  logProcEnd (LOG_PROC, "uisongselStatusGet", "");
  return statusGetStatus (uisongsel->status, idx);
}

static char *
uisongselFavoriteGet (void *udata, int idx)
{
  uisongsel_t         *uisongsel = udata;
  char                tbuff [100];
  songfavoriteinfo_t  *favorite;

  logProcBegin (LOG_PROC, "uisongselFavoriteGet");

  favorite = songGetFavorite (idx);
  if (strcmp (favorite->color, "") != 0) {
    snprintf (tbuff, sizeof (tbuff),
        "spinbutton { color: %s; } ", favorite->color);
    uiutilsSetCss (uisongsel->filterfavoritesel.spinbox, tbuff);
  } else {
    /* how to remove css??? */
    /* this will not work for light colored themes */
    snprintf (tbuff, sizeof (tbuff),
        "spinbutton { color: white; } ");
    uiutilsSetCss (uisongsel->filterfavoritesel.spinbox, tbuff);
  }
  logProcEnd (LOG_PROC, "uisongselFavoriteGet", "");
  return favorite->dispStr;
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

  idx = uiutilsSpinboxGetValue (&uisongsel->filterratingsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_RATING);
  }

  idx = uiutilsSpinboxGetValue (&uisongsel->filterlevelsel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW, idx);
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_LOW);
    songfilterClear (uisongsel->songfilter, SONG_FILTER_LEVEL_HIGH);
  }

  idx = uiutilsSpinboxGetValue (&uisongsel->filterstatussel);
  if (idx >= 0) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_STATUS, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_STATUS);
  }

  idx = uiutilsSpinboxGetValue (&uisongsel->filterfavoritesel);
  if (idx != SONG_FAVORITE_NONE) {
    songfilterSetNum (uisongsel->songfilter, SONG_FILTER_FAVORITE, idx);
  } else {
    songfilterClear (uisongsel->songfilter, SONG_FILTER_FAVORITE);
  }

  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  uisongsel->idxStart = 0;
  uisongselClearData (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: response handler");
  uisongselPopulateData (uisongsel);
  logProcEnd (LOG_PROC, "uisongselFilterResponseHandler", "");
}

static void
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
  uiutilsSpinboxSetValue (&uisongsel->filterratingsel, -1);
  uiutilsSpinboxSetValue (&uisongsel->filterlevelsel, -1);
  uiutilsSpinboxSetValue (&uisongsel->filterstatussel, -1);
  uiutilsSpinboxSetValue (&uisongsel->filterfavoritesel, 0);
  logProcEnd (LOG_PROC, "uisongselInitFilterDisplay", "");
}
