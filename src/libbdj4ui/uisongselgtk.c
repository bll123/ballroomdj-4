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
#include "bdj4ui.h"
#include "conn.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "songfilter.h"
#include "uisongsel.h"
#include "ui.h"
#include "uiutils.h"

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

/* for callbacks */
enum {
  SONGSEL_CALLBACK_SELECT,
  SONGSEL_CALLBACK_QUEUE,
  SONGSEL_CALLBACK_PLAY,
  SONGSEL_CALLBACK_FILTER,
  SONGSEL_CALLBACK_MAX,
};

typedef struct uisongselgtk {
  UICallback          callbacks [SONGSEL_CALLBACK_MAX];
  UIWidget            *parentwin;
  UIWidget            vbox;
  GtkWidget           *songselTree;
  GtkTreeSelection    *sel;
  GtkWidget           *songselScrollbar;
  GtkEventController  *scrollController;
  GtkTreeViewColumn   *favColumn;
  GtkWidget           *scrolledwin;
  /* other data */
  int               lastTreeSize;
  double            lastRowHeight;
  int               maxRows;
  nlist_t           *selectedList;
  UICallback        *newselcb;
  bool              controlPressed : 1;
  bool              shiftPressed : 1;
  bool              inscroll : 1;
} uisongselgtk_t;

static bool uisongselQueueProcessQueueCallback (void *udata);
static bool uisongselQueueProcessPlayCallback (void *udata);
static void uisongselQueueProcessHandler (void *udata, musicqidx_t mqidx);
static void uisongselInitializeStore (uisongsel_t *uisongsel);
static void uisongselCreateRows (uisongsel_t *uisongsel);

static void uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);

/* scrolling/key event/selection change handling */
static void uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer user_data);
static gboolean uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata);
static gboolean uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event,
    gpointer udata);
static void uisongselClearAllSelections (uisongsel_t *uisongsel);
static void uisongselClearSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static gboolean uisongselKeyEvent (GtkWidget *w, GdkEventKey *event, gpointer udata);
static void uisongselSelChanged (GtkTreeSelection *sel, gpointer udata);
static void uisongselProcessSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;

  uiw = malloc (sizeof (uisongselgtk_t));
  uiutilsUIWidgetInit (&uiw->vbox);
  uiw->songselTree = NULL;
  uiw->sel = NULL;
  uiw->songselScrollbar = NULL;
  uiw->scrollController = NULL;
  uiw->favColumn = NULL;
  uiw->lastTreeSize = 0;
  uiw->lastRowHeight = 0.0;
  uiw->maxRows = 0;
  uiw->controlPressed = false;
  uiw->shiftPressed = false;
  uiw->inscroll = false;
  uiw->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
  uiw->newselcb = NULL;

  uisongsel->uiWidgetData = uiw;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  if (uisongsel->uiWidgetData != NULL) {
    uisongselgtk_t    *uiw;

    uiw = uisongsel->uiWidgetData;
    if (uiw->selectedList != NULL) {
      nlistFree (uiw->selectedList);
    }
    free (uiw);
    uisongsel->uiWidgetData = NULL;
  }
}

