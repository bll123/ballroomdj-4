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
#include "bdjvarsdf.h"
#include "conn.h"
#include "dance.h"
#include "genre.h"
#include "musicdb.h"
#include "musicq.h"
#include "pathbld.h"
#include "progstate.h"
#include "rating.h"
#include "songfilter.h"
#include "sortopt.h"
#include "tagdef.h"
#include "uisongsel.h"
#include "uiutils.h"

enum {
  RESPONSE_NONE,
  RESPONSE_RESET,
};

enum {
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
static void uisongselSortBySelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselCreateSortByList (uisongsel_t *uisongsel);
static void uisongselGenreSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static void uisongselCreateGenreList (uisongsel_t *uisongsel);

uisongsel_t *
uisongselInit (progstate_t *progstate, conn_t *conn)
{
  uisongsel_t    *uisongsel;

  uisongsel = malloc (sizeof (uisongsel_t));
  assert (uisongsel != NULL);
  uisongsel->progstate = progstate;
  uisongsel->conn = conn;
  uisongsel->maxRows = 0;
  uisongsel->idxStart = 0;
  uisongsel->createRowProcessFlag = false;
  uisongsel->createRowFlag = false;
  uisongsel->dfilterCount = (double) dbCount ();
  uisongsel->lastTreeSize = 0;
  uisongsel->lastStepIncrement = 0.0;
  uiutilsDropDownInit (&uisongsel->dancesel);
  uiutilsDropDownInit (&uisongsel->sortbysel);
  uiutilsDropDownInit (&uisongsel->filterdancesel);
  uiutilsDropDownInit (&uisongsel->filtergenresel);
  uiutilsEntryInit (&uisongsel->searchentry, 30, 100);
  uisongsel->songfilter = songfilterAlloc (SONG_FILTER_FOR_PLAYBACK);
  uisongsel->search [0] = '\0';
  songfilterSetSort (uisongsel->songfilter, "TITLE"); // ### fix later

  uisongsel->filterDialog = NULL;

  return uisongsel;
}

void
uisongselFree (uisongsel_t *uisongsel)
{
  if (uisongsel != NULL) {
    if (uisongsel->songfilter != NULL) {
      songfilterFree (uisongsel->songfilter);
    }
    uiutilsDropDownFree (&uisongsel->dancesel);
    uiutilsDropDownFree (&uisongsel->sortbysel);
    uiutilsDropDownFree (&uisongsel->filterdancesel);
    uiutilsDropDownFree (&uisongsel->filtergenresel);
    free (uisongsel);
  }
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
  GValue            gvalue = G_VALUE_INIT;

  g_value_init (&gvalue, G_TYPE_INT);
  g_value_set_int (&gvalue, PANGO_ELLIPSIZE_END);

  uisongsel->parentwin = parentwin;

  uisongsel->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uisongsel->vbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (uisongsel->vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uisongsel->vbox), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uisongsel->vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Queue"), NULL,
      uisongselQueueProcess, uisongsel);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  widget = uiutilsDropDownCreate (parentwin,
      _("Filter by Dance"), uisongselFilterDanceProcess,
      &uisongsel->dancesel, uisongsel);
  uiutilsCreateDanceList (&uisongsel->dancesel);
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Filters"), NULL,
      uisongselFilterDialog, uisongsel);
  gtk_box_pack_end (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_vexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uisongsel->vbox), GTK_WIDGET (hbox),
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
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (widget),
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
  g_signal_connect (uisongsel->songselTree, "size-allocate",
      G_CALLBACK (uisongselProcessTreeSize), uisongsel);
  g_signal_connect (uisongsel->songselTree, "row-activated",
      G_CALLBACK (uisongselRowSelected), uisongsel);
  g_signal_connect (uisongsel->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uisongsel->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_DANCE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, _("Dance"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_TITLE, NULL);
  /* if set to autosize, the column disappears on a resize */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_fixed_width (column, 400);
  gtk_tree_view_column_set_expand (column, TRUE);
  g_object_set_property (G_OBJECT (renderer), "ellipsize", &gvalue);
  gtk_tree_view_column_set_title (column, _("Title"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", SONGSEL_COL_ARTIST, NULL);
  gtk_tree_view_column_set_fixed_width (column, 250);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  g_object_set_property (G_OBJECT (renderer), "ellipsize", &gvalue);
  gtk_tree_view_column_set_title (column, _("Artist"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 0.5, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "markup", SONGSEL_COL_FAVORITE,
      "foreground", SONGSEL_COL_FAV_COLOR, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, "\xE2\x98\x85");
  gtk_tree_view_append_column (GTK_TREE_VIEW (uisongsel->songselTree), column);
  uisongsel->favColumn = column;

  uisongselInitializeStore (uisongsel);
  uisongselCreateRows (uisongsel);

//  g_object_set (gtk_widget_get_settings (uisongsel->songselTree),
//      "gtk-enable-animations", FALSE, NULL);

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

  store = gtk_list_store_new (SONGSEL_COL_MAX,
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uisongsel->songselTree), GTK_TREE_MODEL (store));
  uisongsel->createRowProcessFlag = true;
  uisongsel->createRowFlag = true;
}

static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  GtkTreeModel      *model = NULL;
  GtkTreeIter       iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));
  /* for now, some number that should be large enough */
  for (int i = 0; i < 50; ++i) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  }

  /* initial song filter process */
  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  uisongselPopulateData (uisongsel);
}

