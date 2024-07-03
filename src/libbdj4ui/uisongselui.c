/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "colorutils.h"
#include "conn.h"
#include "genre.h"
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
#include "uivirtlist.h"
#include "uivlutil.h"

enum {
  SONGSEL_COL_DBIDX,
  SONGSEL_COL_MARK,
  SONGSEL_COL_MAX,
};

enum {
  UISONGSEL_FIRST,
  UISONGSEL_NEXT,
  UISONGSEL_PREVIOUS,
  UISONGSEL_DIR_NONE,
  UISONGSEL_MOVE_SE,
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
  SONGSEL_CB_KEYB,
  SONGSEL_CB_SEL_CHG,
  SONGSEL_CB_SELECT_PROCESS,
  SONGSEL_CB_ROW_CLICK,
  SONGSEL_CB_RIGHT_CLICK,
  SONGSEL_CB_MAX,
};

enum {
  SONGSEL_W_BUTTON_EDIT,
  SONGSEL_W_BUTTON_FILTER,
  SONGSEL_W_BUTTON_PLAY,
  SONGSEL_W_BUTTON_QUEUE,
  SONGSEL_W_BUTTON_SELECT,
  SONGSEL_W_MAIN_VBOX,
  SONGSEL_W_SCROLL_WIN,
  SONGSEL_W_REQ_QUEUE,
  SONGSEL_W_SCROLLBAR,
  SONGSEL_W_MAX,
};

typedef struct ss_internal {
  callback_t          *callbacks [SONGSEL_CB_MAX];
  uiwcont_t           *wcont [SONGSEL_W_MAX];
  uisongsel_t         *uisongsel;
  uivirtlist_t        *uivl;
  genre_t             *genres;
  slist_t             *sellist;
  slist_t             *sscolorlist;
  /* other data */
//  int                 maxRows;
  nlist_t             *selectedBackup;
  nlist_t             *selectedList;
  nlistidx_t          selectListIter;
  nlistidx_t          selectListKey;
//  int                 *typelist;
  int                 colcount;
  const char          *marktext;
  /* for shift-click */
  nlistidx_t          shiftfirstidx;
  nlistidx_t          shiftlastidx;
  /* for double-click checks */
//  dbidx_t             lastRowDBIdx;
  bool                inchange : 1;
  bool                inscroll : 1;
  bool                inselectchgprocess : 1;
  bool                rightclick : 1;
} ss_internal_t;

//static void uisongselClearSelections (uisongsel_t *uisongsel);
//static bool uisongselScrollSelection (uisongsel_t *uisongsel, dbidx_t idxStart, int scrollflag, int dir);
static bool uisongselQueueCallback (void *udata);
static void uisongselQueueHandler (uisongsel_t *uisongsel, int mqidx, int action);
//static void uisongselInitializeStore (uisongsel_t *uisongsel);
//static void uisongselInitializeStoreCallback (int type, void *udata);
//static void uisongselCreateRows (uisongsel_t *uisongsel);
static void uisongselProcessSongFilter (uisongsel_t *uisongsel);

static bool uisongselRowClickCallback (void *udata, int32_t col);
static bool uisongselRightClickCallback (void *udata);

//static bool uisongselProcessTreeSize (void *udata, int32_t rows);
//static bool uisongselScroll (void *udata, double value);
//static void uisongselUpdateSelections (uisongsel_t *uisongsel);
//static bool uisongselScrollEvent (void *udata, int32_t dir);
//static void uisongselProcessScroll (uisongsel_t *uisongsel, int dir, int lines);
static bool uisongselKeyEvent (void *udata);
static bool uisongselSelectionChgCallback (void *udata);
static bool uisongselProcessSelection (void *udata, int32_t row);
//static void uisongselPopulateDataCallback (int col, long num, const char *str, void *udata);

// static void uisongselMoveSelection (void *udata, int where, int lines, int moveflag);

