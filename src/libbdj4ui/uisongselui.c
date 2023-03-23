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
  SONGSEL_CB_SCROLL_CHG,
  SONGSEL_CB_SEL_CHG,
  SONGSEL_CB_SELECT_PROCESS,
  SONGSEL_CB_CHK_FAV_CHG,
  SONGSEL_CB_SZ_CHG,
  SONGSEL_CB_SCROLL_EVENT,
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

typedef struct ss_internal {
  callback_t          *callbacks [SONGSEL_CB_MAX];
  uiwcont_t           *parentwin;
  uiwcont_t           *vbox;
  uitree_t            *songselTree;
  uiscrollbar_t       *songselScrollbar;
  GtkEventController  *scrollController;
  uiwcont_t           *scrolledwin;
  uibutton_t          *buttons [SONGSEL_BUTTON_MAX];
  uiwcont_t           reqQueueLabel;
  uikey_t             *uikey;
  /* other data */
  int                 maxRows;
  nlist_t             *selectedBackup;
  nlist_t             *selectedList;
  nlistidx_t          selectListIter;
  nlistidx_t          selectListKey;
  GtkTreeIter         currIter;
  int                 *typelist;
  int                 colcount;            // for the display type callback
  const   char        *markcolor;
  /* for shift-click */
  nlistidx_t          shiftfirstidx;
  nlistidx_t          shiftlastidx;
  /* for double-click checks */
  dbidx_t             lastRowDBIdx;
  mstime_t            lastRowCheck;
  bool                controlPressed : 1;
  bool                shiftPressed : 1;
  bool                inscroll : 1;
  bool                inselectchgprocess : 1;
} ss_internal_t;

static void uisongselClearSelections (uisongsel_t *uisongsel);
static bool uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart, int scrollflag, int dir);
static bool uisongselQueueCallback (void *udata);
static void uisongselQueueHandler (uisongsel_t *uisongsel, musicqidx_t mqidx, int action);
static void uisongselInitializeStore (uisongsel_t *uisongsel);
static void uisongselInitializeStoreCallback (int type, void *udata);
static void uisongselCreateRows (uisongsel_t *uisongsel);
static void uisongselProcessSongFilter (uisongsel_t *uisongsel);

static bool uisongselCheckFavChgCallback (void *udata, long col);

static bool uisongselProcessTreeSize (void *udata, long rows);
static bool uisongselScroll (void *udata, double value);
static void uisongselUpdateSelections (uisongsel_t *uisongsel);
static bool uisongselScrollEvent (void *udata, long dir);
static void uisongselProcessScroll (uisongsel_t *uisongsel, int dir, int lines);
static bool uisongselKeyEvent (void *udata);
static bool uisongselSelectionChgCallback (void *udata);
static bool uisongselProcessSelection (void *udata, long row);
static void uisongselPopulateDataCallback (int col, long num, const char *str, void *udata);

static void uisongselMoveSelection (void *udata, int where, int lines, int moveflag);

static bool uisongselUIDanceSelectCallback (void *udata, long idx, int count);
static bool uisongselSongEditCallback (void *udata);

static int * uiTreeViewAddDisplayType (int *types, int type, int col);
static int * uiAppendType (int *types, int ncol, int type);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = mdmalloc (sizeof (ss_internal_t));
  ssint->vbox = NULL;
  ssint->songselTree = NULL;
  ssint->songselScrollbar = NULL;
  ssint->scrollController = NULL;
  ssint->maxRows = 0;
  ssint->controlPressed = false;
  ssint->shiftPressed = false;
  ssint->inscroll = false;
  ssint->inselectchgprocess = false;
  ssint->selectedBackup = NULL;
  ssint->selectedList = nlistAlloc ("selected-list-a", LIST_ORDERED, NULL);
  nlistStartIterator (ssint->selectedList, &ssint->selectListIter);
  ssint->selectListKey = -1;
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    ssint->callbacks [i] = NULL;
  }
  ssint->markcolor = bdjoptGetStr (OPT_P_UI_MARK_COL);
  for (int i = 0; i < SONGSEL_BUTTON_MAX; ++i) {
    ssint->buttons [i] = NULL;
  }
  ssint->lastRowDBIdx = -1;
  mstimeset (&ssint->lastRowCheck, 0);
  ssint->scrolledwin = NULL;

  ssint->uikey = uiKeyAlloc ();
  ssint->callbacks [SONGSEL_CB_KEYB] = callbackInit (
      uisongselKeyEvent, uisongsel, NULL);
  ssint->callbacks [SONGSEL_CB_SELECT_PROCESS] = callbackInitLong (
      uisongselProcessSelection, uisongsel);

  uisongsel->ssInternalData = ssint;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  if (uisongsel->ssInternalData != NULL) {
    ss_internal_t    *ssint;

    ssint = uisongsel->ssInternalData;
    uiwcontFree (ssint->vbox);
    uiwcontFree (ssint->scrolledwin);
    uiScrollbarFree (ssint->songselScrollbar);
    uiKeyFree (ssint->uikey);
    nlistFree (ssint->selectedBackup);
    nlistFree (ssint->selectedList);
    for (int i = 0; i < SONGSEL_BUTTON_MAX; ++i) {
      uiButtonFree (ssint->buttons [i]);
    }
    uiTreeViewFree (ssint->songselTree);
    for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
      callbackFree (ssint->callbacks [i]);
    }
    mdfree (ssint);
    uisongsel->ssInternalData = NULL;
  }
}

