/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "colorutils.h"
#include "conn.h"
#include "log.h"
#include "mdebug.h"
#include "musicq.h"
#include "nlist.h"
#include "songfav.h"
#include "songfilter.h"
#include "tmutil.h"
#include "uidance.h"
#include "uifavorite.h"
#include "ui.h"
#include "uisongfilter.h"
#include "uisong.h"
#include "uisongsel.h"
#include "uitreedisp.h"

enum {
  SONGSEL_COL_ELLIPSIZE,
  SONGSEL_COL_FONT,
  SONGSEL_COL_IDX,
  SONGSEL_COL_SORTIDX,
  SONGSEL_COL_DBIDX,
  SONGSEL_COL_MARK_MARKUP,
  SONGSEL_COL_SAMESONG_MARKUP,
  SONGSEL_COL_MAX,
};

enum {
  UISONGSEL_FIRST,
  UISONGSEL_NEXT,
  UISONGSEL_PREVIOUS,
  UISONGSEL_DIR_NONE,
  UISONGSEL_MOVE_KEY,
  UISONGSEL_MOVE_SE,
  UISONGSEL_PLAY,
  UISONGSEL_QUEUE,
  UISONGSEL_SCROLL_NORMAL,
  UISONGSEL_SCROLL_FORCE,
};

enum {
  STORE_ROWS = 60,
  TREE_DOUBLE_CLICK_TIME = 250,
};

/* for callbacks */
enum {
  SONGSEL_CB_SELECT,
  SONGSEL_CB_QUEUE,
  SONGSEL_CB_PLAY,
  SONGSEL_CB_FILTER,
  SONGSEL_CB_EDIT_LOCAL,
  SONGSEL_CB_DANCE_SEL,
  SONGSEL_CB_KEYB,
  SONGSEL_CB_MAX,
};

enum {
  SONGSEL_BUTTON_EDIT,
  SONGSEL_BUTTON_FILTER,
  SONGSEL_BUTTON_PLAY,
  SONGSEL_BUTTON_QUEUE,
  SONGSEL_BUTTON_SELECT,
  SONGSEL_BUTTON_MAX,
};

#define MARK_DISPLAY "\xe2\x96\x8B"  // left five-eights block

typedef struct uisongselgtk {
  callback_t          *callbacks [SONGSEL_CB_MAX];
  uiwidget_t          *parentwin;
  uiwidget_t          vbox;
  uitree_t            *songselTree;
  GtkTreeSelection    *sel;
  uiwidget_t          songselScrollbar;
  GtkEventController  *scrollController;
  GtkTreeViewColumn   *favColumn;
  uiwidget_t          scrolledwin;
  uibutton_t          *buttons [SONGSEL_BUTTON_MAX];
  uiwidget_t          reqQueueLabel;
  uikey_t             *uikey;
  /* other data */
  int               lastTreeSize;
  double            lastRowHeight;
  double            upperLimit;         // for windows work-around
  int               maxRows;
  nlist_t           *selectedBackup;
  nlist_t           *selectedList;
  nlistidx_t        selectListIter;
  nlistidx_t        selectListKey;
  GtkTreeIter       currIter;
  int               *typelist;
  int               colcount;            // for the display type callback
  const char        *markcolor;
  /* for shift-click */
  nlistidx_t        shiftfirstidx;
  nlistidx_t        shiftlastidx;
  /* for double-click checks */
  dbidx_t           lastRowDBIdx;
  mstime_t          lastRowCheck;
  bool              controlPressed : 1;
  bool              shiftPressed : 1;
  bool              inscroll : 1;
} ss_internal_t;


static void uisongselClearSelections (uisongsel_t *uisongsel);
static bool uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart, int scrollflag, int dir);
static bool uisongselQueueCallback (void *udata);
static void uisongselQueueHandler (uisongsel_t *uisongsel, musicqidx_t mqidx, int action);
static void uisongselInitializeStore (uisongsel_t *uisongsel);
static void uisongselInitializeStoreCallback (int type, void *udata);
static void uisongselCreateRows (uisongsel_t *uisongsel);
static void uisongselProcessSongFilter (uisongsel_t *uisongsel);

static void uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path, GtkTreeViewColumn* column, gpointer udata);

static void uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation, gpointer user_data);
static gboolean uisongselScroll (GtkRange *range, GtkScrollType scrolltype, gdouble value, gpointer udata);
static gboolean uisongselScrollEvent (GtkWidget* tv, GdkEventScroll *event, gpointer udata);
static void uisongselProcessScroll (uisongsel_t *uisongsel, int dir, int lines);
static void uisongselClearSelection (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void uisongselClearSingleSelection (uisongsel_t *uisongsel);
static bool uisongselKeyEvent (void *udata);
static void uisongselSelectionChgCallback (GtkTreeSelection *sel, gpointer udata);
static void uisongselProcessSelection (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void uisongselPopulateDataCallback (int col, long num, const char *str, void *udata);

static void uisongselMoveSelection (void *udata, int where, int lines, int moveflag);
static void uisongselGetIter (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);

static bool uisongselUIDanceSelectCallback (void *udata, long idx, int count);
static bool uisongselSongEditCallback (void *udata);

static int * uiTreeViewAddDisplayType (int *types, int type, int col);
static int * uiAppendType (int *types, int ncol, int type);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = mdmalloc (sizeof (ss_internal_t));
  uiutilsUIWidgetInit (&uiw->vbox);
  uiw->songselTree = NULL;
  uiw->sel = NULL;
  uiutilsUIWidgetInit (&uiw->songselScrollbar);
  uiw->scrollController = NULL;
  uiw->favColumn = NULL;
  uiw->lastTreeSize = 0;
  uiw->lastRowHeight = 0.0;
  uiw->upperLimit = 0.0;
  uiw->maxRows = 0;
  uiw->controlPressed = false;
  uiw->shiftPressed = false;
  uiw->inscroll = false;
  uiw->selectedBackup = NULL;
  uiw->selectedList = nlistAlloc ("selected-list-a", LIST_ORDERED, NULL);
  nlistStartIterator (uiw->selectedList, &uiw->selectListIter);
  uiw->selectListKey = -1;
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    uiw->callbacks [i] = NULL;
  }
  uiw->markcolor = bdjoptGetStr (OPT_P_UI_MARK_COL);
  for (int i = 0; i < SONGSEL_BUTTON_MAX; ++i) {
    uiw->buttons [i] = NULL;
  }
  uiw->lastRowDBIdx = -1;
  mstimeset (&uiw->lastRowCheck, 0);

  uiw->uikey = uiKeyAlloc ();
  uiw->callbacks [SONGSEL_CB_KEYB] = callbackInit (
      uisongselKeyEvent, uisongsel, NULL);

  uisongsel->uiWidgetData = uiw;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  if (uisongsel->uiWidgetData != NULL) {
    ss_internal_t    *uiw;

    uiw = uisongsel->uiWidgetData;
    uiKeyFree (uiw->uikey);
    nlistFree (uiw->selectedBackup);
    nlistFree (uiw->selectedList);
    for (int i = 0; i < SONGSEL_BUTTON_MAX; ++i) {
      uiButtonFree (uiw->buttons [i]);
    }
    uiTreeViewFree (uiw->songselTree);
    for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
      callbackFree (uiw->callbacks [i]);
    }
    mdfree (uiw);
    uisongsel->uiWidgetData = NULL;
  }
}

