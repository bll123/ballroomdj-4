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
#include "colorutils.h"
#include "conn.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "songfilter.h"
#include "uidance.h"
#include "uisongsel.h"
#include "ui.h"
#include "uifavorite.h"
#include "uisong.h"
#include "uisongfilter.h"

enum {
  SONGSEL_COL_ELLIPSIZE,
  SONGSEL_COL_FONT,
  SONGSEL_COL_IDX,
  SONGSEL_COL_SORTIDX,
  SONGSEL_COL_DBIDX,
  SONGSEL_COL_FAV_COLOR,
  SONGSEL_COL_MARK_COLOR,
  SONGSEL_COL_MARK,
  SONGSEL_COL_SAMESONG_COLOR,
  SONGSEL_COL_MAX,
};

enum {
  STORE_ROWS = 60,
};

enum {
  UISONGSEL_FIRST,
  UISONGSEL_NEXT,
  UISONGSEL_PREVIOUS,
  UISONGSEL_PLAY,
  UISONGSEL_QUEUE,
};

/* for callbacks */
enum {
  SONGSEL_CB_SELECT,
  SONGSEL_CB_QUEUE,
  SONGSEL_CB_PLAY,
  SONGSEL_CB_FILTER,
  SONGSEL_CB_EDIT_LOCAL,
  SONGSEL_CB_DANCE_SEL,
  SONGSEL_CB_MAX,
};

#define MARK_DISPLAY "\xe2\x96\x8B"  // left five-eights block


typedef struct uisongselgtk {
  UICallback          callbacks [SONGSEL_CB_MAX];
  UIWidget            *parentwin;
  UIWidget            vbox;
  GtkWidget           *songselTree;
  GtkTreeSelection    *sel;
  GtkWidget           *songselScrollbar;
  GtkEventController  *scrollController;
  GtkTreeViewColumn   *favColumn;
  UIWidget            scrolledwin;
  UIWidget            playbutton;
  /* other data */
  int               lastTreeSize;
  double            lastRowHeight;
  int               maxRows;
  nlist_t           *selectedBackup;
  nlist_t           *selectedList;
  GtkTreeModel      *model;
  GtkTreeIter       *iterp;
  GType             *typelist;
  int               col;        // for the display type callback
  const char        *markcolor;
  bool              controlPressed : 1;
  bool              shiftPressed : 1;
  bool              inscroll : 1;
} uisongselgtk_t;