uiwcont_t *
uisongselBuildUI (uisongsel_t *uisongsel, uiwcont_t *parentwin)
{
  ss_internal_t     *ssint;
  uibutton_t        *uibutton;
  uiwcont_t         uiwidget;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *uitreewidgetp;
  uiwcont_t         *hbox;
  uiwcont_t         *vbox;
  GtkAdjustment     *adjustment;
  slist_t           *sellist;
  char              tbuff [200];
  double            tupper;
  int               col;

  logProcBegin (LOG_PROC, "uisongselBuildUI");

  ssint = uisongsel->ssInternalData;
  uisongsel->windowp = parentwin;

  ssint->vbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (ssint->vbox);
  uiWidgetExpandVert (ssint->vbox);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (ssint->vbox, hbox);

  /* The ez song selection does not need a select button, as it has */
  /* the left-arrow button.  Saves real estate. */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL) {
    /* CONTEXT: song-selection: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_SELECT] = callbackInit (
        uisongselSelectCallback, uisongsel, "songsel: select");
    uibutton = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_SELECT], tbuff, NULL);
    ssint->buttons [SONGSEL_BUTTON_SELECT] = uibutton;
    uiwidgetp = uiButtonGetWidgetContainer (uibutton);
    uiBoxPackStart (hbox, uiwidgetp);
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL) {
    ssint->callbacks [SONGSEL_CB_EDIT_LOCAL] = callbackInit (
        uisongselSongEditCallback, uisongsel, "songsel: edit");
    uibutton = uiCreateButton (ssint->callbacks [SONGSEL_CB_EDIT_LOCAL],
        /* CONTEXT: song-selection: edit the selected song */
        _("Edit"), "button_edit");
    ssint->buttons [SONGSEL_BUTTON_EDIT] = uibutton;
    uiwidgetp = uiButtonGetWidgetContainer (uibutton);
    uiBoxPackStart (hbox, uiwidgetp);
  }

  if (uisongsel->dispselType == DISP_SEL_REQUEST) {
    /* CONTEXT: (verb) song-selection: queue a song to be played */
    strlcpy (tbuff, _("Queue"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_QUEUE] = callbackInit (
        uisongselQueueCallback, uisongsel, "songsel: queue");
    uibutton = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_QUEUE], tbuff, NULL);
    ssint->buttons [SONGSEL_BUTTON_QUEUE] = uibutton;
    uiwidgetp = uiButtonGetWidgetContainer (uibutton);
    uiBoxPackStart (hbox, uiwidgetp);

    uiCreateLabel (&uiwidget, "");
    uiWidgetSetClass (&uiwidget, DARKACCENT_CLASS);
    uiBoxPackStart (hbox, &uiwidget);
    uiwcontCopy (&ssint->reqQueueLabel, &uiwidget);
  }
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    /* CONTEXT: song-selection: play the selected songs */
    strlcpy (tbuff, _("Play"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_PLAY] = callbackInit (
        uisongselPlayCallback, uisongsel, "songsel: play");
    uibutton = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_PLAY], tbuff, NULL);
    ssint->buttons [SONGSEL_BUTTON_PLAY] = uibutton;
    uiwidgetp = uiButtonGetWidgetContainer (uibutton);
    uiBoxPackStart (hbox, uiwidgetp);
  }

  ssint->callbacks [SONGSEL_CB_DANCE_SEL] = callbackInitLongInt (
      uisongselUIDanceSelectCallback, uisongsel);
  uisongsel->uidance = uidanceDropDownCreate (hbox, parentwin,
      /* CONTEXT: song-selection: filter: all dances are selected */
      UIDANCE_ALL_DANCES, _("All Dances"), UIDANCE_PACK_END, 1);
  uidanceSetCallback (uisongsel->uidance,
      ssint->callbacks [SONGSEL_CB_DANCE_SEL]);

  ssint->callbacks [SONGSEL_CB_FILTER] = callbackInit (
      uisfDialog, uisongsel->uisongfilter, "songsel: filters");
  uibutton = uiCreateButton (
      ssint->callbacks [SONGSEL_CB_FILTER],
      /* CONTEXT: song-selection: a button that starts the filters (narrowing down song selections) dialog */
      _("Filters"), NULL);
  ssint->buttons [SONGSEL_BUTTON_FILTER] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (hbox, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (ssint->vbox, hbox);

  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (hbox, vbox);

  tupper = uisongsel->dfilterCount;
  ssint->songselScrollbar = uiCreateVerticalScrollbar (tupper);
  uiBoxPackEnd (hbox, uiScrollbarGetWidgetContainer (ssint->songselScrollbar));
  ssint->callbacks [SONGSEL_CB_SCROLL_CHG] = callbackInitDouble (
      uisongselScroll, uisongsel);
  uiScrollbarSetChangeCallback (ssint->songselScrollbar,
      ssint->callbacks [SONGSEL_CB_SCROLL_CHG]);

  ssint->scrolledwin = uiCreateScrolledWindow (400);
  uiWindowSetPolicyExternal (ssint->scrolledwin);
  uiWidgetExpandHoriz (ssint->scrolledwin);
  uiBoxPackStartExpand (vbox, ssint->scrolledwin);

  ssint->songselTree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetWidgetContainer (ssint->songselTree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetExpandHoriz (uitreewidgetp);
  uiWidgetExpandVert (uitreewidgetp);
  uiTreeViewEnableHeaders (ssint->songselTree);
  /* for song list editing, multiple selections are valid */
  /* for the music manager, multiple selections are valid to allow */
  /* set/clear of same song marks.  also creates a new selection */
  /* mode for the song editor */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_EZSONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    uiTreeViewSelectSetMode (ssint->songselTree, SELECT_MULTIPLE);
  }
  uiKeySetKeyCallback (ssint->uikey, uitreewidgetp,
      ssint->callbacks [SONGSEL_CB_KEYB]);

  adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (uitreewidgetp->widget));
  tupper = uisongsel->dfilterCount;
  gtk_adjustment_set_upper (adjustment, tupper);
  ssint->scrollController =
      gtk_event_controller_scroll_new (uitreewidgetp->widget,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uitreewidgetp->widget, GDK_SCROLL_MASK);
  uiBoxPackInWindow (ssint->scrolledwin, uitreewidgetp);

  ssint->callbacks [SONGSEL_CB_CHK_FAV_CHG] = callbackInitLong (
        uisongselCheckFavChgCallback, uisongsel);
  uiTreeViewSetRowActivatedCallback (ssint->songselTree,
        ssint->callbacks [SONGSEL_CB_CHK_FAV_CHG]);

  ssint->callbacks [SONGSEL_CB_SCROLL_EVENT] = callbackInitLong (
        uisongselScrollEvent, uisongsel);
  uiTreeViewSetScrollEventCallback (ssint->songselTree,
        ssint->callbacks [SONGSEL_CB_SCROLL_EVENT]);

  gtk_event_controller_scroll_new (uitreewidgetp->widget,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);

  /* the mark display is a special case, it always exists */
  col = SONGSEL_COL_MARK_MARKUP;
  if (uisongsel->dispselType == DISP_SEL_MM) {
    col = SONGSEL_COL_SAMESONG_MARKUP;
  }
  uiTreeViewAppendColumn (ssint->songselTree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_MARKUP, col,
      TREE_COL_TYPE_FONT, SONGSEL_COL_FONT,
      TREE_COL_TYPE_END);

  uitreedispAddDisplayColumns (
      ssint->songselTree, sellist, SONGSEL_COL_MAX,
      SONGSEL_COL_FONT, SONGSEL_COL_ELLIPSIZE);

  uisongselInitializeStore (uisongsel);
  ssint->maxRows = STORE_ROWS;

  uisongselCreateRows (uisongsel);
  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: initial", uisongsel->tag);
  uisongselProcessSongFilter (uisongsel);
  /* pre-populate so that the number of displayable rows can be calculated */
  uisongselPopulateData (uisongsel);

  uidanceSetValue (uisongsel->uidance, -1);

  ssint->callbacks [SONGSEL_CB_SEL_CHG] = callbackInit (
      uisongselSelectionChgCallback, uisongsel, NULL);
  uiTreeViewSetSelectChangedCallback (ssint->songselTree,
      ssint->callbacks [SONGSEL_CB_SEL_CHG]);

  ssint->callbacks [SONGSEL_CB_SZ_CHG] = callbackInitLong (
      uisongselProcessTreeSize, uisongsel);
  uiTreeViewSetSizeChangeCallback (ssint->songselTree,
      ssint->callbacks [SONGSEL_CB_SZ_CHG]);

  uiwcontFree (hbox);
  uiwcontFree (vbox);

  logProcEnd (LOG_PROC, "uisongselBuildUI", "");
  return ssint->vbox;
}