UIWidget *
uisongselBuildUI (uisongsel_t *uisongsel, uiwidget_t *parentwin)
{
  ss_internal_t    *uiw;
  uibutton_t        *uibutton;
  uiwidget_t        uiwidget;
  uiwidget_t        *uiwidgetp;
  uiwidget_t        *uitreewidgetp;
  uiwidget_t        hbox;
  uiwidget_t        vbox;
  GtkAdjustment     *adjustment;
  slist_t           *sellist;
  char              tbuff [200];
  double            tupper;
  int               col;

  logProcBegin (LOG_PROC, "uisongselBuildUI");

  uiw = uisongsel->uiWidgetData;
  uisongsel->windowp = parentwin;

  uiCreateVertBox (&uiw->vbox);
  uiWidgetExpandHoriz (&uiw->vbox);
  uiWidgetExpandVert (&uiw->vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  /* The ez song selection does not need a select button, as it has */
  /* the left-arrow button.  Saves real estate. */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL) {
    /* CONTEXT: song-selection: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    uiw->callbacks [SONGSEL_CB_SELECT] = callbackInit (
        uisongselSelectCallback, uisongsel, "songsel: select");
    uibutton = uiCreateButton (
        uiw->callbacks [SONGSEL_CB_SELECT], tbuff, NULL);
    uiw->buttons [SONGSEL_BUTTON_SELECT] = uibutton;
    uiwidgetp = uiButtonGetWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    uiw->callbacks [SONGSEL_CB_EDIT_LOCAL] = callbackInit (
        uisongselSongEditCallback, uisongsel, "songsel: edit");
    uibutton = uiCreateButton (uiw->callbacks [SONGSEL_CB_EDIT_LOCAL],
        /* CONTEXT: song-selection: edit the selected song */
        _("Edit"), "button_edit");
    uiw->buttons [SONGSEL_BUTTON_EDIT] = uibutton;
    uiwidgetp = uiButtonGetWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uisongsel->dispselType == DISP_SEL_REQUEST) {
    /* CONTEXT: (verb) song-selection: queue a song to be played */
    strlcpy (tbuff, _("Queue"), sizeof (tbuff));
    uiw->callbacks [SONGSEL_CB_QUEUE] = callbackInit (
        uisongselQueueCallback, uisongsel, "songsel: queue");
    uibutton = uiCreateButton (
        uiw->callbacks [SONGSEL_CB_QUEUE], tbuff, NULL);
    uiw->buttons [SONGSEL_BUTTON_QUEUE] = uibutton;
    uiwidgetp = uiButtonGetWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);

    uiCreateLabel (&uiwidget, "");
    uiWidgetSetClass (&uiwidget, DARKACCENT_CLASS);
    uiBoxPackStart (&hbox, &uiwidget);
    uiutilsUIWidgetCopy (&uiw->reqQueueLabel, &uiwidget);
  }
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    /* CONTEXT: song-selection: play the selected songs */
    strlcpy (tbuff, _("Play"), sizeof (tbuff));
    uiw->callbacks [SONGSEL_CB_PLAY] = callbackInit (
        uisongselPlayCallback, uisongsel, "songsel: play");
    uibutton = uiCreateButton (
        uiw->callbacks [SONGSEL_CB_PLAY], tbuff, NULL);
    uiw->buttons [SONGSEL_BUTTON_PLAY] = uibutton;
    uiwidgetp = uiButtonGetWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  uiw->callbacks [SONGSEL_CB_DANCE_SEL] = callbackInitLongInt (
      uisongselUIDanceSelectCallback, uisongsel);
  uisongsel->uidance = uidanceDropDownCreate (&hbox, parentwin,
      /* CONTEXT: song-selection: filter: all dances are selected */
      UIDANCE_ALL_DANCES, _("All Dances"), UIDANCE_PACK_END, 1);
  uidanceSetCallback (uisongsel->uidance,
      uiw->callbacks [SONGSEL_CB_DANCE_SEL]);

  uiw->callbacks [SONGSEL_CB_FILTER] = callbackInit (
      uisfDialog, uisongsel->uisongfilter, "songsel: filters");
  uibutton = uiCreateButton (
      uiw->callbacks [SONGSEL_CB_FILTER],
      /* CONTEXT: song-selection: a button that starts the filters (narrowing down song selections) dialog */
      _("Filters"), NULL);
  uiw->buttons [SONGSEL_BUTTON_FILTER] = uibutton;
  uiwidgetp = uiButtonGetWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&uiw->vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiBoxPackStartExpand (&hbox, &vbox);

  tupper = uisongsel->dfilterCount;
  uiCreateVerticalScrollbar (&uiw->songselScrollbar, tupper);
  uiBoxPackEnd (&hbox, &uiw->songselScrollbar);
  g_signal_connect (uiw->songselScrollbar.widget, "change-value",
      G_CALLBACK (uisongselScroll), uisongsel);

  uiCreateScrolledWindow (&uiw->scrolledwin, 400);
  uiWindowSetPolicyExternal (&uiw->scrolledwin);
  uiWidgetExpandHoriz (&uiw->scrolledwin);
  uiBoxPackStartExpand (&vbox, &uiw->scrolledwin);

  uiw->songselTree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetWidget (uiw->songselTree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetExpandHoriz (uitreewidgetp);
  uiWidgetExpandVert (uitreewidgetp);
  uiTreeViewEnableHeaders (uiw->songselTree);
  /* for song list editing, multiple selections are valid */
  /* for the music manager, multiple selections are valid to allow */
  /* set/clear of same song marks.  also creates a new selection */
  /* mode for the song editor */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    uiTreeViewSelectSetMode (uiw->songselTree, SELECT_MULTIPLE);
  }
  uiKeySetKeyCallback (uiw->uikey, uitreewidgetp,
      uiw->callbacks [SONGSEL_CB_KEYB]);
  uiw->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uitreewidgetp->widget));

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uitreewidgetp->widget));
  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);
  uiw->scrollController =
      gtk_event_controller_scroll_new (uitreewidgetp->widget,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uitreewidgetp->widget, GDK_SCROLL_MASK);
  uiBoxPackInWindow (&uiw->scrolledwin, uitreewidgetp);
  g_signal_connect (uitreewidgetp->widget, "row-activated",
      G_CALLBACK (uisongselCheckFavChgSignal), uisongsel);
  g_signal_connect (uitreewidgetp->widget, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uitreewidgetp->widget,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);

  /* the mark display is a special case, it always exists */
  col = SONGSEL_COL_MARK_MARKUP;
  if (uisongsel->dispselType == DISP_SEL_MM) {
    col = SONGSEL_COL_SAMESONG_MARKUP;
  }
  uiTreeViewAppendColumn (uiw->songselTree,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_MARKUP, col,
      TREE_COL_TYPE_FONT, SONGSEL_COL_FONT,
      TREE_COL_TYPE_END);

  uitreedispAddDisplayColumns (
      uiw->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  uiw->maxRows = STORE_ROWS;

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: initial", uisongsel->tag);
  uisongselProcessSongFilter (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uisongselPopulateData (uisongsel);

  uidanceSetValue (uisongsel->uidance, -1);

  g_signal_connect ((GtkWidget *) uiw->sel, "changed",
      G_CALLBACK (uisongselSelectionChgCallback), uisongsel);
  g_signal_connect (uitreewidgetp->widget, "size-allocate",
      G_CALLBACK (uisongselProcessTreeSize), uisongsel);

  logProcEnd (LOG_PROC, "uisongselBuildUI", "");
  return &uiw->vbox;
}

void
uisongselClearData (uisongsel_t *uisongsel)
{
  ss_internal_t  * uiw;
  GtkTreeModel    * model = NULL;
  uiwidget_t      * uiwidgetp;

  logProcBegin (LOG_PROC, "uisongselClearData");

  uiw = uisongsel->uiWidgetData;
  uiwidgetp = uiTreeViewGetWidget (uiw->songselTree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  gtk_list_store_clear (GTK_LIST_STORE (model));
  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
  uiScrollbarSetPosition (&uiw->songselScrollbar, 0.0);
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  ss_internal_t  * uiw;
  long            idx;
  int             row;
  song_t          * song;
  dbidx_t         dbidx;
  char            * listingFont;
  slist_t         * sellist;
  double          tupper;
  const char      * sscolor = ""; // "#000000";

  logProcBegin (LOG_PROC, "uisongselPopulateData");

  uiw = uisongsel->uiWidgetData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  /* re-fetch the count, as the songfilter process may not have been */
  /* processed by this instance */
  uisongsel->dfilterCount = (double) songfilterGetCount (uisongsel->songfilter);

  tupper = uisongsel->dfilterCount;
  uiScrollbarSetUpper (&uiw->songselScrollbar, tupper);

  row = 0;
  idx = uisongsel->idxStart;
  while (row < uiw->maxRows) {
    char        colorbuff [200];

    uiTreeViewValueIteratorSet (uiw->songselTree, row);

    dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
    song = NULL;
    if (dbidx >= 0) {
      song = dbGetByIdx (uisongsel->musicdb, dbidx);
    }
    if (song != NULL && (double) row < uisongsel->dfilterCount) {
      *colorbuff = '\0';

      if (uisongsel->dispselType != DISP_SEL_MM &&
          uisongsel->songlistdbidxlist != NULL) {
        /* check and see if the song is in the song list */
        if (nlistGetNum (uisongsel->songlistdbidxlist, dbidx) >= 0) {
          snprintf (colorbuff, sizeof (colorbuff),
              "<span color=\"%s\">%s</span>", uiw->markcolor, MARK_DISPLAY);
        }
      }

      if (uisongsel->dispselType == DISP_SEL_MM) {
        const char *tsscolor;
        tsscolor = samesongGetColorByDBIdx (uisongsel->samesong, dbidx);
        if (tsscolor != NULL) {
          sscolor = tsscolor;
          snprintf (colorbuff, sizeof (colorbuff),
              "<span color=\"%s\">%s</span>", sscolor, MARK_DISPLAY);
        }
      }

      uiTreeViewSetValueEllipsize (uiw->songselTree, SONGSEL_COL_ELLIPSIZE);
      uiTreeViewSetValues (uiw->songselTree,
          SONGSEL_COL_FONT, listingFont,
          SONGSEL_COL_IDX, (treenum_t) idx,
          SONGSEL_COL_SORTIDX, (treenum_t) idx,
          SONGSEL_COL_DBIDX, (treenum_t) dbidx,
          SONGSEL_COL_MARK_MARKUP, colorbuff,
          SONGSEL_COL_SAMESONG_MARKUP, colorbuff,
          TREE_VALUE_END);

      sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
      // uiw->model = model;
      // uiw->iterp = &iter;
      uisongSetDisplayColumns (sellist, song, SONGSEL_COL_MAX,
          uisongselPopulateDataCallback, uisongsel);
    } /* song is not null */

    ++idx;
    ++row;
  }

  uiTreeViewValueIteratorClear (uiw->songselTree);
  logProcEnd (LOG_PROC, "uisongselPopulateData", "");
}

bool
uisongselSelectCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;
  musicqidx_t       mqidx;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel select");
  /* only the song selection and ez song selection have a select button */
  /* queue to the song list */
  mqidx = MUSICQ_SL;
  uisongselQueueHandler (uisongsel, mqidx, UISONGSEL_QUEUE);
  /* don't clear the selected list or the displayed selections */
  /* it's confusing for the user */
  return UICB_CONT;
}

void
uisongselSetDefaultSelection (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;
  int             count;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }

  count = uiTreeViewSelectGetCount (uiw->songselTree);
  if (count < 1) {
    uiTreeViewSelectFirst (uiw->songselTree);
  }

  return;
}