UIWidget *
uisongselBuildUI (uisongsel_t *uisongsel, UIWidget *parentwin)
{
  uisongselgtk_t    *uiw;
  UIWidget          uiwidget;
  UIWidget          hbox;
  UIWidget          vbox;
  GtkWidget         *widget;
  GtkAdjustment     *adjustment;
  slist_t           *sellist;
  char              tbuff [200];
  double            tupper;

  logProcBegin (LOG_PROC, "uisongselBuildUI");

  uiw = uisongsel->uiWidgetData;
  uisongsel->windowp = parentwin;

  uiCreateVertBox (&uiw->vbox);
  uiWidgetExpandHoriz (&uiw->vbox);
  uiWidgetExpandVert (&uiw->vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    /* CONTEXT: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CALLBACK_SELECT],
        uisongselQueueProcessSelectCallback, uisongsel);
    uiCreateButton (&uiwidget,
        &uiw->callbacks [SONGSEL_CALLBACK_SELECT], tbuff, NULL);
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_REQUEST ||
      uisongsel->dispselType == DISP_SEL_MM) {
    if (uisongsel->dispselType == DISP_SEL_REQUEST) {
      /* CONTEXT: queue a song to be played */
      strlcpy (tbuff, _("Queue"), sizeof (tbuff));
      uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CALLBACK_QUEUE],
          uisongselQueueProcessQueueCallback, uisongsel);
      uiCreateButton (&uiwidget,
          &uiw->callbacks [SONGSEL_CALLBACK_QUEUE], tbuff, NULL);
    }
    if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
        uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
        uisongsel->dispselType == DISP_SEL_MM) {
      /* CONTEXT: play the selected songs */
      strlcpy (tbuff, _("Play"), sizeof (tbuff));
      uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CALLBACK_PLAY],
          uisongselQueueProcessPlayCallback, uisongsel);
      uiCreateButton (&uiwidget,
          &uiw->callbacks [SONGSEL_CALLBACK_PLAY], tbuff, NULL);
    }
    uiBoxPackStart (&hbox, &uiwidget);
  }

  widget = uiComboboxCreate (parentwin->widget,
      "", uisongselFilterDanceSignal,
      &uisongsel->dancesel, uisongsel);
  /* CONTEXT: filter: all dances are selected */
  uiutilsCreateDanceList (&uisongsel->dancesel, _("All Dances"));
  uiBoxPackEndUW (&hbox, widget);

  uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CALLBACK_FILTER],
      uisongselFilterDialog, uisongsel);
  uiCreateButton (&uiwidget,
      &uiw->callbacks [SONGSEL_CALLBACK_FILTER],
      /* CONTEXT: a button that starts the filters (narrowing down song selections) dialog */
      _("Filters"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&uiw->vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiBoxPackStartExpand (&hbox, &vbox);

  tupper = uisongsel->dfilterCount;
  adjustment = gtk_adjustment_new (0.0, 0.0, tupper, 1.0, 10.0, 10.0);
  uiw->songselScrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  assert (uiw->songselScrollbar != NULL);
  uiWidgetExpandVertW (uiw->songselScrollbar);
  uiSetCss (uiw->songselScrollbar,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  uiBoxPackEndUW (&hbox, uiw->songselScrollbar);
  g_signal_connect (uiw->songselScrollbar, "change-value",
      G_CALLBACK (uisongselScroll), uisongsel);

  widget = uiCreateScrolledWindowW (400);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
  uiWidgetExpandHorizW (widget);
  uiBoxPackStartExpandUW (&vbox, widget);
  uiw->scrolledwin = widget;

  uiw->songselTree = uiCreateTreeView ();
  assert (uiw->songselTree != NULL);
  uiWidgetAlignHorizFillW (uiw->songselTree);
  uiWidgetExpandHorizW (uiw->songselTree);
  uiWidgetExpandVertW (uiw->songselTree);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiw->songselTree), TRUE);
  /* for song list editing, multiple selections are valid */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    uiTreeViewAllowMultiple (uiw->songselTree);
  }
  g_signal_connect (uiw->songselTree, "key-press-event",
      G_CALLBACK (uisongselKeyEvent), uisongsel);
  g_signal_connect (uiw->songselTree, "key-release-event",
      G_CALLBACK (uisongselKeyEvent), uisongsel);
  uiw->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uiw->songselTree));
  g_signal_connect ((GtkWidget *) uiw->sel, "changed",
      G_CALLBACK (uisongselSelChanged), uisongsel);

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uiw->songselTree));
  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);
  uiw->scrollController =
      gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uiw->songselTree, GDK_SCROLL_MASK);
  uiBoxPackInWindowWW (widget, uiw->songselTree);
  g_signal_connect (uiw->songselTree, "row-activated",
      G_CALLBACK (uisongselCheckFavChgSignal), uisongsel);
  g_signal_connect (uiw->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  uiw->favColumn = uiAddDisplayColumns (
      uiw->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uiw->maxRows = STORE_ROWS;
  uisongselPopulateData (uisongsel);

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "populate: initial");
  uisongselPopulateData (uisongsel);

  uiDropDownSelectionSetNum (&uisongsel->dancesel, -1);

  g_signal_connect (uiw->songselTree, "size-allocate",
      G_CALLBACK (uisongselProcessTreeSize), uisongsel);

  logProcEnd (LOG_PROC, "uisongselBuildUI", "");
  return &uiw->vbox;
}