static void
uisongselClearData (uisongsel_t *uisongsel)
{
  GtkTreeModel        * model = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uisongsel->songselTree));
  gtk_list_store_clear (GTK_LIST_STORE (model));
  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
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
        danceStr = danceGetData (dances, danceIdx, DANCE_DANCE);
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
        snprintf (tbuff, sizeof (tbuff), "<span color=\"%s\">%s</span>",
            color, favorite->dispStr);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            SONGSEL_COL_IDX, idx,
            SONGSEL_COL_SORTIDX, idx,
            SONGSEL_COL_DBIDX, dbidx,
            SONGSEL_COL_DANCE, danceStr,
            SONGSEL_COL_ARTIST, songGetData (song, TAG_ARTIST),
            SONGSEL_COL_TITLE, songGetData (song, TAG_TITLE),
            SONGSEL_COL_FAV_COLOR, color,
            SONGSEL_COL_FAVORITE, tbuff,
            -1);
      }
    }

    ++idx;
    ++count;
  }
}

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uisongsel_t   *uisongsel = udata;
  GtkAdjustment *adjustment;
  double        ps, si;


  if (allocation->height != uisongsel->lastTreeSize) {
    if (allocation->height < 200) {
      return;
    }

    /* get the page size and the step increment from the scrolled window */
    /* scrollbar; this will allow us to determine how many rows are */
    /* currently displayed */
    adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uisongsel->songselTree));
    ps = gtk_adjustment_get_page_size (adjustment);
    si = gtk_adjustment_get_step_increment (adjustment);
    if (si <= 0.05) {
      return;
    }
    if (uisongsel->lastStepIncrement == 0.0) {
      /* save the original step increment for use in calculations later */
      /* the current step increment has been adjusted for the current */
      /* number of rows that are displayed */
      uisongsel->lastStepIncrement = si;
    }

    uisongsel->maxRows = (int) (ps / uisongsel->lastStepIncrement);

    /* force a redraw */
    gtk_adjustment_set_value (adjustment, 0.0);
    g_signal_emit_by_name (GTK_RANGE (uisongsel->songselScrollbar),
        "change-value", NULL, 0.0, uisongsel, NULL);

    adjustment = gtk_range_get_adjustment (GTK_RANGE (uisongsel->songselScrollbar));
    /* the step increment does not work correctly with smooth scrolling */
    gtk_adjustment_set_step_increment (adjustment, 4.0);
    gtk_adjustment_set_page_increment (adjustment, (double) uisongsel->maxRows);
    gtk_adjustment_set_page_size (adjustment, (double) uisongsel->maxRows);

    uisongsel->lastTreeSize = allocation->height;

    uisongselPopulateData (uisongsel);

    g_signal_emit_by_name (GTK_RANGE (uisongsel->songselScrollbar),
        "change-value", NULL, (double) uisongsel->idxStart, uisongsel, NULL);
  }
}