static void uisongselClearSelections (uisongsel_t *uisongsel);
static bool uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart);
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
static void uisongselClearAllSelections (uisongsel_t *uisongsel);
static void uisongselClearSelection (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static gboolean uisongselKeyEvent (GtkWidget *w, GdkEventKey *event, gpointer udata);
static void uisongselSelectionChgCallback (GtkTreeSelection *sel, gpointer udata);
static void uisongselProcessSelection (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void uisongselPopulateDataCallback (int col, long num, const char *str, void *udata);

static void uisongselMoveSelection (void *udata, int where);

static bool uisongselUIDanceSelectCallback (void *udata, long idx);
static bool uisongselSongEditCallback (void *udata);


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
  uiw->selectedBackup = NULL;
  uiw->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    uiutilsUICallbackInit (&uiw->callbacks [i], NULL, NULL);
  }
  uiutilsUIWidgetInit (&uiw->playbutton);
  uiw->markcolor = bdjoptGetStr (OPT_P_UI_MARK_COL);

  uisongsel->uiWidgetData = uiw;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  if (uisongsel->uiWidgetData != NULL) {
    uisongselgtk_t    *uiw;

    uiw = uisongsel->uiWidgetData;
    if (uiw->selectedBackup != NULL) {
      nlistFree (uiw->selectedBackup);
    }
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
  GtkAdjustment     *adjustment;
  slist_t           *sellist;
  char              tbuff [200];
  double            tupper;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
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

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    /* CONTEXT: song-selection: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CB_SELECT],
        uisongselSelectCallback, uisongsel);
    uiCreateButton (&uiwidget,
        &uiw->callbacks [SONGSEL_CB_SELECT], tbuff, NULL);
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CB_EDIT_LOCAL],
        uisongselSongEditCallback, uisongsel);
    uiCreateButton (&uiwidget, &uiw->callbacks [SONGSEL_CB_EDIT_LOCAL],
        /* CONTEXT: song-selection: edit the selected song */
        _("Edit"), "button_edit");
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uisongsel->dispselType == DISP_SEL_REQUEST) {
    /* CONTEXT: song-selection: queue a song to be played */
    strlcpy (tbuff, _("Queue"), sizeof (tbuff));
    uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CB_QUEUE],
        uisongselQueueCallback, uisongsel);
    uiCreateButton (&uiwidget,
        &uiw->callbacks [SONGSEL_CB_QUEUE], tbuff, NULL);
    uiBoxPackStart (&hbox, &uiwidget);
  }
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    /* CONTEXT: song-selection: play the selected songs */
    strlcpy (tbuff, _("Play"), sizeof (tbuff));
    uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CB_PLAY],
        uisongselPlayCallback, uisongsel);
    uiCreateButton (&uiwidget,
        &uiw->callbacks [SONGSEL_CB_PLAY], tbuff, NULL);
    uiBoxPackStart (&hbox, &uiwidget);
    uiutilsUIWidgetCopy (&uiw->playbutton, &uiwidget);
  }

  uiutilsUICallbackLongInit (&uiw->callbacks [SONGSEL_CB_DANCE_SEL],
      uisongselUIDanceSelectCallback, uisongsel);
  uisongsel->uidance = uidanceDropDownCreate (&hbox, parentwin,
      /* CONTEXT: song-selection: filter: all dances are selected */
      UIDANCE_ALL_DANCES, _("All Dances"), UIDANCE_PACK_END);
  uidanceSetCallback (uisongsel->uidance,
      &uiw->callbacks [SONGSEL_CB_DANCE_SEL]);

  uiutilsUICallbackInit (&uiw->callbacks [SONGSEL_CB_FILTER],
      uisfDialog, uisongsel->uisongfilter);
  uiCreateButton (&uiwidget,
      &uiw->callbacks [SONGSEL_CB_FILTER],
      /* CONTEXT: song-selection: a button that starts the filters (narrowing down song selections) dialog */
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

  uiCreateScrolledWindow (&uiw->scrolledwin, 400);
  uiWindowSetPolicyExternal (&uiw->scrolledwin);
  uiWidgetExpandHoriz (&uiw->scrolledwin);
  uiBoxPackStartExpand (&vbox, &uiw->scrolledwin);

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

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uiw->songselTree));
  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);
  uiw->scrollController =
      gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uiw->songselTree, GDK_SCROLL_MASK);
  uiBoxPackInWindowUW (&uiw->scrolledwin, uiw->songselTree);
  g_signal_connect (uiw->songselTree, "row-activated",
      G_CALLBACK (uisongselCheckFavChgSignal), uisongsel);
  g_signal_connect (uiw->songselTree, "scroll-event",
      G_CALLBACK (uisongselScrollEvent), uisongsel);

  gtk_event_controller_scroll_new (uiw->songselTree,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);

  /* the mark display is a special case, it always exists */
  renderer = gtk_cell_renderer_text_new ();
  col = SONGSEL_COL_MARK_COLOR;
  if (uisongsel->dispselType == DISP_SEL_MM) {
    col = SONGSEL_COL_SAMESONG_COLOR;
  }
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", SONGSEL_COL_MARK,
      "foreground", col,
      "font", SONGSEL_COL_FONT,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiw->songselTree), column);

  uiw->favColumn = uiAddDisplayColumns (
      uiw->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uiw->maxRows = STORE_ROWS;
  uisongselPopulateData (uisongsel);

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: initial", uisongsel->tag);
  uisongselProcessSongFilter (uisongsel);
  uisongselPopulateData (uisongsel);

  uidanceSetValue (uisongsel->uidance, -1);

  g_signal_connect ((GtkWidget *) uiw->sel, "changed",
      G_CALLBACK (uisongselSelectionChgCallback), uisongsel);
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
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkTreeModel        * model = NULL;
  GtkTreeIter         iter;
  GtkAdjustment       * adjustment;
  long                idx;
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
  char                * sscolor = "#000000";

  logProcBegin (LOG_PROC, "uisongselPopulateData");
  uiw = uisongsel->uiWidgetData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));

  /* re-fetch the count, as the songfilter process may not have been */
  /* processed by this instance */
  uisongsel->dfilterCount = (double) songfilterGetCount (uisongsel->songfilter);

  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));

  count = 0;
  idx = uisongsel->idxStart;
  while (count < uiw->maxRows) {
    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      song = NULL;
      if (dbidx >= 0) {
        song = dbGetByIdx (uisongsel->musicdb, dbidx);
      }
      if (song != NULL && (double) count < uisongsel->dfilterCount) {
        char    *mark;

        mark = "";
        if (uisongsel->dispselType != DISP_SEL_MM &&
            uisongsel->songlistdbidxlist != NULL) {
          /* check and see if the song is in the song list */
          if (nlistGetNum (uisongsel->songlistdbidxlist, dbidx) >= 0) {
            mark = MARK_DISPLAY;
          }
        }

        if (uisongsel->dispselType == DISP_SEL_MM) {
          ssize_t samesong;
          char    tbuff [40];

          samesong = songGetNum (song, TAG_SAMESONG);
          if (samesong > 0) {
            sscolor = nlistGetStr (uisongsel->samesonglist, samesong);
            if (sscolor == NULL) {
              createRandomColor (tbuff, sizeof (tbuff));
              nlistSetStr (uisongsel->samesonglist, samesong, tbuff);
              sscolor = nlistGetStr (uisongsel->samesonglist, samesong);
            }
            mark = MARK_DISPLAY;
          }
        }

        /* favorite color processing */
        favorite = songGetFavoriteData (song);
        color = favorite->color;
        if (strcmp (color, "") == 0) {
          uiGetForegroundColorW (uiw->songselTree, tmp, sizeof (tmp));
          color = tmp;
        }

        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            SONGSEL_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
            SONGSEL_COL_FONT, listingFont,
            SONGSEL_COL_IDX, (glong) idx,
            SONGSEL_COL_SORTIDX, (glong) idx,
            SONGSEL_COL_DBIDX, (glong) dbidx,
            SONGSEL_COL_FAV_COLOR, color,
            SONGSEL_COL_MARK_COLOR, uiw->markcolor,
            SONGSEL_COL_MARK, mark,
            SONGSEL_COL_SAMESONG_COLOR, sscolor,
            -1);

        sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
        uiw->model = model;
        uiw->iterp = &iter;
        uisongSetDisplayColumns (sellist, song, SONGSEL_COL_MAX,
            uisongselPopulateDataCallback, uisongsel);
      } /* song is not null */
    } /* iter is valid */

    ++idx;
    ++count;
  }

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
  /* it's confusing */
  return UICB_CONT;
}