void
uisongselClearData (uisongsel_t *uisongsel)
{
  uisongselgtk_t  * uiw;
  GtkTreeModel    * model = NULL;
  GtkAdjustment   *adjustment;

  logProcBegin (LOG_PROC, "uisongselClearData");

  uiw = uisongsel->uiWidgetData;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));
  gtk_list_store_clear (GTK_LIST_STORE (model));
  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
  gtk_adjustment_set_value (adjustment, 0.0);
  uisongsel->idxStart = 0;
  uisongsel->oldIdxStart = 0;
  nlistFree (uiw->selectedList);
  uiw->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkTreeModel        * model = NULL;
  GtkTreeIter         iter;
  GtkAdjustment       * adjustment;
  ssize_t             idx;
  int                 count;
  song_t              * song;
  songfavoriteinfo_t  * favorite;
  char                * color;
  char                tmp [40];
  char                tbuff [100];
  dbidx_t             dbidx;
  char                * listingFont;
  slist_t             * sellist;
  double              tupper;

  logProcBegin (LOG_PROC, "uisongselPopulateData");
  uiw = uisongsel->uiWidgetData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);

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
          uiGetForegroundColor (uiw->songselTree, tmp, sizeof (tmp));
          color = tmp;
        }

        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            SONGSEL_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
            SONGSEL_COL_FONT, listingFont,
            SONGSEL_COL_IDX, (glong) idx,
            SONGSEL_COL_SORTIDX, (glong) idx,
            SONGSEL_COL_DBIDX, (glong) dbidx,
            SONGSEL_COL_FAV_COLOR, color,
            -1);

        sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
        uiSetDisplayColumns (
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
  char                tmp [40];

  uiw = uisongsel->uiWidgetData;

  if (strcmp (color, "") != 0) {
    uiSpinboxSetColor (uisongsel->filterfavoritesel, color);
  } else {
    uiGetForegroundColor (uiw->songselTree, tmp, sizeof (tmp));
    uiSpinboxSetColor (uisongsel->filterfavoritesel, tmp);
  }
}

bool
uisongselQueueProcessSelectCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;
  musicqidx_t       mqidx;

  /* queue to the song list */
  mqidx = MUSICQ_A;
  uisongselQueueProcessHandler (uisongsel, mqidx);
  /* don't clear the selected list or the displayed selections */
  /* it's confusing */
  return UICB_CONT;
}

void
uisongselSetSelectionCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  uisongselgtk_t  * uiw;

  if (uisongsel == NULL) {
    return;
  }
  if (uisongsel->uiWidgetData == NULL) {
    return;
  }
  uiw = uisongsel->uiWidgetData;
  uiw->newselcb = uicb;
}

/* internal routines */

static bool
uisongselQueueProcessQueueCallback (void *udata)
{
  musicqidx_t       mqidx;

  mqidx = MUSICQ_CURRENT;
  uisongselQueueProcessHandler (udata, mqidx);
  return UICB_CONT;
}

static bool
uisongselQueueProcessPlayCallback (void *udata)
{
  uisongsel_t     * uisongsel = udata;
  uisongselgtk_t  * uiw;
  musicqidx_t     mqidx;
  char            tmp [20];

  uiw = uisongsel->uiWidgetData;

  mqidx = MUSICQ_B;
  /* clear the queue; start index is 1 */
  snprintf (tmp, sizeof (tmp), "%d%c%d", mqidx, MSG_ARGS_RS, 1);
  connSendMessage (uisongsel->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tmp);
  /* clear any playing song via main */
  connSendMessage (uisongsel->conn, ROUTE_MAIN, MSG_CMD_NEXTSONG, NULL);
  /* queue to the hidden music queue */
  uisongselQueueProcessHandler (udata, mqidx);
  return UICB_CONT;
}