void
uisongselClearData (uisongsel_t *uisongsel)
{
  ss_internal_t   * ssint;

  logProcBegin (LOG_PROC, "uisongselClearData");

  ssint = uisongsel->ssInternalData;

  uiTreeViewValueClear (ssint->songselTree, 0);

  /* having cleared the list, the rows must be re-created */
  uisongselCreateRows (uisongsel);
  uiScrollbarSetPosition (ssint->songselScrollbar, 0.0);
  logProcEnd (LOG_PROC, "uisongselClearData", "");
}

void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  ss_internal_t   * ssint;
  long            idx;
  int             row;
  song_t          * song;
  dbidx_t         dbidx;
  char            * listingFont;
  slist_t         * sellist;
  double          tupper;
  const char      * sscolor = ""; // "#000000";

  logProcBegin (LOG_PROC, "uisongselPopulateData");

  ssint = uisongsel->ssInternalData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  /* re-fetch the count, as the songfilter process may not have been */
  /* processed by this instance */
  uisongsel->dfilterCount = (double) songfilterGetCount (uisongsel->songfilter);

  tupper = uisongsel->dfilterCount;
  uiScrollbarSetUpper (ssint->songselScrollbar, tupper);

  row = 0;
  idx = uisongsel->idxStart;
  while (row < ssint->maxRows) {
    char        colorbuff [200];

    uiTreeViewValueIteratorSet (ssint->songselTree, row);

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
              "<span color=\"%s\">%s</span>", ssint->markcolor, MARK_DISPLAY);
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

      uiTreeViewSetValueEllipsize (ssint->songselTree, SONGSEL_COL_ELLIPSIZE);
      uiTreeViewSetValues (ssint->songselTree,
          SONGSEL_COL_FONT, listingFont,
          SONGSEL_COL_IDX, (treenum_t) idx,
          SONGSEL_COL_SORTIDX, (treenum_t) idx,
          SONGSEL_COL_DBIDX, (treenum_t) dbidx,
          SONGSEL_COL_MARK_MARKUP, colorbuff,
          SONGSEL_COL_SAMESONG_MARKUP, colorbuff,
          TREE_VALUE_END);

      sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
      // ssint->model = model;
      // ssint->iterp = &iter;
      uisongSetDisplayColumns (sellist, song, SONGSEL_COL_MAX,
          uisongselPopulateDataCallback, uisongsel);
    } /* song is not null */

    ++idx;
    ++row;
  }

  uiTreeViewValueIteratorClear (ssint->songselTree);
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
  ss_internal_t  *ssint;
  int             count;

  ssint = uisongsel->ssInternalData;

  count = uiTreeViewSelectGetCount (ssint->songselTree);
  if (count < 1) {
    uiTreeViewSelectFirst (ssint->songselTree);
  }

  return;
}