void
uisongselSetDefaultSelection (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;
  int             count;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count < 1) {
    GtkTreeModel  *model;
    GtkTreeIter   iter;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));
    gtk_tree_model_get_iter_first (model, &iter);
    gtk_tree_selection_select_iter (uiw->sel, &iter);
  }

  return;
}

void
uisongselSetSelection (uisongsel_t *uisongsel, long idx)
{
  uisongselgtk_t  *uiw;
  GtkTreePath     *path = NULL;
  char            tbuff [40];

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }
  if (idx < 0) {
    return;
  }

  uisongselClearAllSelections (uisongsel);
  snprintf (tbuff, sizeof (tbuff), "%ld", idx);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL) {
    gtk_tree_selection_select_path (uiw->sel, path);
    gtk_tree_path_free (path);
  }
}

bool
uisongselNextSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_NEXT);
  return UICB_CONT;
}

bool
uisongselPreviousSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_PREVIOUS);
  return UICB_CONT;
}

bool
uisongselFirstSelection (void *udata)
{
  uisongselMoveSelection (udata, UISONGSEL_FIRST);
  return UICB_CONT;
}

/* only works for single selection */
long
uisongselGetSelectLocation (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;
  GtkTreeModel    *model = NULL;
  GtkTreeIter     iter;
  GtkTreePath     *path;
  char            *pathstr;
  int             count;
  long            loc;

  uiw = uisongsel->uiWidgetData;
  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    return 0;
  }
  gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
  path = gtk_tree_model_get_path (model, &iter);
  loc = 0;
  if (path != NULL) {
    pathstr = gtk_tree_path_to_string (path);
    loc = atol (pathstr);
    gtk_tree_path_free (path);
    free (pathstr);
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
uisongselDanceSelectHandler (uisongsel_t *uisongsel, ssize_t danceIdx)
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
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  uiw->selectedBackup = uiw->selectedList;
  uiw->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);

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
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  if (uiw->selectedList != NULL) {
    nlistFree (uiw->selectedList);
  }
  uiw->selectedList = uiw->selectedBackup;
  uiw->selectedBackup = NULL;
  uisongselScrollSelection (uisongsel, uisongsel->idxStart);

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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: play button");
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
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetDisable (&uiw->playbutton);
  } else {
    uiWidgetEnable (&uiw->playbutton);
  }
}