static gboolean
uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata)
{
  uisongsel_t   *uisongsel = udata;
  double        start;

  if (value < 0 || value > uisongsel->dfilterCount) {
    return TRUE;
  }

  start = floor (value);
  if (start > uisongsel->dfilterCount - uisongsel->maxRows) {
    start = uisongsel->dfilterCount - uisongsel->maxRows;
  }
  uisongsel->idxStart = (ssize_t) start;

  uisongselPopulateData (uisongsel);
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

  if (column != uisongsel->favColumn) {
    return;
  }

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);
  song = dbGetByIdx ((ssize_t) dbidx);
  songChangeFavorite (song);
  // ## TODO song data must be saved to the database.
  uisongselPopulateData (uisongsel);
}

static gboolean
uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event, gpointer udata)
{
  uisongsel_t   *uisongsel = udata;

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

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uisongsel->songselTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  /* for the moment, this is hard-coded at position 4 */
  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%ld", MUSICQ_CURRENT,
      MSG_ARGS_RS, 4, MSG_ARGS_RS, dbidx);
  connSendMessage (uisongsel->conn, ROUTE_MAIN,
      MSG_MUSICQ_INSERT, tbuff);

  return;
}

static void
uisongselFilterDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uisongsel_t *uisongsel = udata;
  ssize_t     idx;
  ilist_t     *danceList;

  idx = uiutilsDropDownSelectionGet (&uisongsel->dancesel, path);
  if (idx >= 0) {
    danceList = ilistAlloc ("songsel-filter-dance", LIST_ORDERED, NULL);
    /* any value will do; only interested in the dance index at this point */
    ilistSetNum (danceList, idx, 0, 0);
    songfilterSetData (uisongsel->songfilter, SONG_FILTER_DANCE, danceList);
  }
  uisongsel->dfilterCount = (double) songfilterProcess (uisongsel->songfilter);
  uisongsel->idxStart = 0;
  uisongselClearData (uisongsel);
  uisongselPopulateData (uisongsel);
  return;
}

static void
uisongselFilterDialog (GtkButton *b, gpointer udata)
{
  uisongsel_t       * uisongsel = udata;

  if (uisongsel->filterDialog == NULL) {
    GtkWidget *content;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkSizeGroup    *sgA;
    GtkSizeGroup    *sgB;


    sgA = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    sgB = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

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

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Sort by"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, GTK_WIDGET (widget));

// need to initialize the button/drop-down to the current selection
    widget = uiutilsComboboxCreate (uisongsel->filterDialog,
        "", uisongselSortBySelect, &uisongsel->sortbysel, uisongsel);
    uisongselCreateSortByList (uisongsel);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Search"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);

    widget = uiutilsEntryCreate (&uisongsel->searchentry);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgB, widget);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Genre"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);

/* ### fix callback */
    widget = uiutilsComboboxCreate (uisongsel->filterDialog,
        _("Select Genre"), uisongselGenreSelect, &uisongsel->filtergenresel,
        uisongsel);
    uisongselCreateGenreList (uisongsel);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Dance"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);

/* ### fix callback */
    widget = uiutilsComboboxCreate (uisongsel->filterDialog,
        _("Select Dance"), NULL,
        &uisongsel->filterdancesel, uisongsel);
    uiutilsCreateDanceList (&uisongsel->filterdancesel);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgB, widget);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Dance Rating"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);

//    widget = uiutilsSpinboxCreate (uisongsel->filterDialog,
//        "", NULL,
//        &uisongsel->filterrating, uisongsel);
//    uisongselCreateRatingList (&uisongsel->filterrating);
//    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
//    gtk_size_group_add_widget (sgB, widget);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Dance Level"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    assert (hbox != NULL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    widget = uiutilsCreateColonLabel (_("Status"));
    gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sgA, widget);
  }

  gtk_widget_show_all (uisongsel->filterDialog);
}

static void
uisongselSortBySelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
}

static void
uisongselCreateSortByList (uisongsel_t *uisongsel)
{
  slist_t           *sortoptlist;
  sortopt_t         *sortopt;

  sortopt = bdjvarsdfGet (BDJVDF_SORT_OPT);
  sortoptlist = sortoptGetList (sortopt);
  uiutilsDropDownSetList (&uisongsel->sortbysel, sortoptlist);
}

static void
uisongselGenreSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
}

static void
uisongselCreateGenreList (uisongsel_t *uisongsel)
{
  slist_t   *genrelist;
  genre_t   *genre;

  genre = bdjvarsdfGet (BDJVDF_GENRES);
  genrelist = genreGetList (genre);
  uiutilsDropDownSetNumList (&uisongsel->filtergenresel, genrelist);
}