void
uisongselSetSelection (uisongsel_t *uisongsel, long idx)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }
  if (idx < 0) {
    return;
  }

  uiTreeViewSelectSet (uiw->songselTree, idx);
}

void
uisongselSetSelectionOffset (uisongsel_t *uisongsel, long idx)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }
  if (idx < 0) {
    return;
  }

  uisongselScrollSelection (uisongsel, idx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_DIR_NONE);
  idx -= uisongsel->idxStart;

  uiTreeViewSelectSet (uiw->songselTree, idx);
}

bool
uisongselNextSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_NEXT, 1, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

bool
uisongselPreviousSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_PREVIOUS, 1, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

bool
uisongselFirstSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_FIRST, 0, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

long
uisongselGetSelectLocation (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;
  int             count;
  long            loc = -1;
  GtkTreeModel    *model = NULL;
  GtkTreePath     *path;
  char            *pathstr;
  uiwidget_t      *uiwidgetp;

  uiw = uisongsel->uiWidgetData;
  count = uiTreeViewSelectGetCount (uiw->songselTree);
  if (count != 1) {
    return 0;
  }
  gtk_tree_selection_selected_foreach (uiw->sel,
      uisongselGetIter, uisongsel);
  uiwidgetp = uiTreeViewGetWidget (uiw->songselTree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  path = gtk_tree_model_get_path (model, &uiw->currIter);
  mdextalloc (path);
  loc = 0;
  if (path != NULL) {
    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    loc = atol (pathstr);
    mdextfree (path);
    gtk_tree_path_free (path);
    mdfree (pathstr);     // allocated by gtk
  }

  return loc + uisongsel->idxStart;
}

bool
uisongselApplySongFilter (void *udata)
{
  uisongsel_t *uisongsel = udata;

  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;

  /* the call to cleardata() will remove any selections */
  /* afterwards, make sure something is selected */
  uisongselClearData (uisongsel);
  uisongselClearSelections (uisongsel);
  uisongselPopulateData (uisongsel);

  /* the song filter has been processed, the peers need to be populated */

  /* if the song selection is displaying something else, do not */
  /* update the peers */
  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselClearData (uisongsel->peers [i]);
    uisongselClearSelections (uisongsel->peers [i]);
    uisongselPopulateData (uisongsel->peers [i]);
  }

  /* set the selection after the populate is done */

  uisongselSetDefaultSelection (uisongsel);

  return UICB_CONT;
}