void
uisongselSetSelection (uisongsel_t *uisongsel, long idx)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;

  if (idx < 0) {
    return;
  }

  uiTreeViewSelectSet (ssint->songselTree, idx);
}

void
uisongselSetSelectionOffset (uisongsel_t *uisongsel, long idx)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;

  if (idx < 0) {
    return;
  }

  uisongselScrollSelection (uisongsel, idx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_DIR_NONE);
  idx -= uisongsel->idxStart;

  uiTreeViewSelectSet (ssint->songselTree, idx);
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
  ss_internal_t  *ssint;
  int             count;
  long            loc = -1;

  ssint = uisongsel->ssInternalData;
  count = uiTreeViewSelectGetCount (ssint->songselTree);
  if (count != 1) {
    return -1;
  }

  loc = uiTreeViewSelectGetIndex (ssint->songselTree);
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
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  if (ssint->selectedBackup != NULL) {
    return;
  }

  ssint->selectedBackup = ssint->selectedList;
  ssint->selectedList = nlistAlloc ("selected-list-save", LIST_ORDERED, NULL);

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
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  if (ssint->selectedBackup == NULL) {
    return;
  }

  nlistFree (ssint->selectedList);
  ssint->selectedList = ssint->selectedBackup;
  ssint->selectedBackup = NULL;
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
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;

  /* if the player is active, disable the button */
  if (active) {
    uiButtonSetState (ssint->buttons [SONGSEL_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiButtonSetState (ssint->buttons [SONGSEL_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
}

nlist_t *
uisongselGetSelectedList (uisongsel_t *uisongsel)
{
  ss_internal_t   * ssint;
  nlist_t         *tlist;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  ssint = uisongsel->ssInternalData;
  tlist = nlistAlloc ("selected-list-dbidx", LIST_UNORDERED, NULL);
  nlistSetSize (tlist, nlistGetCount (ssint->selectedList));
  nlistStartIterator (ssint->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (ssint->selectedList, &iteridx)) >= 0) {
    nlistSetNum (tlist, dbidx, 0);
  }
  nlistSort (tlist);
  return tlist;
}

void
uisongselClearAllUISelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  ssint->inselectchgprocess = true;
  uiTreeViewSelectClearAll (ssint->songselTree);
  ssint->inselectchgprocess = false;
}

void
uisongselSetRequestLabel (uisongsel_t *uisongsel, const char *txt)
{
  ss_internal_t  *ssint;

  if (uisongsel == NULL) {
    return;
  }

  ssint = uisongsel->ssInternalData;
  uiLabelSetText (&ssint->reqQueueLabel, txt);
}

/* internal routines */

static void
uisongselClearSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;

  nlistFree (ssint->selectedList);
  ssint->selectedList = nlistAlloc ("selected-list-clr", LIST_ORDERED, NULL);
  nlistStartIterator (ssint->selectedList, &ssint->selectListIter);
  ssint->selectListKey = -1;
}

static bool
uisongselScrollSelection (uisongsel_t *uisongsel, long idxStart,
    int scrollflag, int direction)
{
  ss_internal_t  *ssint;
  bool            scrolled = false;
  dbidx_t         oidx;
  bool            idxchanged = false;

  ssint = uisongsel->ssInternalData;

  oidx = uisongsel->idxStart;

  if (scrollflag == UISONGSEL_SCROLL_NORMAL &&
      idxStart >= oidx &&
      idxStart < oidx + ssint->maxRows) {
    if (direction == UISONGSEL_DIR_NONE) {
      return false;
    }
    if (direction == UISONGSEL_NEXT) {
      if (idxStart < oidx + ssint->maxRows / 2) {
        return false;
      } else {
        idxchanged = true;
        uisongsel->idxStart += 1;
      }
    }
    if (direction == UISONGSEL_PREVIOUS) {
      if (idxStart >= oidx + ssint->maxRows / 2 - 1) {
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
  uisongselScroll (uisongsel, (double) uisongsel->idxStart);

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
  ss_internal_t   * ssint;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  logProcBegin (LOG_PROC, "uisongselQueueHandler");
  ssint = uisongsel->ssInternalData;
  nlistStartIterator (ssint->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (ssint->selectedList, &iteridx)) >= 0) {
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
  ss_internal_t     * ssint;
  slist_t           *sellist;

  logProcBegin (LOG_PROC, "uisongselInitializeStore");

  ssint = uisongsel->ssInternalData;
  ssint->typelist = mdmalloc (sizeof (int) * SONGSEL_COL_MAX);
  ssint->colcount = 0;
  /* attributes ellipsize/font*/
  ssint->typelist [ssint->colcount++] = TREE_TYPE_ELLIPSIZE;
  ssint->typelist [ssint->colcount++] = TREE_TYPE_STRING;
  /* internal idx/sortidx/dbidx */
  ssint->typelist [ssint->colcount++] = TREE_TYPE_NUM;
  ssint->typelist [ssint->colcount++] = TREE_TYPE_NUM;
  ssint->typelist [ssint->colcount++] = TREE_TYPE_NUM;
  /* fav color/mark color/mark/samesong color */
  ssint->typelist [ssint->colcount++] = TREE_TYPE_STRING;
  ssint->typelist [ssint->colcount++] = TREE_TYPE_STRING;
  if (ssint->colcount != SONGSEL_COL_MAX) {
    fprintf (stderr, "ERR: mismatched SONGSEL_COL_MAX and %d\n", ssint->colcount);
  }

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  uisongAddDisplayTypes (sellist, uisongselInitializeStoreCallback, uisongsel);

  uiTreeViewCreateValueStoreFromList (ssint->songselTree, ssint->colcount, ssint->typelist);
  mdfree (ssint->typelist);

  logProcEnd (LOG_PROC, "uisongselInitializeStore", "");
}

static void
uisongselInitializeStoreCallback (int type, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  ssint->typelist = uiTreeViewAddDisplayType (ssint->typelist, type, ssint->colcount);
  ++ssint->colcount;
}


static void
uisongselCreateRows (uisongsel_t *uisongsel)
{
  ss_internal_t    *ssint;

  logProcBegin (LOG_PROC, "uisongselCreateRows");

  ssint = uisongsel->ssInternalData;
  /* enough pre-allocated rows are needed so that if the windows is */
  /* maximized and the font size is not large, enough rows are available */
  /* to be displayed */
  for (int i = 0; i < STORE_ROWS; ++i) {
    uiTreeViewValueAppend (ssint->songselTree);
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

static bool
uisongselCheckFavChgCallback (void *udata, long col)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;

  logProcBegin (LOG_PROC, "uisongselCheckFavChgCallback");

  ssint = uisongsel->ssInternalData;

  /* double-click processing */
  if (ssint->lastRowDBIdx == uisongsel->lastdbidx &&
      ! mstimeCheck (&ssint->lastRowCheck)) {
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
  mstimeset (&ssint->lastRowCheck, TREE_DOUBLE_CLICK_TIME);
  ssint->lastRowDBIdx = uisongsel->lastdbidx;

  if (col == TREE_NO_COLUMN) {
    logProcEnd (LOG_PROC, "uisongselCheckFavChgCallback", "not-fav-col");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");
  uisongselChangeFavorite (uisongsel, uisongsel->lastdbidx);
  logProcEnd (LOG_PROC, "uisongselCheckFavChgCallback", "");
  return UICB_CONT;
}

static bool
uisongselProcessTreeSize (void *udata, long rows)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;

  if (uisongsel->ispeercall) {
    return UICB_CONT;
  }

  logProcBegin (LOG_PROC, "uisongselProcessTreeSize");

  ssint = uisongsel->ssInternalData;

  ssint->maxRows = rows;
  logMsg (LOG_DBG, LOG_IMPORTANT, "%s max-rows:%d", uisongsel->tag, ssint->maxRows);

  /* the step increment does not work correctly with smooth scrolling */
  /* and it appears there's no easy way to turn smooth scrolling off */
  uiScrollbarSetStepIncrement (ssint->songselScrollbar, 4.0);
  uiScrollbarSetPageIncrement (ssint->songselScrollbar, (double) ssint->maxRows);
  uiScrollbarSetPageSize (ssint->songselScrollbar, (double) ssint->maxRows);

  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: tree size change", uisongsel->tag);
  uisongselPopulateData (uisongsel);

  /* neither queue_draw nor queue_resize on the tree */
  /* does not help with the redraw issue */

  /* this is necessary because gtk on windows has a bug where the */
  /* music manager song-selection does not receive the size-allocate */
  /* signal from gtk */
  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselProcessTreeSize (uisongsel->peers [i], rows);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }

  logProcEnd (LOG_PROC, "uisongselProcessTreeSize", "");
  return UICB_CONT;
}

static bool
uisongselScroll (void *udata, double value)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;
  double          start;
  double          tval;

  logProcBegin (LOG_PROC, "uisongselScroll");

  if (uisongsel == NULL) {
    return UICB_STOP;
  }

  ssint = uisongsel->ssInternalData;
  if (ssint == NULL) {
    return UICB_STOP;
  }
  if (ssint->inselectchgprocess) {
    return UICB_STOP;
  }

  start = floor (value);
  if (start < 0.0) {
    start = 0.0;
  }
  tval = uisongsel->dfilterCount - (double) ssint->maxRows;
  if (tval < 0) {
    tval = 0;
  }
  if (start >= tval) {
    start = tval;
  }
  uisongsel->idxStart = (dbidx_t) start;

  ssint->inscroll = true;

  logMsg (LOG_DBG, LOG_SONGSEL, "%s populate: scroll", uisongsel->tag);
  uisongselPopulateData (uisongsel);
  uiScrollbarSetPosition (ssint->songselScrollbar, value);
  uisongselUpdateSelections (uisongsel);

  ssint->inscroll = false;
  logProcEnd (LOG_PROC, "uisongselScroll", "");
  return UICB_CONT;
}

static void
uisongselUpdateSelections (uisongsel_t *uisongsel)
{
  nlistidx_t    idx;
  nlistidx_t    iteridx;
  ss_internal_t *ssint;

  ssint = uisongsel->ssInternalData;

  /* clear the current selections */
  uisongselClearAllUISelections (uisongsel);

  ssint->inselectchgprocess = true;
  /* set the selections based on the saved selection list */
  nlistStartIterator (ssint->selectedList, &iteridx);
  while ((idx = nlistIterateKey (ssint->selectedList, &iteridx)) >= 0) {
    if (idx >= uisongsel->idxStart &&
        idx < uisongsel->idxStart + ssint->maxRows) {
      uiTreeViewSelectSet (ssint->songselTree, idx - uisongsel->idxStart);

      if (! uisongsel->ispeercall) {
        for (int i = 0; i < uisongsel->peercount; ++i) {
          if (uisongsel->peers [i] == NULL) {
            continue;
          }
          uisongselSetPeerFlag (uisongsel->peers [i], true);
          uisongselSetSelectionOffset (uisongsel->peers [i], idx);
          uisongselSetPeerFlag (uisongsel->peers [i], false);
        }
      }
    }
  }

  ssint->inselectchgprocess = false;
}

static bool
uisongselScrollEvent (void *udata, long dir)
{
  uisongsel_t     *uisongsel = udata;
  int             ndir = UISONGSEL_NEXT;

  logProcBegin (LOG_PROC, "uisongselScrollEvent");
fprintf (stderr, "ssui: scroll-event: dir:%ld\n", dir);

  if (dir == TREE_SCROLL_NEXT) {
    ndir = UISONGSEL_NEXT;
  }
  if (dir == TREE_SCROLL_PREV) {
    ndir = UISONGSEL_PREVIOUS;
  }
  uisongselProcessScroll (uisongsel, ndir, 1);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
  return UICB_STOP;
}

static void
uisongselProcessScroll (uisongsel_t *uisongsel, int dir, int lines)
{
  ss_internal_t  *ssint;
  nlistidx_t      tval;

  ssint = uisongsel->ssInternalData;

  if (dir == UISONGSEL_NEXT) {
    uisongsel->idxStart += lines;
  }
  if (dir == UISONGSEL_PREVIOUS) {
    uisongsel->idxStart -= lines;
  }

  if (uisongsel->idxStart < 0) {
    uisongsel->idxStart = 0;
  }

  tval = (nlistidx_t) uisongsel->dfilterCount - ssint->maxRows;
  if (uisongsel->idxStart > tval) {
    uisongsel->idxStart = tval;
  }

  uisongselScroll (uisongsel, (double) uisongsel->idxStart);

  logProcEnd (LOG_PROC, "uisongselScrollEvent", "");
}

static bool
uisongselKeyEvent (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *ssint;

  if (uisongsel == NULL) {
    return UICB_CONT;
  }

  ssint = uisongsel->ssInternalData;

  ssint->shiftPressed = false;
  ssint->controlPressed = false;
  if (uiKeyIsShiftPressed (ssint->uikey)) {
    ssint->shiftPressed = true;
  }
  if (uiKeyIsControlPressed (ssint->uikey)) {
    ssint->controlPressed = true;
  }

  if (uiKeyIsPressEvent (ssint->uikey) &&
      uiKeyIsAudioPlayKey (ssint->uikey)) {
    uisongselPlayCallback (uisongsel);
  }

  if (uiKeyIsMovementKey (ssint->uikey)) {
    int     dir;
    int     lines;

    dir = UISONGSEL_DIR_NONE;
    lines = 1;

    if (uiKeyIsPressEvent (ssint->uikey)) {
      if (uiKeyIsUpKey (ssint->uikey)) {
        dir = UISONGSEL_PREVIOUS;
      }
      if (uiKeyIsDownKey (ssint->uikey)) {
        dir = UISONGSEL_NEXT;
      }
      if (uiKeyIsPageUpDownKey (ssint->uikey)) {
        lines = ssint->maxRows;
      }

      uisongselMoveSelection (uisongsel, dir, lines, UISONGSEL_MOVE_KEY);
    }

    /* movement keys are handled internally */
    return UICB_STOP;
  }

  return UICB_CONT;
}

static bool
uisongselSelectionChgCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;
  nlist_t           *tlist;
  nlistidx_t        idx;
  nlistidx_t        iteridx;

  ssint = uisongsel->ssInternalData;

  if (ssint->inscroll) {
    return UICB_CONT;
  }
  if (ssint->inselectchgprocess) {
    return UICB_CONT;
  }

  if (ssint->selectedBackup != NULL &&
      uisongsel->dispselType != DISP_SEL_MM) {
    /* if the music manager is currently using the song list */
    /* do not update the selection list for the music manager */
    return UICB_CONT;
  }

  ssint->inselectchgprocess = true;

  /* if neither the control key nor the shift key are pressed */
  /* then this will be a new selection and not a modification */
  tlist = nlistAlloc ("selected-list-chg", LIST_ORDERED, NULL);

  /* clear the selections from the peers first */
  if (! uisongsel->ispeercall) {
    for (int i = 0; i < uisongsel->peercount; ++i) {
      if (uisongsel->peers [i] == NULL) {
        continue;
      }
      uisongselSetPeerFlag (uisongsel->peers [i], true);
      uisongselClearSelections (uisongsel->peers [i]);
      uisongselClearAllUISelections (uisongsel->peers [i]);
      uisongselSetPeerFlag (uisongsel->peers [i], false);
    }
  }

  /* if the shift key is pressed, get the first and the last item */
  /* in the selection list (both, as it is not yet known where */
  /* the new selection is in relation). */
  if (ssint->shiftPressed) {
    ssint->shiftfirstidx = -1;
    ssint->shiftlastidx = -1;
    nlistStartIterator (ssint->selectedList, &iteridx);
    while ((idx = nlistIterateKey (ssint->selectedList, &iteridx)) >= 0) {
      if (ssint->shiftfirstidx == -1) {
        ssint->shiftfirstidx = idx;
      }
      ssint->shiftlastidx = idx;
    }
  }

  /* if the control-key is pressed, add any current */
  /* selection that is not in view to the new selection list */
  if (ssint->controlPressed) {
    nlistStartIterator (ssint->selectedList, &iteridx);
    while ((idx = nlistIterateKey (ssint->selectedList, &iteridx)) >= 0) {
      if (idx < uisongsel->idxStart ||
          idx > uisongsel->idxStart + ssint->maxRows - 1) {
        nlistSetNum (tlist, idx, nlistGetNum (ssint->selectedList, idx));
      }
    }
  }

  nlistFree (ssint->selectedList);
  ssint->selectedList = tlist;

  /* and now process the selections from gtk */
  uiTreeViewSelectForeach (ssint->songselTree,
      ssint->callbacks [SONGSEL_CB_SELECT_PROCESS]);

  /* any time the selection is changed, re-start the edit iterator */
  nlistStartIterator (ssint->selectedList, &ssint->selectListIter);
  ssint->selectListKey = nlistIterateKey (ssint->selectedList, &ssint->selectListIter);

  /* update the current song-selection's selections */
  uisongselUpdateSelections (uisongsel);

  /* process the peers after the selections have been made */
  if (! uisongsel->ispeercall) {
    for (int i = 0; i < uisongsel->peercount; ++i) {
      if (uisongsel->peers [i] == NULL) {
        continue;
      }
      uisongselSetPeerFlag (uisongsel->peers [i], true);
      uisongselScroll (uisongsel->peers [i], (double) uisongsel->idxStart);
      uisongselSetPeerFlag (uisongsel->peers [i], false);
    }
  }

  if (uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    /* the song editor points to the first selected */
    dbidx = nlistGetNum (ssint->selectedList, ssint->selectListKey);
    if (dbidx >= 0) {
      callbackHandlerLong (uisongsel->newselcb, dbidx);
    }
  }

  ssint->inselectchgprocess = false;
  return UICB_CONT;
}

static bool
uisongselProcessSelection (void *udata, long row)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;
  long              idx;
  dbidx_t           dbidx = -1;

  ssint = uisongsel->ssInternalData;

  idx = uiTreeViewSelectForeachGetValue (ssint->songselTree, SONGSEL_COL_IDX);

  if (ssint->shiftPressed) {
    nlistidx_t    beg = 0;
    nlistidx_t    end = -1;

    if (idx <= ssint->shiftfirstidx) {
      beg = idx;
      end = ssint->shiftfirstidx;
    }
    if (idx >= ssint->shiftlastidx) {
      beg = ssint->shiftlastidx;
      end = idx;
    }

    for (nlistidx_t i = beg; i <= end; ++i) {
      dbidx = songfilterGetByIdx (uisongsel->songfilter, idx);
      nlistSetNum (ssint->selectedList, i, dbidx);
    }
  } else {
    dbidx = uiTreeViewSelectForeachGetValue (ssint->songselTree, SONGSEL_COL_DBIDX);
    nlistSetNum (ssint->selectedList, idx, dbidx);
  }

  uisongsel->lastdbidx = dbidx;

  return UICB_CONT;
}

static void
uisongselPopulateDataCallback (int col, long num, const char *str, void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  uitreedispSetDisplayColumn (ssint->songselTree, col, num, str);
}


static void
uisongselMoveSelection (void *udata, int where, int lines, int moveflag)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;
  int             count;
  long            loc = -1;
  dbidx_t         nidx;
  bool            scrolled = false;

  ssint = uisongsel->ssInternalData;

  count = nlistGetCount (ssint->selectedList);

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
      nlistStartIterator (ssint->selectedList, &ssint->selectListIter);
      ssint->selectListKey = nlistIterateKey (ssint->selectedList, &ssint->selectListIter);
    }
    if (where == UISONGSEL_NEXT) {
      nlistidx_t  pkey;
      nlistidx_t  piter;

      pkey = ssint->selectListKey;
      piter = ssint->selectListIter;

      ssint->selectListKey = nlistIterateKey (ssint->selectedList, &ssint->selectListIter);
      if (ssint->selectListKey < 0) {
        /* remain on the current selection, keep the iterator intact */
        ssint->selectListKey = pkey;
        ssint->selectListIter = piter;
      }
    }
    if (where == UISONGSEL_PREVIOUS) {
      ssint->selectListKey = nlistIterateKeyPrevious (ssint->selectedList, &ssint->selectListIter);
      if (ssint->selectListKey < 0) {
        /* reset to the beginning */
        ssint->selectListKey = nlistIterateKey (ssint->selectedList, &ssint->selectListIter);
      }
    }

    uisongselScrollSelection (uisongsel, ssint->selectListKey, UISONGSEL_SCROLL_NORMAL, where);
    if (uisongsel->newselcb != NULL) {
      dbidx_t   dbidx;

      dbidx = nlistGetNum (ssint->selectedList, ssint->selectListKey);
      if (dbidx >= 0) {
        callbackHandlerLong (uisongsel->newselcb, dbidx);
      }
    }
  }

  if (count == 1) {
    uiTreeViewSelectCurrent (ssint->songselTree);
    nidx = uisongselGetSelectLocation (uisongsel);
    loc = nidx - uisongsel->idxStart;

    /* there's only one selected, clear them all */
    uisongselClearSelections (uisongsel);
    uisongselClearAllUISelections (uisongsel);

    if (where == UISONGSEL_FIRST) {
      nidx = 0;
      loc = 0;
      scrolled = uisongselScrollSelection (uisongsel, 0, UISONGSEL_SCROLL_NORMAL, UISONGSEL_DIR_NONE);
      uiTreeViewSelectFirst (ssint->songselTree);
    }
    if (where == UISONGSEL_NEXT) {
      while (lines > 0) {
        ++nidx;
        scrolled = uisongselScrollSelection (uisongsel, nidx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_NEXT);
        if (! scrolled) {
          long    idx;

          idx = uiTreeViewGetValue (ssint->songselTree, SONGSEL_COL_IDX);
          if (loc < ssint->maxRows - 1 &&
              idx < uisongsel->dfilterCount - 1) {
            /* any current selection must be re-cleared */
            uisongselClearAllUISelections (uisongsel);
            uiTreeViewSelectNext (ssint->songselTree);
            ++loc;
          } else {
            break;
          }
        }
        --lines;
      }
    }
    if (where == UISONGSEL_PREVIOUS) {
      while (lines > 0) {
        --nidx;
        scrolled = uisongselScrollSelection (uisongsel, nidx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_PREVIOUS);
        if (! scrolled) {
          if (loc > 0) {
            /* any current selection must be re-cleared */
            uisongselClearAllUISelections (uisongsel);
            uiTreeViewSelectPrevious (ssint->songselTree);
            --loc;
          } else {
            break;
          }
        }
        --lines;
      }
    }

    /* if the scroll was bumped, the iterator is still pointing to the same */
    /* row (but a new dbidx), re-select it */
    /* if the iter was moved, it is pointing at the new selection */
    /* if the iter was not moved, the original must be re-selected */
    if (where != UISONGSEL_FIRST) {
      uiTreeViewSelectSet (ssint->songselTree, loc);
    }
    uiTreeViewSelectCurrent (ssint->songselTree);
  }
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