static void
uisongselQueueProcessHandler (void *udata, musicqidx_t mqidx)
{
  uisongsel_t     * uisongsel = udata;
  uisongselgtk_t  * uiw;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  logProcBegin (LOG_PROC, "uisongselQueueProcessHandler");
  uiw = uisongsel->uiWidgetData;
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (uiw->selectedList, &iteridx)) >= 0) {
    uisongselQueueProcess (uisongsel, dbidx, mqidx);
  }
  logProcEnd (LOG_PROC, "uisongselQueueProcessHandler", "");
  return;
}

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
  songselstoretypes [colcount++] = G_TYPE_LONG,
  songselstoretypes [colcount++] = G_TYPE_LONG,
  songselstoretypes [colcount++] = G_TYPE_LONG,
  songselstoretypes [colcount++] = G_TYPE_STRING,

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  songselstoretypes = uiAddDisplayTypes (songselstoretypes, sellist, &colcount);
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
uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uisongselgtk_t    * uiw;
  uisongsel_t       * uisongsel = udata;
  int               count;
  GtkTreeModel      * model = NULL;
  GtkTreeIter       iter;
  long              dbidx;


  logProcBegin (LOG_PROC, "uisongselCheckFavChgSignal");

  uiw = uisongsel->uiWidgetData;
  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "not-fav-col");
    return;
  }

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
  gtk_tree_model_get (model, &iter, SONGSEL_COL_DBIDX, &dbidx, -1);

  uisongselChangeFavorite (uisongsel, dbidx);

  logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "");
}

/* scrolling/key event/selection change */

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uisongselgtk_t  *uiw;
  uisongsel_t   *uisongsel = udata;
  slist_t           *sellist;
  GtkAdjustment *adjustment;
  double        ps;

  logProcBegin (LOG_PROC, "uisongselProcessTreeSize");

  uiw = uisongsel->uiWidgetData;
  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);

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

    adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
    /* the step increment does not work correctly with smooth scrolling */
    /* and it appears there's no easy way to turn smooth scrolling off */
    gtk_adjustment_set_step_increment (adjustment, 4.0);
    gtk_adjustment_set_page_increment (adjustment, (double) uiw->maxRows);
    gtk_adjustment_set_page_size (adjustment, (double) uiw->maxRows);

    uiw->lastTreeSize = allocation->height;

    logMsg (LOG_DBG, LOG_SONGSEL, "populate: tree size change");
    uisongselPopulateData (uisongsel);

    uisongselScroll (GTK_RANGE (uiw->songselScrollbar), GTK_SCROLL_JUMP,
        (double) uisongsel->idxStart, uisongsel);
    /* neither queue_draw nor queue_resize on the tree */
    /* do not help with the redraw issue */
  }
  logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "");
}

static gboolean
uisongselScroll (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;
  GtkAdjustment   *adjustment;
  double          start;
  double          tval;
  nlistidx_t      idx;
  nlistidx_t      iteridx;

  logProcBegin (LOG_PROC, "uisongselScroll");

  uiw = uisongsel->uiWidgetData;

  start = floor (value);
  if (start < 0) {
    start = 0;
  }
  tval = uisongsel->dfilterCount - (double) uiw->maxRows;
  if (start > tval) {
    start = tval;
  }
  uisongsel->idxStart = (nlistidx_t) start;

  uiw->inscroll = true;

  /* clear the current selections */
  uisongselClearAllSelections (uisongsel);

  logMsg (LOG_DBG, LOG_SONGSEL, "populate: scroll");
  uisongselPopulateData (uisongsel);
  uisongsel->oldIdxStart = uisongsel->idxStart;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
  gtk_adjustment_set_value (adjustment, value);

  /* set the selections based on the saved selection list */
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
    GtkTreePath *path;
    char        tmp [40];

    if (idx >= uisongsel->idxStart &&
        idx < uisongsel->idxStart + uiw->maxRows) {
      snprintf (tmp, sizeof (tmp), "%ld", idx - uisongsel->idxStart);
      path = gtk_tree_path_new_from_string (tmp);
      if (path != NULL) {
        gtk_tree_selection_select_path (uiw->sel, path);
        gtk_tree_path_free (path);
      }
    }
  }

  uiw->inscroll = false;
  logProcEnd (LOG_PROC, "uisongselScroll", "");
  return FALSE;
}