/* handles the dance drop-down */
/* when a dance is selected, the song filter must be updated */
/* call danceselectcallback to set all the peer drop-downs */
/* will apply the filter */
void
uisongselDanceSelectHandler (uisongsel_t *uisongsel, ilistidx_t danceIdx)
{
  uisfSetDanceIdx (uisongsel->uisongfilter, danceIdx);
  uisongselDanceSelectCallback (uisongsel, danceIdx);
  uisongselApplySongFilter (uisongsel);
}

/* callback for the song filter when the dance selection is changed */
/* also used by DanceSelectHandler to set the peers dance drop-down */
/* does not apply the filter */
bool
uisongselDanceSelectCallback (void *udata, long danceIdx)
{
  uisongsel_t *uisongsel = udata;

  uidanceSetValue (uisongsel->uidance, danceIdx);

  if (uisongsel->ispeercall) {
    return UICB_CONT;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselDanceSelectCallback (uisongsel->peers [i], danceIdx);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
  return UICB_CONT;
}

void
uisongselSaveSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  if (uiw->selectedBackup != NULL) {
    return;
  }

  uiw->selectedBackup = uiw->selectedList;
  uiw->selectedList = nlistAlloc ("selected-list-save", LIST_ORDERED, NULL);

  if (uisongsel->ispeercall) {
    return;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselSaveSelections (uisongsel->peers [i]);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
}

void
uisongselRestoreSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  if (uiw->selectedBackup == NULL) {
    return;
  }

  nlistFree (uiw->selectedList);
  uiw->selectedList = uiw->selectedBackup;
  uiw->selectedBackup = NULL;
  uisongselScrollSelection (uisongsel, uisongsel->idxStart, UISONGSEL_SCROLL_FORCE, UISONGSEL_DIR_NONE);

  if (uisongsel->ispeercall) {
    return;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselRestoreSelections (uisongsel->peers [i]);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
}

bool
uisongselPlayCallback (void *udata)
{
  musicqidx_t mqidx;
  uisongsel_t *uisongsel = udata;

  /* only song selection, ez song sel and music manager have a play button */
  /* use the hidden manage playback music queue */
  mqidx = MUSICQ_MNG_PB;
  /* the manageui callback clears the queue and plays */
  uisongselQueueHandler (uisongsel, mqidx, UISONGSEL_PLAY);
  return UICB_CONT;
}

void
uisongselSetPlayButtonState (uisongsel_t *uisongsel, int active)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  /* if the player is active, disable the button */
  if (active) {
    uiButtonSetState (uiw->buttons [SONGSEL_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiButtonSetState (uiw->buttons [SONGSEL_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
}

nlist_t *
uisongselGetSelectedList (uisongsel_t *uisongsel)
{
  ss_internal_t  * uiw;
  nlist_t         *tlist;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  uiw = uisongsel->uiWidgetData;
  tlist = nlistAlloc ("selected-list-dbidx", LIST_UNORDERED, NULL);
  nlistSetSize (tlist, nlistGetCount (uiw->selectedList));
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (uiw->selectedList, &iteridx)) >= 0) {
    nlistSetNum (tlist, dbidx, 0);
  }
  nlistSort (tlist);
  return tlist;
}

void
uisongselClearAllSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  gtk_tree_selection_selected_foreach (uiw->sel,
      uisongselClearSelection, uisongsel);
}

double
uisongselGetUpperWorkaround (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  if (uisongsel == NULL) {
    return 0.0;
  }

  uiw = uisongsel->uiWidgetData;
  return uiw->upperLimit;
}

void
uisongselSetRequestLabel (uisongsel_t *uisongsel, const char *txt)
{
  ss_internal_t  *uiw;

  if (uisongsel == NULL) {
    return;
  }

  uiw = uisongsel->uiWidgetData;
  uiLabelSetText (&uiw->reqQueueLabel, txt);
}

/* internal routines */

static void
uisongselClearSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  uisongsel->idxStart = 0;
  nlistFree (uiw->selectedList);
  uiw->selectedList = nlistAlloc ("selected-list-clr", LIST_ORDERED, NULL);
  nlistStartIterator (uiw->selectedList, &uiw->selectListIter);
  uiw->selectListKey = -1;
}

static bool
uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart,
    int scrollflag, int direction)
{
  ss_internal_t  *uiw;
  bool            scrolled = false;
  dbidx_t         oidx;
  bool            idxchanged = false;

  uiw = uisongsel->uiWidgetData;

  oidx = uisongsel->idxStart;

  if (scrollflag == UISONGSEL_SCROLL_NORMAL &&
      idxStart >= oidx &&
      idxStart < oidx + uiw->maxRows) {
    if (direction == UISONGSEL_DIR_NONE) {
      return false;
    }
    if (direction == UISONGSEL_NEXT) {
      if (idxStart < oidx + uiw->maxRows / 2) {
        return false;
      } else {
        idxchanged = true;
        uisongsel->idxStart += 1;
      }
    }
    if (direction == UISONGSEL_PREVIOUS) {
      if (idxStart >= oidx + uiw->maxRows / 2 - 1) {
        return false;
      } else {
        idxchanged = true;
        uisongsel->idxStart -= 1;
      }
    }
  }
  if (! idxchanged) {
    uisongsel->idxStart = idxStart;
  }

  uisongselPopulateData (uisongsel);

  uisongselScroll (GTK_RANGE (uiw->songselScrollbar.widget), GTK_SCROLL_JUMP,
      (double) uisongsel->idxStart, uisongsel);

  scrolled = true;
  if (oidx == uisongsel->idxStart) {
    scrolled = false;
  }
  return scrolled;
}