static bool uisongselUIDanceSelectCallback (void *udata, int32_t idx, int32_t count);
static bool uisongselSongEditCallback (void *udata);
static void uisongselFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = mdmalloc (sizeof (ss_internal_t));
  ssint->uivl = NULL;
  ssint->uisongsel = uisongsel;
  ssint->sscolorlist = slistAlloc ("ss-colors", LIST_ORDERED, NULL);
  ssint->inchange = false;
  ssint->inscroll = false;
  ssint->inselectchgprocess = false;
  ssint->rightclick = false;
  ssint->selectedBackup = NULL;
  ssint->selectedList = nlistAlloc ("selected-list", LIST_ORDERED, NULL);
  nlistStartIterator (ssint->selectedList, &ssint->selectListIter);
  ssint->selectListKey = -1;
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    ssint->callbacks [i] = NULL;
  }
  for (int i = 0; i < SONGSEL_W_MAX; ++i) {
    ssint->wcont [i] = NULL;
  }
  ssint->marktext = bdjoptGetStr (OPT_P_UI_MARK_TEXT);
//  ssint->lastRowDBIdx = -1;
  ssint->genres = bdjvarsdfGet (BDJVDF_GENRES);

//  ssint->callbacks [SONGSEL_CB_KEYB] = callbackInit (
//      uisongselKeyEvent, uisongsel, NULL);
  ssint->callbacks [SONGSEL_CB_SELECT_PROCESS] = callbackInitI (
      uisongselProcessSelection, uisongsel);

  uisongsel->ssInternalData = ssint;
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  ss_internal_t    *ssint;

  if (uisongsel->ssInternalData == NULL) {
    return;
  }

  ssint = uisongsel->ssInternalData;

  nlistFree (ssint->selectedBackup);
  nlistFree (ssint->selectedList);
  slistFree (ssint->sscolorlist);
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    callbackFree (ssint->callbacks [i]);
  }
  for (int i = 0; i < SONGSEL_W_MAX; ++i) {
    uiwcontFree (ssint->wcont [i]);
  }
  uivlFree (ssint->uivl);
  mdfree (ssint);
  uisongsel->ssInternalData = NULL;
}