/* internal routines */

static void
uisongselClearSelections (uisongsel_t *uisongsel)
{
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;

  uisongsel->idxStart = 0;
  nlistFree (uiw->selectedList);
  uiw->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
}

static bool
uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart)
{
  uisongselgtk_t  *uiw;
  bool            scrolled = false;
  dbidx_t         oidx;

  uiw = uisongsel->uiWidgetData;

  oidx = uisongsel->idxStart;
  uisongsel->idxStart = idxStart;

  uisongselPopulateData (uisongsel);

  uisongselScroll (GTK_RANGE (uiw->songselScrollbar), GTK_SCROLL_JUMP,
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

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: queue button");
  mqidx = MUSICQ_CURRENT;
  uisongselQueueHandler (uisongsel, mqidx, UISONGSEL_QUEUE);
  return UICB_CONT;
}

static void
uisongselQueueHandler (uisongsel_t *uisongsel, musicqidx_t mqidx, int action)
{
  uisongselgtk_t  * uiw;
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
  }
  logProcEnd (LOG_PROC, "uisongselQueueHandler", "");
  return;
}

static bool
uisongselUIDanceSelectCallback (void *udata, long idx)
{
  uisongsel_t *uisongsel = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: dance select");
  uisongselDanceSelectHandler (uisongsel, idx);
  return UICB_CONT;
}

static void
uisongselInitializeStore (uisongsel_t *uisongsel)
{
  uisongselgtk_t      * uiw;
  GtkListStore      *store = NULL;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

  uiw = uisongsel->uiWidgetData;
  uiw->typelist = malloc (sizeof (GType) * SONGSEL_COL_MAX);
  uiw->col = 0;
  /* attributes ellipsize/align/font*/
  uiw->typelist [uiw->col++] = G_TYPE_INT;
  uiw->typelist [uiw->col++] = G_TYPE_STRING;
  /* internal idx/sortidx/dbidx */
  uiw->typelist [uiw->col++] = G_TYPE_LONG,
  uiw->typelist [uiw->col++] = G_TYPE_LONG,
  uiw->typelist [uiw->col++] = G_TYPE_LONG,
  /* fav color/mark color/mark/samesong color */
  uiw->typelist [uiw->col++] = G_TYPE_STRING,
  uiw->typelist [uiw->col++] = G_TYPE_STRING,
  uiw->typelist [uiw->col++] = G_TYPE_STRING,
  uiw->typelist [uiw->col++] = G_TYPE_STRING,

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  uisongAddDisplayTypes (sellist, uisongselInitializeStoreCallback, uisongsel);

  store = gtk_list_store_newv (uiw->col, uiw->typelist);
  free (uiw->typelist);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->songselTree), GTK_TREE_MODEL (store));
  logProcEnd (LOG_PROC, "uisongselInitializeStore", "");
}

static void
uisongselInitializeStoreCallback (int type, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  uiw->typelist = uiTreeViewAddDisplayType (uiw->typelist, type, uiw->col);
  ++uiw->col;
}


static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  uisongselgtk_t    *uiw;
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
  uisongselgtk_t    * uiw;

  logProcBegin (LOG_PROC, "uisongselCheckFavChgSignal");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");

  uiw = uisongsel->uiWidgetData;
  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "not-fav-col");
    return;
  }

  uisongselChangeFavorite (uisongsel, uisongsel->lastdbidx);
  logProcEnd (LOG_PROC, "uisongselCheckFavChgSignal", "");
}