static bool
uisongselQueueCallback (void *udata)
{
  musicqidx_t mqidx;
  uisongsel_t *uisongsel = udata;

  mqidx = MUSICQ_CURRENT;
  uisongselQueueHandler (uisongsel, mqidx, UISONGSEL_QUEUE);
  return UICB_CONT;
}

static void
uisongselQueueHandler (uisongsel_t *uisongsel, musicqidx_t mqidx, int action)
{
  ss_internal_t  * uiw;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  logProcBegin (LOG_PROC, "uisongselQueueHandler");
  uiw = uisongsel->uiWidgetData;
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (uiw->selectedList, &iteridx)) >= 0) {
    if (action == UISONGSEL_QUEUE) {
      uisongselQueueProcess (uisongsel, dbidx, mqidx);
    }
    if (action == UISONGSEL_PLAY) {
      uisongselPlayProcess (uisongsel, dbidx, mqidx);
    }
    action = UISONGSEL_QUEUE;
  }
  logProcEnd (LOG_PROC, "uisongselQueueHandler", "");
  return;
}

/* count is not used */
static bool
uisongselUIDanceSelectCallback (void *udata, long idx, int count)
{
  uisongsel_t *uisongsel = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: dance select");
  uisongselDanceSelectHandler (uisongsel, idx);
  return UICB_CONT;
}

static void
uisongselInitializeStore (uisongsel_t *uisongsel)
{
  ss_internal_t      * uiw;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

  uiw = uisongsel->uiWidgetData;
  uiw->typelist = mdmalloc (sizeof (int) * SONGSEL_COL_MAX);
  uiw->colcount = 0;
  /* attributes ellipsize/font*/
  uiw->typelist [uiw->colcount++] = TREE_TYPE_ELLIPSIZE;
  uiw->typelist [uiw->colcount++] = TREE_TYPE_STRING;
  /* internal idx/sortidx/dbidx */
  uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
  uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
  uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
  /* fav color/mark color/mark/samesong color */
  uiw->typelist [uiw->colcount++] = TREE_TYPE_STRING;
  uiw->typelist [uiw->colcount++] = TREE_TYPE_STRING;
  if (uiw->colcount != SONGSEL_COL_MAX) {
    fprintf (stderr, "ERR: mismatched SONGSEL_COL_MAX and %d\n", uiw->colcount);
  }

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  uisongAddDisplayTypes (sellist, uisongselInitializeStoreCallback, uisongsel);

  uiTreeViewCreateValueStoreFromList (uiw->songselTree, uiw->colcount, uiw->typelist);
  mdfree (uiw->typelist);

  logProcEnd (LOG_PROC, "uisongselInitializeStore", "");
}

static void
uisongselInitializeStoreCallback (int type, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  uiw->typelist = uiTreeViewAddDisplayType (uiw->typelist, type, uiw->colcount);
  ++uiw->colcount;
}


static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  ss_internal_t    *uiw;

  logProcBegin (LOG_PROC, "uisongselCreateRows");

  uiw = uisongsel->uiWidgetData;
  /* enough pre-allocated rows are needed so that if the windows is */
  /* maximized and the font size is not large, enough rows are available */
  /* to be displayed */
  for (int i = 0; i < STORE_ROWS; ++i) {
    uiTreeViewValueAppend (uiw->songselTree);
  }
  logProcEnd (LOG_PROC, "uisongselCreateRows", "");
}

static void
uisongselProcessSongFilter (uisongsel_t *uisongsel)
{
  /* initial song filter process */
  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
}