uiwcont_t *
uisongselBuildUI (uisongsel_t *uisongsel, uiwcont_t *parentwin)
{
  ss_internal_t     *ssint;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *hbox;
  slist_t           *sellist;
  char              tbuff [200];
  int               startcol;
  uivirtlist_t      *uivl;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;
  uisongsel->windowp = parentwin;

  ssint->wcont [SONGSEL_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (ssint->wcont [SONGSEL_W_MAIN_VBOX]);
  uiWidgetExpandVert (ssint->wcont [SONGSEL_W_MAIN_VBOX]);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (ssint->wcont [SONGSEL_W_MAIN_VBOX], hbox);

  /* The side-by-side song selection does not need a select button, */
  /* as it has the left-arrow button.  Saves real estate. */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL) {
    /* CONTEXT: song-selection: select a song to be added to the song list */
    strlcpy (tbuff, _("Select"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_SELECT] = callbackInit (
        uisongselSelectCallback, uisongsel, "songsel: select");
    uiwidgetp = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_SELECT], tbuff, NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_SELECT] = uiwidgetp;
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_SBS_SONGSEL) {
    ssint->callbacks [SONGSEL_CB_EDIT_LOCAL] = callbackInit (
        uisongselSongEditCallback, uisongsel, "songsel: edit");
    uiwidgetp = uiCreateButton (ssint->callbacks [SONGSEL_CB_EDIT_LOCAL],
        /* CONTEXT: song-selection: edit the selected song */
        _("Edit"), "button_edit");
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_EDIT] = uiwidgetp;
  }

  if (uisongsel->dispselType == DISP_SEL_REQUEST) {
    /* CONTEXT: (verb) song-selection: queue a song to be played */
    strlcpy (tbuff, _("Queue"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_QUEUE] = callbackInit (
        uisongselQueueCallback, uisongsel, "songsel: queue");
    uiwidgetp = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_QUEUE], tbuff, NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_QUEUE] = uiwidgetp;

    uiwidgetp = uiCreateLabel ("");
    uiWidgetAddClass (uiwidgetp, DARKACCENT_CLASS);
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_REQ_QUEUE] = uiwidgetp;
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_SBS_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    /* CONTEXT: song-selection: tooltip: play the selected songs */
    strlcpy (tbuff, _("Play"), sizeof (tbuff));
    ssint->callbacks [SONGSEL_CB_PLAY] = callbackInit (
        uisongselPlayCallback, uisongsel, "songsel: play");
    uiwidgetp = uiCreateButton (
        ssint->callbacks [SONGSEL_CB_PLAY], tbuff, "button_play");
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_PLAY] = uiwidgetp;
  }

  ssint->callbacks [SONGSEL_CB_DANCE_SEL] = callbackInitII (
      uisongselUIDanceSelectCallback, uisongsel);
  uisongsel->uidance = uidanceCreate (hbox, parentwin,
      /* CONTEXT: song-selection: filter: all dances are selected */
      UIDANCE_ALL_DANCES, _("All Dances"), UIDANCE_PACK_END, 1);
  uidanceSetCallback (uisongsel->uidance,
      ssint->callbacks [SONGSEL_CB_DANCE_SEL]);

  ssint->callbacks [SONGSEL_CB_FILTER] = callbackInit (
      uisfDialog, uisongsel->uisongfilter, "songsel: filters");
  uiwidgetp = uiCreateButton (
      ssint->callbacks [SONGSEL_CB_FILTER],
      /* CONTEXT: song-selection: tooltip: a button that starts the filters (narrowing down song selections) dialog */
      _("Filter Songs"), "button_filter");
  uiBoxPackEnd (hbox, uiwidgetp);
  ssint->wcont [SONGSEL_W_BUTTON_FILTER] = uiwidgetp;

  uiwcontFree (hbox);

  if (uisongsel->dispselType == DISP_SEL_SBS_SONGSEL) {
    /* need a filler box to match the musicq */
    hbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (ssint->wcont [SONGSEL_W_MAIN_VBOX], hbox);

    uiwidgetp = uiCreateLabel (" ");
    uiWidgetSetMarginBottom (uiwidgetp, 5);
    uiBoxPackStart (hbox, uiwidgetp);
    uiwcontFree (uiwidgetp);
    uiwcontFree (hbox);
  }

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (ssint->wcont [SONGSEL_W_MAIN_VBOX], hbox);

  sellist = dispselGetList (uisongsel->dispsel, uisongsel->dispselType);
  ssint->sellist = sellist;

  uivl = uivlCreate (uisongsel->tag, NULL, hbox, 5, 400, VL_ENABLE_KEYS);
  ssint->uivl = uivl;
  uivlSetUseListingFont (uivl);
  uivlSetAllowMultiple (uivl);
  uivlSetAllowDoubleClick (uivl);

  startcol = SONGSEL_COL_MAX;
  ssint->colcount = SONGSEL_COL_MAX + slistGetCount (sellist);
  uivlSetNumColumns (uivl, ssint->colcount);

  uivlMakeColumn (uivl, "dbidx", SONGSEL_COL_DBIDX, VL_TYPE_INTERNAL_NUMERIC);
  uivlMakeColumn (uivl, "mark", SONGSEL_COL_MARK, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, SONGSEL_COL_MARK, VL_COL_WIDTH_GROW_ONLY);

  uivlAddDisplayColumns (uivl, sellist, startcol);
  uivlSetRowFillCallback (uivl, uisongselFillRow, ssint);
  uivlSetNumRows (uivl, uisongsel->numrows);

  uivlDisplay (uivl);

  ssint->callbacks [SONGSEL_CB_ROW_CLICK] = callbackInitI (
        uisongselRowClickCallback, uisongsel);
//  uiTreeViewSetRowActivatedCallback (uiwidgetp,
//        ssint->callbacks [SONGSEL_CB_ROW_CLICK]);

  ssint->callbacks [SONGSEL_CB_RIGHT_CLICK] = callbackInit (
        uisongselRightClickCallback, uisongsel, NULL);
//  uiTreeViewSetButton3Callback (uiwidgetp,
//        ssint->callbacks [SONGSEL_CB_RIGHT_CLICK]);

//  ssint->callbacks [SONGSEL_CB_SCROLL_EVENT] = callbackInitI (
//        uisongselScrollEvent, uisongsel);
//  uiTreeViewSetScrollEventCallback (uiwidgetp,
//        ssint->callbacks [SONGSEL_CB_SCROLL_EVENT]);

  uisongselProcessSongFilter (uisongsel);
  uidanceSetKey (uisongsel->uidance, -1);

  ssint->callbacks [SONGSEL_CB_SEL_CHG] = callbackInit (
      uisongselSelectionChgCallback, uisongsel, NULL);
//  uiTreeViewSetSelectChangedCallback (ssint->wcont [SONGSEL_W_TREE],
//      ssint->callbacks [SONGSEL_CB_SEL_CHG]);

  uiwcontFree (hbox);

  logProcEnd ("");
  return ssint->wcont [SONGSEL_W_MAIN_VBOX];
}