static void
uisongselProcessTreeSize (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uisongselgtk_t  *uiw;
  uisongsel_t     *uisongsel = udata;
  GtkAdjustment   *adjustment;
  double          ps;
  double          tmax;

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

    tmax = ps / uiw->lastRowHeight;
    uiw->maxRows = (int) round (tmax);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s max-rows:%d", uisongsel->tag, uiw->maxRows);

    adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
    /* the step increment does not work correctly with smooth scrolling */
    /* and it appears there's no easy way to turn smooth scrolling off */
    gtk_adjustment_set_step_increment (adjustment, 4.0);
    gtk_adjustment_set_page_increment (adjustment, (double) uiw->maxRows);
    gtk_adjustment_set_page_size (adjustment, (double) uiw->maxRows);

    uiw->lastTreeSize = allocation->height;

    logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: tree size change", uisongsel->tag);
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

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiw->songselScrollbar));
  gtk_adjustment_set_value (adjustment, value);

  /* set the selections based on the saved selection list */
  nlistStartIterator (uiw->selectedList, &iteridx);
  while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
    GtkTreePath *path;
    char        tmp [40];

    if (idx >= uisongsel->idxStart &&
        idx < uisongsel->idxStart + uiw->maxRows) {
      snprintf (tmp, sizeof (tmp), "%d", idx - uisongsel->idxStart);
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
uisongselSelectionChgCallback (GtkTreeSelection *sel, gpointer udata)
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
  nlistFree (uiw->selectedList);
  if (uiw->controlPressed || uiw->shiftPressed) {
    nlistStartIterator (uiw->selectedList, &iteridx);
    while ((idx = nlistIterateKey (uiw->selectedList, &iteridx)) >= 0) {
      if (idx < uisongsel->idxStart ||
          idx > uisongsel->idxStart + uiw->maxRows - 1) {
        nlistSetNum (tlist, idx, nlistGetNum (uiw->selectedList, idx));
      }
    }
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
  char              *pathstr;

  uiw = uisongsel->uiWidgetData;

  gtk_tree_model_get (model, iter, SONGSEL_COL_IDX, &idx, -1);
  gtk_tree_model_get (model, iter, SONGSEL_COL_DBIDX, &dbidx, -1);
  nlistSetNum (uiw->selectedList, idx, dbidx);
  if (uisongsel->newselcb != NULL) {
    uiutilsCallbackLongHandler (uisongsel->newselcb, dbidx);
    uisongsel->lastdbidx = dbidx;
  }

  if (uisongsel->ispeercall) {
    return;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselScrollSelection (uisongsel->peers [i], uisongsel->idxStart);
    pathstr = gtk_tree_path_to_string (path);
    uisongselSetSelection (uisongsel->peers [i], atol (pathstr));
    free (pathstr);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
}

static void
uisongselPopulateDataCallback (int col, long num, const char *str, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;

  uiw = uisongsel->uiWidgetData;
  uiTreeViewSetDisplayColumn (uiw->model, uiw->iterp, col, num, str);
}


/* only works when in single selection mode */
static void
uisongselMoveSelection (void *udata, int where)
{
  uisongsel_t     *uisongsel = udata;
  uisongselgtk_t  *uiw;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  GtkTreePath     *path;
  char            *pathstr;
  int             count;
  int             valid;
  int             loc = -1;
  dbidx_t         nidx;
  bool            scrolled = false;

  uiw = uisongsel->uiWidgetData;

  if (uiw->sel == NULL) {
    return;
  }

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count == 1) {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->songselTree));
    gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    if (path != NULL) {
      pathstr = gtk_tree_path_to_string (path);
      loc = atol (pathstr);
      gtk_tree_path_free (path);
      free (pathstr);
    }

    valid = false;
    if (where == UISONGSEL_FIRST) {
      scrolled = uisongselScrollSelection (uisongsel, 0);
      valid = gtk_tree_model_get_iter_first (model, &iter);
    }
    if (where == UISONGSEL_NEXT) {
      nidx = uisongsel->idxStart + 1;
      scrolled = uisongselScrollSelection (uisongsel, nidx);
      if (! scrolled) {
        if (loc < uiw->maxRows - 1 &&
            (double) loc < uisongsel->dfilterCount) {
          valid = gtk_tree_model_iter_next (model, &iter);
        }
      } else {
        valid = true;
      }
    }
    if (where == UISONGSEL_PREVIOUS) {
      nidx = uisongsel->idxStart - 1;
      scrolled = uisongselScrollSelection (uisongsel, nidx);
      if (! scrolled) {
        valid = gtk_tree_model_iter_previous (model, &iter);
      } else {
        valid = true;
      }
    }
    if (valid) {
      gtk_tree_selection_select_iter (uiw->sel, &iter);
    }
  }
}

/* have to handle the case where the user switches tabs back to the */
/* song selection tab */
static bool
uisongselSongEditCallback (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  long            dbidx;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: edit button");
  if (uisongsel->newselcb != NULL) {
    dbidx = uisongsel->lastdbidx;
    if (dbidx < 0) {
      return UICB_CONT;
    }
    uiutilsCallbackLongHandler (uisongsel->newselcb, dbidx);
  }
  return uiutilsCallbackHandler (uisongsel->editcb);
}