static void
uisongselCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uisongsel_t       * uisongsel = udata;
  ss_internal_t    * uiw;

  logProcBegin (LOG_PROC, "uisongselCheckFavChgSignal");

  uiw = uisongsel->uiWidgetData;

  /* double-click processing */
  if (uiw->lastRowDBIdx == uisongsel->lastdbidx &&
      ! mstimeCheck (&uiw->lastRowCheck)) {
    /* double-click in the song selection or ez-song selection adds */
    /* the song to the song list */
    if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
        uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
      uisongselSelectCallback (uisongsel);
    }
    /* double-click in the music manager edits the song */
    if (uisongsel->dispselType == DISP_SEL_MM) {
      uisongselSongEditCallback (uisongsel);
    }
    /* double-click in the request window queues the song */
    if (uisongsel->dispselType == DISP_SEL_REQUEST) {
      uisongselQueueCallback (uisongsel);
    }
  }
  mstimeset (&uiw->lastRowCheck, TREE_DOUBLE_CLICK_TIME);
  uiw->lastRowDBIdx = uisongsel->lastdbidx;

  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "not-fav-col");
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");
  uisongselChangeFavorite (uisongsel, uisongsel->lastdbidx);
  logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "");
}

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  ss_internal_t  *uiw;
  uisongsel_t     *uisongsel = udata;
  GtkAdjustment   *adjustment;
  double          ps;
  double          tmax;

  logProcBegin (LOG_PROC, "uisongselProcessTreeSize");

  uiw = uisongsel->uiWidgetData;

  if (allocation->height != uiw->lastTreeSize) {
    uiwidget_t  *uiwidgetp;

    if (allocation->height < 200) {
      logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "small-alloc-height");
      return;
    }

    /* the step increment is useless */
    /* the page-size and upper can be used to determine */
    /* how many rows can be displayed */
    uiwidgetp = uiTreeViewGetWidget (uiw->songselTree);
    adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uiwidgetp->widget));
    ps = gtk_adjustment_get_page_size (adjustment);

    if (uiw->lastRowHeight == 0.0) {
      double      u, ub, hpr;

      u = gtk_adjustment_get_upper (adjustment);

      /* this is a really gross work-around for a windows gtk problem */
      /* the music manager internal v-scroll adjustment is not set correctly */
      /* use the value from one of the peers instead */
      ub = 0.0;
      for (int i = 0; i < uisongsel->peercount; ++i) {
        if (uisongsel->peers [i] == NULL) {
          continue;
        }
        ub = uisongselGetUpperWorkaround (uisongsel->peers [i]);
        break;
      }
      if (ub > u) {
        u = ub;
      }

      hpr = u / STORE_ROWS;
      /* save the original step increment for use in calculations later */
      /* the current step increment has been adjusted for the current */
      /* number of rows that are displayed */
      uiw->lastRowHeight = hpr;
      uiw->upperLimit = u;
    }

    tmax = ps / uiw->lastRowHeight;
    uiw->maxRows = (int) round (tmax);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s max-rows:%d", uisongsel->tag, uiw->maxRows);

    /* the step increment does not work correctly with smooth scrolling */
    /* and it appears there's no easy way to turn smooth scrolling off */
    uiScrollbarSetStepIncrement (&uiw->songselScrollbar, 4.0);
    uiScrollbarSetPageIncrement (&uiw->songselScrollbar, (double) uiw->maxRows);
    uiScrollbarSetPageSize (&uiw->songselScrollbar, (double) uiw->maxRows);

    uiw->lastTreeSize = allocation->height;

    logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: tree size change", uisongsel->tag);
    uisongselPopulateData (uisongsel);

    uisongselScroll (GTK_RANGE (uiw->songselScrollbar.widget), GTK_SCROLL_JUMP,
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
  ss_internal_t  *uiw;
  double          start;
  double          tval;
  nlistidx_t      idx;
  nlistidx_t      iteridx;

  logProcBegin (LOG_PROC, "uisongselScroll");

  uiw = uisongsel->uiWidgetData;

  start = floor (value);
  if (start < 0.0) {
    start = 0.0;
  }
  tval = uisongsel->dfilterCount - (double) uiw->maxRows;
  if (tval < 0) {
    tval = 0;
  }
  if (start >= tval) {
    start = tval;
  }
  uisongsel->idxStart = (dbidx_t) start;

  uiw->inscroll = true;

  /* clear the current selections */
  uisongselClearAllSelections (uisongsel);

  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: scroll", uisongsel->tag);
  uisongselPopulateData (uisongsel);

  uiScrollbarSetPosition (&uiw->songselScrollbar, value);

  /* set the selections based on the saved selection list */
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
    if (idx >= uisongsel->idxStart &&
        idx < uisongsel->idxStart + uiw->maxRows) {
      uiTreeViewSelectSet (uiw->songselTree, idx - uisongsel->idxStart);
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
  int             dir = UISONGSEL_NEXT;

  logProcBegin (LOG_PROC, "uisongselScrollEvent");

  /* i'd like to have a way to turn off smooth scrolling for the application */
  if (event->direction == GDK_SCROLL_SMOOTH) {
    double dx, dy;

    gdk_event_get_scroll_deltas ((GdkEvent *) event, &dx, &dy);
    if (dy < 0.0) {
      dir = UISONGSEL_PREVIOUS;
    }
    if (dy > 0.0) {
      dir = UISONGSEL_NEXT;
    }
  }
  if (event->direction == GDK_SCROLL_DOWN) {
    dir = UISONGSEL_NEXT;
  }
  if (event->direction == GDK_SCROLL_UP) {
    dir = UISONGSEL_PREVIOUS;
  }

  uisongselProcessScroll (uisongsel, dir, 1);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
  return UICB_STOP;
}

static void
uisongselProcessScroll (uisongsel_t *uisongsel, int dir, int lines)
{
  ss_internal_t  *uiw;
  nlistidx_t      tval;

  uiw = uisongsel->uiWidgetData;

  if (dir == UISONGSEL_NEXT) {
    uisongsel->idxStart += lines;
  }
  if (dir == UISONGSEL_PREVIOUS) {
    uisongsel->idxStart -= lines;
  }

  if (uisongsel->idxStart < 0) {
    uisongsel->idxStart = 0;
  }

  tval = (nlistidx_t) uisongsel->dfilterCount - uiw->maxRows;
  if (uisongsel->idxStart > tval) {
    uisongsel->idxStart = tval;
  }

  uisongselScroll (GTK_RANGE (uiw->songselScrollbar.widget), GTK_SCROLL_JUMP,
      (double) uisongsel->idxStart, uisongsel);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
}

/* used by clear all selections */
static void
uisongselClearSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t    *uiw;

  uiw = uisongsel->uiWidgetData;
  gtk_tree_selection_unselect_iter (uiw->sel, iter);
}