void
uisongselPopulateData (uisongsel_t *uisongsel)
{
  ss_internal_t   * ssint;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

  /* re-fetch the count, as the songfilter process may not have been */
  /* processed by this instance */
  uisongsel->numrows = songfilterGetCount (uisongsel->songfilter);
  uivlSetNumRows (ssint->uivl, uisongsel->numrows);
  uivlPopulate (ssint->uivl);

  logProcEnd ("");
}

bool
uisongselSelectCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel select");
  /* only the song selection and side-by-side song selection have */
  /* a select button to queue to the song list */
  uisongselQueueHandler (uisongsel, UISONGSEL_MQ_NOTSET, UISONGSEL_QUEUE);
  /* don't clear the selected list or the displayed selections */
  /* it's confusing for the user */
  return UICB_CONT;
}

void
uisongselSetSelectionOffset (uisongsel_t *uisongsel, dbidx_t idx)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;

  if (idx < 0) {
    return;
  }

//  uisongselScrollSelection (uisongsel, idx, UISONGSEL_SCROLL_NORMAL, UISONGSEL_DIR_NONE);
  idx -= uisongsel->idxStart;

//  uiTreeViewSelectSet (ssint->wcont [SONGSEL_W_TREE], idx);
}

bool
uisongselNextSelection (void *udata)
{
//  uisongselMoveSelection (udata, UISONGSEL_NEXT, 1, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

bool
uisongselPreviousSelection (void *udata)
{
//  uisongselMoveSelection (udata, UISONGSEL_PREVIOUS, 1, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

bool
uisongselFirstSelection (void *udata)
{
//  uisongselMoveSelection (udata, UISONGSEL_FIRST, 0, UISONGSEL_MOVE_SE);
  return UICB_CONT;
}

nlistidx_t
uisongselGetSelectLocation (uisongsel_t *uisongsel)
{
  ss_internal_t   *ssint;
  int             count;
  nlistidx_t      nidx = -1;
  nlistidx_t      iteridx;

  ssint = uisongsel->ssInternalData;
  count = nlistGetCount (ssint->selectedList);
  if (count != 1) {
    return -1;
  }

  /* get the select location from the selected list, not from on-screen */
  nlistStartIterator (ssint->selectedList, &iteridx);
  nidx = nlistIterateKey (ssint->selectedList, &iteridx);
  return nidx;
}

bool
uisongselApplySongFilter (void *udata)
{
  uisongsel_t *uisongsel = udata;

  uisongsel->numrows = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;

  /* the call to cleardata() will remove any selections */
  /* afterwards, make sure something is selected */
//  uisongselClearData (uisongsel);
//  uisongselClearSelections (uisongsel);
  uisongselPopulateData (uisongsel);

  /* the song filter has been processed, the peers need to be populated */

  /* if the song selection is displaying something else, do not */
  /* update the peers */
  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
//    uisongselClearData (uisongsel->peers [i]);
//    uisongselClearSelections (uisongsel->peers [i]);
    uisongselPopulateData (uisongsel->peers [i]);
  }

  /* set the selection after the populate is done */

//  uisongselSetDefaultSelection (uisongsel);

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
uisongselDanceSelectCallback (void *udata, int32_t danceIdx)
{
  uisongsel_t *uisongsel = udata;

  uidanceSetKey (uisongsel->uidance, danceIdx);

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
//  uisongselScrollSelection (uisongsel, uisongsel->idxStart, UISONGSEL_SCROLL_FORCE, UISONGSEL_DIR_NONE);

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

  /* only song selection, side-by-side song sel and music manager */
  /* have a play button */
  /* use the hidden manage playback music queue */
  mqidx = MUSICQ_MNG_PB;
  /* the manageui callback clears the queue and plays */
  /* note that only the first of multiple selections */
  /* will use 'play', and the rest will be queued */
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
    uiWidgetSetState (ssint->wcont [SONGSEL_W_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiWidgetSetState (ssint->wcont [SONGSEL_W_BUTTON_PLAY], UIWIDGET_ENABLE);
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

#if 0
void
uisongselClearAllUISelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
//  uiTreeViewSelectClearAll (ssint->wcont [SONGSEL_W_TREE]);
}
#endif

void
uisongselSetRequestLabel (uisongsel_t *uisongsel, const char *txt)
{
  ss_internal_t  *ssint;

  if (uisongsel == NULL) {
    return;
  }

  ssint = uisongsel->ssInternalData;
  uiLabelSetText (ssint->wcont [SONGSEL_W_REQ_QUEUE], txt);
}

/* internal routines */


static bool
uisongselQueueCallback (void *udata)
{
  uisongsel_t *uisongsel = udata;

  /* queues from the request listing to a music queue in the player */
  uisongselQueueHandler (uisongsel, UISONGSEL_MQ_NOTSET, UISONGSEL_QUEUE);
  return UICB_CONT;
}

static void
uisongselQueueHandler (uisongsel_t *uisongsel, int mqidx, int action)
{
  ss_internal_t   * ssint;
  nlistidx_t      iteridx;
  dbidx_t         dbidx;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;


  nlistStartIterator (ssint->selectedList, &iteridx);
  while ((dbidx = nlistIterateValueNum (ssint->selectedList, &iteridx)) >= 0) {
    if (action == UISONGSEL_QUEUE) {
      uisongselQueueProcess (uisongsel, dbidx);
    }
    if (action == UISONGSEL_PLAY) {
      uisongselPlayProcess (uisongsel, dbidx, mqidx);
    }
    action = UISONGSEL_QUEUE;
  }

  logProcEnd ("");
  return;
}

/* count is not used */
static bool
uisongselUIDanceSelectCallback (void *udata, int32_t idx, int32_t count)
{
  uisongsel_t *uisongsel = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: dance select");
  uisongselDanceSelectHandler (uisongsel, idx);
  return UICB_CONT;
}

static void
uisongselProcessSongFilter (uisongsel_t *uisongsel)
{
  /* initial song filter process */
  uisongsel->numrows = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
}

static bool
uisongselRowClickCallback (void *udata, int32_t col)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

  /* double-click processing */
  if (0) {
//  if (ssint->lastRowDBIdx == uisongsel->lastdbidx &&
//      ! mstimeCheck (&ssint->lastRowCheck)) {
    /* double-click in the song selection or side-by-side */
    /* song selection adds the song to the song list */
    if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
        uisongsel->dispselType == DISP_SEL_SBS_SONGSEL) {
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
//  ssint->lastRowDBIdx = uisongsel->lastdbidx;

  if (col == VL_COL_UNKNOWN) {
    logProcEnd ("not-fav-col");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");
  uisongselChangeFavorite (uisongsel, uisongsel->lastdbidx);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uisongselRightClickCallback (void *udata)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;
  ssint->rightclick = true;

  logProcEnd ("");
  return UICB_CONT;
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

//  if (uiEventIsKeyPressEvent (ssint->wcont [SONGSEL_W_KEY_HNDLR]) &&
//      uiEventIsAudioPlayKey (ssint->wcont [SONGSEL_W_KEY_HNDLR])) {
//    uisongselPlayCallback (uisongsel);
//  }

  return UICB_CONT;
}

static bool
uisongselSelectionChgCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;
//  nlist_t           *tlist;
//  nlistidx_t        idx;
//  nlistidx_t        iteridx;

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

  /* process the peers after the selections have been made */
  if (! uisongsel->ispeercall) {
    for (int i = 0; i < uisongsel->peercount; ++i) {
      if (uisongsel->peers [i] == NULL) {
        continue;
      }
      uisongselSetPeerFlag (uisongsel->peers [i], true);
//      uisongselScroll (uisongsel->peers [i], (double) uisongsel->idxStart);
      uisongselSetPeerFlag (uisongsel->peers [i], false);
    }
  }

  if (uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    /* the song editor points to the first selected */
    dbidx = nlistGetNum (ssint->selectedList, ssint->selectListKey);
    if (dbidx >= 0) {
      callbackHandlerI (uisongsel->newselcb, dbidx);
    }
  }

  ssint->inselectchgprocess = false;
  ssint->rightclick = false;
  return UICB_CONT;
}

static bool
uisongselProcessSelection (void *udata, int32_t row)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;
  nlistidx_t        idx = 0;
  dbidx_t           dbidx = -1;

  ssint = uisongsel->ssInternalData;

//  idx = uiTreeViewSelectForeachGetValue (ssint->wcont [SONGSEL_W_TREE],
//      SONGSEL_COL_IDX);

  {
    dbidx = 0; // ### temp
//    dbidx = uiTreeViewSelectForeachGetValue (ssint->wcont [SONGSEL_W_TREE],
//        SONGSEL_COL_DBIDX);
    nlistSetNum (ssint->selectedList, idx, dbidx);
    if (ssint->rightclick) {
      song_t      *song;
      nlistidx_t  genreidx;
      bool        clflag;

      song = dbGetByIdx (uisongsel->musicdb, dbidx);
      genreidx = songGetNum (song, TAG_GENRE);
      clflag = genreGetClassicalFlag (ssint->genres, genreidx);
      if (clflag) {
        char        work [200];
        nlistidx_t  end;

        songGetClassicalWork (song, work, sizeof (work));
        if (*work) {
          char    twork [200];

          end = (nlistidx_t) uisongsel->numrows;
          for (nlistidx_t i = idx + 1; i < end; ++i) {
            dbidx = songfilterGetByIdx (uisongsel->songfilter, i);
            song = dbGetByIdx (uisongsel->musicdb, dbidx);
            genreidx = songGetNum (song, TAG_GENRE);
            clflag = genreGetClassicalFlag (ssint->genres, genreidx);
            if (! clflag) {
              break;
            }

            songGetClassicalWork (song, twork, sizeof (twork));
            if (*twork && strcmp (work, twork) == 0) {
              nlistSetNum (ssint->selectedList, i, dbidx);
            } else {
              break;
            }
          }
        }
      }
    }
  }

  uisongsel->lastdbidx = dbidx;

  return UICB_CONT;
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
    callbackHandlerI (uisongsel->newselcb, dbidx);
  }
  return callbackHandler (uisongsel->editcb);
}

static void
uisongselFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  ss_internal_t       *ssint = udata;
  uisongsel_t         *uisongsel = ssint->uisongsel;
  song_t              *song;
  nlist_t             *tdlist;
  slistidx_t          seliteridx;
  const char          *markstr;
  const char          *sscolor = NULL;
  dbidx_t             dbidx;


  dbidx = songfilterGetByIdx (uisongsel->songfilter, rownum);
  song = dbGetByIdx (uisongsel->musicdb, dbidx);
  ssint->inchange = true;

  uivlSetRowColumnNum (ssint->uivl, rownum, SONGSEL_COL_DBIDX, dbidx);

  markstr = "";

  if (song != NULL) {
    if (uisongsel->dispselType != DISP_SEL_MM &&
        uisongsel->songlistdbidxlist != NULL) {
      /* check and see if the song is in the song list */
      if (nlistGetNum (uisongsel->songlistdbidxlist, dbidx) >= 0) {
        markstr = ssint->marktext;
        sscolor = MARK_CLASS;
      }
    }

    if (uisongsel->dispselType == DISP_SEL_MM) {
      sscolor = samesongGetColorByDBIdx (uisongsel->samesong, dbidx);
      if (sscolor != NULL) {
        if (slistGetNum (ssint->sscolorlist, sscolor) < 0) {
          slistSetNum (ssint->sscolorlist, sscolor, 1);
          uiLabelAddClass (sscolor + 1, sscolor);
        }
        markstr = ssint->marktext;
        /* skip the leading # for the class name */
        sscolor = sscolor + 1;
      }
    }
  }

  uivlSetRowColumnStr (ssint->uivl, rownum, SONGSEL_COL_MARK, markstr);
  if (sscolor != NULL) {
    uivlSetRowColumnClass (ssint->uivl, rownum, SONGSEL_COL_MARK, sscolor);
  }

  tdlist = uisongGetDisplayList (ssint->sellist, NULL, song);
  slistStartIterator (ssint->sellist, &seliteridx);
  for (int colidx = SONGSEL_COL_MAX; colidx < ssint->colcount; ++colidx) {
    int         tagidx;
    const char  *str;

    tagidx = slistIterateValueNum (ssint->sellist, &seliteridx);
    str = nlistGetStr (tdlist, tagidx);
    if (str == NULL) {
      str = "";
    }
    uivlSetRowColumnStr (ssint->uivl, rownum, colidx, str);

    if (tagidx == TAG_FAVORITE) {
      songfav_t   *songfav;
      int         favidx;
      const char  *name;

      songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
      favidx = songGetNum (song, TAG_FAVORITE);
      name = songFavoriteGetStr (songfav, favidx, SONGFAV_NAME);
      uivlSetRowColumnClass (ssint->uivl, rownum, colidx, name);
    }
  }
  nlistFree (tdlist);

  ssint->inchange = false;
}