static gboolean
uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event, gpointer udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;
  nlistidx_t      tval;

  logProcBegin (LOG_PROC, "uisongselScrollEvent");

  uiw = uisongsel->uiWidgetData;
  uisongsel->oldIdxStart = uisongsel->idxStart;

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

  if (uisongsel->idxStart < 0) {
    uisongsel->idxStart = 0;
  }

  tval = (nlistidx_t) uisongsel->dfilterCount - uiw->maxRows;
  if (uisongsel->idxStart > tval) {
    uisongsel->idxStart = tval;
  }

  uisongselScroll (GTK_RANGE (uiw->songselScrollbar), GTK_SCROLL_JUMP,
      (double) uisongsel->idxStart, uisongsel);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
  return TRUE;
}

static void
uisongselClearAllSelections (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  gtk_tree_selection_selected_foreach (uiw->sel,
      uisongselClearSelection, uisongsel);
}

static void
uisongselClearSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  uisongselgtk_t    *uiw;

  uiw = uisongsel->uiWidgetData;
  gtk_tree_selection_unselect_iter (uiw->sel, iter);
}

static gboolean
uisongselKeyEvent (GtkWidget *w, GdkEventKey *event, gpointer udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;
  guint           keyval;

  uiw = uisongsel->uiWidgetData;

  gdk_event_get_keyval ((GdkEvent *) event, &keyval);

  if (event->type == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)) {
    uiw->shiftPressed = true;
  }
  if (event->type == GDK_KEY_PRESS &&
      (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R)) {
    uiw->controlPressed = true;
  }
  if (event->type == GDK_KEY_RELEASE &&
      (event->state & GDK_SHIFT_MASK)) {
    uiw->shiftPressed = false;
  }
  if (event->type == GDK_KEY_RELEASE &&
      (event->state & GDK_CONTROL_MASK)) {
    uiw->controlPressed = false;
  }
  return FALSE; // do not stop event handling
}

static void
uisongselSelChanged (GtkTreeSelection *sel, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  uisongselgtk_t    *uiw;
  nlist_t           *tlist;
  nlistidx_t        idx;
  nlistidx_t        iteridx;

  uiw = uisongsel->uiWidgetData;

  if (uiw->inscroll) {
    return;
  }

  /* if neither the control key nor the shift key are pressed */
  /* then this is a new selection and not a modification */
  tlist = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
  if (! uiw->controlPressed && ! uiw->shiftPressed) {
    nlistFree (uiw->selectedList);
  } else {
    nlistStartIterator (uiw->selectedList, &iteridx);
    while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
      if (idx < uisongsel->idxStart ||
          idx > uisongsel->idxStart + uiw->maxRows - 1) {
        nlistSetNum (tlist, idx, nlistGetNum (uiw->selectedList, idx));
      }
    }
    nlistFree (uiw->selectedList);
  }
  uiw->selectedList = tlist;

  gtk_tree_selection_selected_foreach (sel,
      uisongselProcessSelection, uisongsel);
}

static void
uisongselProcessSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  uisongselgtk_t    *uiw;
  glong             idx;
  glong             dbidx;

  uiw = uisongsel->uiWidgetData;

  gtk_tree_model_get (model, iter, SONGSEL_COL_IDX, &idx, -1);
  gtk_tree_model_get (model, iter, SONGSEL_COL_DBIDX, &dbidx, -1);
  nlistSetNum (uiw->selectedList, idx, dbidx);

  if (uiw->newselcb != NULL) {
    uiutilsCallbackLongHandler (uiw->newselcb, dbidx);
  }
}