/* clears a single selection.  use when only one item is selected */
static void
uisongselClearSingleSelection (uisongsel_t *uisongsel)
{
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  gtk_tree_selection_selected_foreach (uiw->sel,
      uisongselGetIter, uisongsel);

  gtk_tree_selection_unselect_iter (uiw->sel, &uiw->currIter);
}

static bool
uisongselKeyEvent (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *uiw;

  if (uisongsel == NULL) {
    return UICB_CONT;
  }

  uiw = uisongsel->uiWidgetData;

  uiw->shiftPressed = false;
  uiw->controlPressed = false;
  if (uiKeyIsShiftPressed (uiw->uikey)) {
    uiw->shiftPressed = true;
  }
  if (uiKeyIsControlPressed (uiw->uikey)) {
    uiw->controlPressed = true;
  }

  if (uiKeyIsPressEvent (uiw->uikey) &&
      uiKeyIsAudioPlayKey (uiw->uikey)) {
    uisongselPlayCallback (uisongsel);
  }

  if (uiKeyIsMovementKey (uiw->uikey)) {
    int     dir;
    int     lines;

    dir = UISONGSEL_DIR_NONE;
    lines = 1;

    if (uiKeyIsPressEvent (uiw->uikey)) {
      if (uiKeyIsUpKey (uiw->uikey)) {
        dir = UISONGSEL_PREVIOUS;
      }
      if (uiKeyIsDownKey (uiw->uikey)) {
        dir = UISONGSEL_NEXT;
      }
      if (uiKeyIsPageUpDownKey (uiw->uikey)) {
        lines = uiw->maxRows;
      }

      uisongselMoveSelection (uisongsel, dir, lines, UISONGSEL_MOVE_KEY);
    }

    /* movement keys are handled internally */
    return UICB_STOP;
  }

  return UICB_CONT;
}

static void
uisongselSelectionChgCallback (GtkTreeSelection *sel, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t    *uiw;
  nlist_t           *tlist;
  nlistidx_t        idx;
  nlistidx_t        iteridx;

  uiw = uisongsel->uiWidgetData;

  if (uiw->inscroll) {
    return;
  }

  if (uiw->selectedBackup != NULL &&
      uisongsel->dispselType != DISP_SEL_MM) {
    /* if the music manager is currently using the song list */
    /* only update the selection list for the music manager */
    return;
  }

  /* if neither the control key nor the shift key are pressed */
  /* then this is a new selection and not a modification */
  tlist = nlistAlloc ("selected-list-chg", LIST_ORDERED, NULL);

  /* if the shift key is pressed, get the first and the last item */
  /* in the selection list (both, as it is not yet known where */
  /* the new selection is in relation). */
  if (uiw->shiftPressed) {
    uiw->shiftfirstidx = -1;
    uiw->shiftlastidx = -1;
    nlistStartIterator (uiw->selectedList, &iteridx);
    while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
      if (uiw->shiftfirstidx == -1) {
        uiw->shiftfirstidx = idx;
      }
      uiw->shiftlastidx = idx;
    }
  }

  /* if the control-key is pressed, add any current */
  /* selection that is not in view to the new selection list */
  if (uiw->controlPressed) {
    nlistStartIterator (uiw->selectedList, &iteridx);
    while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
      if (idx < uisongsel->idxStart ||
          idx > uisongsel->idxStart + uiw->maxRows - 1) {
        nlistSetNum (tlist, idx, nlistGetNum (uiw->selectedList, idx));
      }
    }
  }

  if (! uisongsel->ispeercall) {
    for (int i = 0; i < uisongsel->peercount; ++i) {
      if (uisongsel->peers [i] == NULL) {
        continue;
      }
      uisongselSetPeerFlag (uisongsel->peers [i], true);
      uisongselClearAllSelections (uisongsel->peers [i]);
      uisongselSetPeerFlag (uisongsel->peers [i], false);
    }
  }

  nlistFree (uiw->selectedList);
  uiw->selectedList = tlist;
  nlistStartIterator (uiw->selectedList, &uiw->selectListIter);

  uiw->selectListKey = nlistIterateKey (uiw->selectedList, &uiw->selectListIter);

  /* and now process the selections from gtk */
  gtk_tree_selection_selected_foreach (sel,
      uisongselProcessSelection, uisongsel);

  /* any time the selection is changed, re-start the edit iterator */
  nlistStartIterator (uiw->selectedList, &uiw->selectListIter);
  uiw->selectListKey = nlistIterateKey (uiw->selectedList, &uiw->selectListIter);

  if (uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    /* the song editor points to the first selected */
    dbidx = nlistGetNum (uiw->selectedList, uiw->selectListKey);
    if (dbidx >= 0) {
      callbackHandlerLong (uisongsel->newselcb, dbidx);
    }
  }
}

static void
uisongselProcessSelection (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t    *uiw;
  glong             idx;
  glong             dbidx;
  char              *pathstr;

  uiw = uisongsel->uiWidgetData;

  gtk_tree_model_get (model, iter, SONGSEL_COL_IDX, &idx, -1);

  if (uiw->shiftPressed) {
    nlistidx_t    beg = 0;
    nlistidx_t    end = -1;

    if (idx <= uiw->shiftfirstidx) {
      beg = idx;
      end = uiw->shiftfirstidx;
    }
    if (idx >= uiw->shiftlastidx) {
      beg = uiw->shiftlastidx;
      end = idx;
    }

    for (nlistidx_t i = beg; i <= end; ++i) {
      if (i >= uisongsel->idxStart &&
          i < uisongsel->idxStart + uiw->maxRows) {
        uiTreeViewSelectSet (uiw->songselTree, i - uisongsel->idxStart);
      }
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      nlistSetNum (uiw->selectedList, i, dbidx);
    }
  } else {
    gtk_tree_model_get (model, iter, SONGSEL_COL_DBIDX, &dbidx, -1);
    nlistSetNum (uiw->selectedList, idx, dbidx);
  }

  uisongsel->lastdbidx = dbidx;

  if (uisongsel->ispeercall) {
    return;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselScrollSelection (uisongsel->peers [i], uisongsel->idxStart, UISONGSEL_SCROLL_FORCE, UISONGSEL_DIR_NONE);
    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    uisongselSetSelection (uisongsel->peers [i], atol (pathstr));
    mdfree (pathstr);     // allocated by gtk
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
}

static void
uisongselPopulateDataCallback (int col, long num, const char *str, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  uitreedispSetDisplayColumn (uiw->songselTree, col, num, str);
}


static void
uisongselMoveSelection (void *udata, int where, int lines, int moveflag)
{
  uisongsel_t     *uisongsel = udata;
  GtkTreeModel    *model = NULL;
  GtkTreePath     *path;
  ss_internal_t  *uiw;
  int             count;
  long            loc = -1;
  char            *pathstr;
  dbidx_t         nidx;
  bool            scrolled = false;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }

  count = nlistGetCount (uiw->selectedList);

  if (count == 0) {
    return;
  }

  if (count > 1 && moveflag == UISONGSEL_MOVE_KEY) {
    return;
  }

  if (count > 1) {
    /* need to be able to move forwards and backwards within the select-list */
    /* do not change the gtk selection */

    if (where == UISONGSEL_FIRST) {
      nlistStartIterator (uiw->selectedList, &uiw->selectListIter);
      uiw->selectListKey = nlistIterateKey (uiw->selectedList, &uiw->selectListIter);
    }
    if (where == UISONGSEL_NEXT) {
      nlistidx_t  pkey;
      nlistidx_t  piter;

      pkey = uiw->selectListKey;
      piter = uiw->selectListIter;

      uiw->selectListKey = nlistIterateKey (uiw->selectedList, &uiw->selectListIter);
      if (uiw->selectListKey < 0) {
        /* remain on the current selection, keep the iterator intact */
        uiw->selectListKey = pkey;
        uiw->selectListIter = piter;
      }
    }
    if (where == UISONGSEL_PREVIOUS) {
      uiw->selectListKey = nlistIterateKeyPrevious (uiw->selectedList, &uiw->selectListIter);
      if (uiw->selectListKey < 0) {
        /* reset to the beginning */
        uiw->selectListKey = nlistIterateKey (uiw->selectedList, &uiw->selectListIter);
      }
    }

    uisongselScrollSelection (uisongsel, uiw->selectListKey, UISONGSEL_SCROLL_NORMAL, where);
    if (uisongsel->newselcb != NULL) {
      dbidx_t   dbidx;

      dbidx = nlistGetNum (uiw->selectedList, uiw->selectListKey);
      if (dbidx >= 0) {
        callbackHandlerLong (uisongsel->newselcb, dbidx);
      }
    }
  }

  if (count == 1) {
    uiwidget_t  *uiwidgetp;

    /* calling getSelectLocation() will set currIter */
    uisongselGetSelectLocation (uisongsel);

    uiwidgetp = uiTreeViewGetWidget (uiw->songselTree);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
    path = gtk_tree_model_get_path (model, &uiw->currIter);
    mdextalloc (path);
    loc = 0;
    if (path != NULL) {
      pathstr = gtk_tree_path_to_string (path);
      mdextalloc (pathstr);
      loc = atol (pathstr);
      mdextfree (path);
      gtk_tree_path_free (path);
      mdfree (pathstr);     // allocated by gtk
    }

    uisongselClearSingleSelection (uisongsel);

    if (where == UISONGSEL_FIRST) {
      scrolled = uisongselScrollSelection (uisongsel, 0, UISONGSEL_SCROLL_NORMAL, UISONGSEL_DIR_NONE);
      gtk_tree_model_get_iter_first (model, &uiw->currIter);
    }
    if (where == UISONGSEL_NEXT) {
      nidx = uisongsel->idxStart + loc;
      while (lines > 0) {
        ++nidx;
        scrolled = uisongselScrollSelection (uisongsel, nidx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_NEXT);
        if (! scrolled) {
          glong    idx;

          gtk_tree_model_get (model, &uiw->currIter, SONGSEL_COL_IDX, &idx, -1);
          if (loc < uiw->maxRows - 1 &&
              idx < uisongsel->dfilterCount - 1) {
            gtk_tree_model_iter_next (model, &uiw->currIter);
            ++loc;
          } else {
            break;
          }
        }
        --lines;
      }
    }
    if (where == UISONGSEL_PREVIOUS) {
      nidx = uisongsel->idxStart + loc;
      while (lines > 0) {
        --nidx;
        scrolled = uisongselScrollSelection (uisongsel, nidx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_PREVIOUS);
        if (! scrolled) {
          if (loc > 0) {
            gtk_tree_model_iter_previous (model, &uiw->currIter);
            --loc;
          } else {
            break;
          }
        }
        --lines;
      }
    }

    /* if the scroll was bumped, 'currIter' is still pointing to the same */
    /* row (but a new dbidx), re-select it */
    /* if the iter was moved, currIter is pointing at the new selection */
    /* if the iter was not moved, the original must be re-selected */
    /* no need to check the return value from the iter change */
    gtk_tree_selection_select_iter (uiw->sel, &uiw->currIter);
  }
}

static void
uisongselGetIter (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *uiw = NULL;

  uiw = uisongsel->uiWidgetData;
  memcpy (&uiw->currIter, iter, sizeof (GtkTreeIter));
}

/* have to handle the case where the user switches tabs back to the */
/* song selection tab */
static bool
uisongselSongEditCallback (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  long            dbidx;

  if (uisongsel->newselcb != NULL) {
    dbidx = uisongsel->lastdbidx;
    if (dbidx < 0) {
      return UICB_CONT;
    }
    callbackHandlerLong (uisongsel->newselcb, dbidx);
  }
  return callbackHandler (uisongsel->editcb);
}

static int *
uiTreeViewAddDisplayType (int *types, int type, int col)
{
  if (type == TREE_TYPE_NUM) {
    /* despite being a numeric type, the display needs a string */
    /* so that empty values can be displayed */
    type = TREE_TYPE_STRING;
  }
  types = uiAppendType (types, col, type);

  return types;
}

static int *
uiAppendType (int *types, int ncol, int type)
{
  types = mdrealloc (types, (ncol + 1) * sizeof (int));
  types [ncol] = type;

  return types;
}

