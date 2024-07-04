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
  nlist_t             *selectedBackup;
  nlistidx_t          vlSelectIter;
  nlistidx_t          selectListKey;
  int                 colcount;
  int                 favcolumn;
  const char          *marktext;
  /* for shift-click */
  nlistidx_t          shiftfirstidx;
  nlistidx_t          shiftlastidx;
  bool                inchange : 1;
  bool                inselectchgprocess : 1;
} ss_internal_t;

static bool uisongselQueueCallback (void *udata);
static void uisongselQueueHandler (uisongsel_t *uisongsel, int mqidx, int action);
static void uisongselProcessSongFilter (uisongsel_t *uisongsel);

static void uisongselDoubleClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uisongselRowClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uisongselRightClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

static bool uisongselKeyEvent (void *udata);
static void uisongselProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

static void uisongselMoveSelection (void *udata, int where);

static bool uisongselUIDanceSelectCallback (void *udata, int32_t idx, int32_t count);
static bool uisongselSongEditCallback (void *udata);
static void uisongselFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uisongselCopySelectList (uisongsel_t *uisongsel, uisongsel_t *peer);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = mdmalloc (sizeof (ss_internal_t));
  ssint->uivl = NULL;
  ssint->uisongsel = uisongsel;
  ssint->sscolorlist = slistAlloc ("ss-colors", LIST_ORDERED, NULL);
  ssint->inchange = false;
  ssint->inselectchgprocess = false;
  ssint->selectedBackup = NULL;
  ssint->selectListKey = -1;
  ssint->favcolumn = -1;
  for (int i = 0; i < SONGSEL_CB_MAX; ++i) {
    ssint->callbacks [i] = NULL;
  }
  for (int i = 0; i < SONGSEL_W_MAX; ++i) {
    ssint->wcont [i] = NULL;
  }
  ssint->marktext = bdjoptGetStr (OPT_P_UI_MARK_TEXT);
  ssint->genres = bdjvarsdfGet (BDJVDF_GENRES);

  ssint->callbacks [SONGSEL_CB_KEYB] = callbackInit (
      uisongselKeyEvent, uisongsel, NULL);
  uivlSetKeyCallback (ssint->uivl, ssint->callbacks [SONGSEL_CB_KEYB]);

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
  int               colidx;
  int               tagidx;
  slistidx_t        iteridx;
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

  colidx = 0;
  slistStartIterator (sellist, &iteridx);
  while ((tagidx = slistIterateValueNum (sellist, &iteridx)) >= 0) {
    if (tagidx == TAG_FAVORITE) {
      ssint->favcolumn = SONGSEL_COL_MAX + colidx;
      break;
    }
    ++colidx;
  }

  uivl = uivlCreate (uisongsel->tag, NULL, hbox, 10, 300, VL_ENABLE_KEYS);
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

  uivlSetDoubleClickCallback (ssint->uivl, uisongselDoubleClickCallback, uisongsel);
  uivlSetRowClickCallback (ssint->uivl, uisongselRowClickCallback, uisongsel);
  uivlSetRightClickCallback (ssint->uivl, uisongselRightClickCallback, uisongsel);
  uivlSetSelectChgCallback (ssint->uivl, uisongselProcessSelectChg, uisongsel);

  uisongselProcessSongFilter (uisongsel);
  uidanceSetKey (uisongsel->uidance, -1);

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

nlistidx_t
uisongselGetSelectLocation (uisongsel_t *uisongsel)
{
  ss_internal_t   *ssint;
  int             count;
  nlistidx_t      nidx = -1;

  ssint = uisongsel->ssInternalData;
  count = uivlSelectionCount (ssint->uivl);
  if (count != 1) {
    return -1;
  }

  uivlStartSelectionIterator (ssint->uivl, &ssint->vlSelectIter);
  nidx = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);
  return nidx;
}

bool
uisongselApplySongFilter (void *udata)
{
  uisongsel_t *uisongsel = udata;

  uisongsel->numrows = songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;

  /* the call to cleardata() will remove any selections */
  /* afterwards, make sure something is selected */
  uisongselPopulateData (uisongsel);

  /* the song filter has been processed, the peers need to be populated */

  /* if the song selection is displaying something else, do not */
  /* update the peers */
  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselPopulateData (uisongsel->peers [i]);
  }

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
    /* sets the dance drop-down for the peer */
    uisongselDanceSelectCallback (uisongsel->peers [i], danceIdx);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
  return UICB_CONT;
}

#if 0
void
uisongselSaveSelections (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  ssint = uisongsel->ssInternalData;
  if (ssint->selectedBackup != NULL) {
    return;
  }

//  ssint->selectedBackup = ssint->selectedList;
//  ssint->selectedList = nlistAlloc ("selected-list-save", LIST_ORDERED, NULL);

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
#endif

#if 0
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
#endif

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
  nlistidx_t      vliteridx;
  dbidx_t         dbidx;
  int32_t         rowidx;

  ssint = uisongsel->ssInternalData;
  tlist = nlistAlloc ("selected-list-dbidx", LIST_UNORDERED, NULL);
  nlistSetSize (tlist, uivlSelectionCount (ssint->uivl));
  uivlStartSelectionIterator (ssint->uivl, &vliteridx);
  while ((rowidx = uivlIterateSelection (ssint->uivl, &vliteridx)) >= 0) {
    dbidx = uivlGetRowColumnNum (ssint->uivl, rowidx, SONGSEL_COL_DBIDX);
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

void
uisongselSetSelection (uisongsel_t *uisongsel, int32_t idx)
{
  ss_internal_t   *ssint;

  ssint = uisongsel->ssInternalData;
  uivlSetSelection (ssint->uivl, idx);
}

/* internal routines */

static void
uisongselMoveSelection (void *udata, int direction)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;
  int             count;

  ssint = uisongsel->ssInternalData;

  count = uivlSelectionCount (ssint->uivl);
  if (count == 0) {
    return;
  }

  if (count > 1) {
    /* need to be able to move forwards and backwards within the select-list */
    /* do not change the virtual-list selection */

    if (direction == UISONGSEL_FIRST) {
      uivlStartSelectionIterator (ssint->uivl, &ssint->vlSelectIter);
      ssint->selectListKey = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);
    }
    if (direction == UISONGSEL_NEXT) {
      nlistidx_t  pkey;
      nlistidx_t  piter;

      pkey = ssint->selectListKey;
      piter = ssint->vlSelectIter;

      ssint->selectListKey = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);
      if (ssint->selectListKey < 0) {
        /* remain on the current selection, keep the iterator intact */
        ssint->selectListKey = pkey;
        ssint->vlSelectIter = piter;
      }
    }
    if (direction == UISONGSEL_PREVIOUS) {
      ssint->selectListKey = uivlIterateSelectionPrevious (ssint->uivl, &ssint->vlSelectIter);
      if (ssint->selectListKey < 0) {
        /* reset to the beginning */
        uivlStartSelectionIterator (ssint->uivl, &ssint->vlSelectIter);
        ssint->selectListKey = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);
      }
    }

    if (uisongsel->newselcb != NULL) {
      dbidx_t   dbidx;

      dbidx = uivlGetRowColumnNum (ssint->uivl, ssint->selectListKey, SONGSEL_COL_DBIDX);
      if (dbidx >= 0) {
        callbackHandlerI (uisongsel->newselcb, dbidx);
      }
    }
  }

  /* if there is only a single selection, move the virtual-list selection */
  if (count == 1) {
    if (direction == UISONGSEL_FIRST) {
      uivlSetSelection (ssint->uivl, 0);
    }
    if (direction == UISONGSEL_NEXT) {
      uivlMoveSelection (ssint->uivl, VL_DIR_NEXT);
    }
    if (direction == UISONGSEL_PREVIOUS) {
      uivlMoveSelection (ssint->uivl, VL_DIR_PREV);
    }
  }
}


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
  nlistidx_t      vliteridx;
  int32_t         rowidx;
  dbidx_t         dbidx;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;

  uivlStartSelectionIterator (ssint->uivl, &vliteridx);
  while ((rowidx = uivlIterateSelection (ssint->uivl, &vliteridx)) >= 0) {
    dbidx = uivlGetRowColumnNum (ssint->uivl, rowidx, SONGSEL_COL_DBIDX);
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
  uisongsel->numrows = songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
}

static void
uisongselDoubleClickCallback (void *udata, uivirtlist_t *vl,
    int32_t rownum, int colidx)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

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


static void
uisongselRowClickCallback (void *udata, uivirtlist_t *vl,
    int32_t rownum, int colidx)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

  if (ssint->favcolumn < 0 || colidx != ssint->favcolumn) {
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");
  dbidx = uivlGetRowColumnNum (ssint->uivl, rownum, SONGSEL_COL_DBIDX);
  if (dbidx < 0) {
    return;
  }
  song = dbGetByIdx (uisongsel->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uisongsel->songsavecb != NULL) {
      callbackHandlerI (uisongsel->songsavecb, dbidx);
    }
  }
  logProcEnd ("");
  return;
}

static void
uisongselRightClickCallback (void *udata, uivirtlist_t *vl,
    int32_t rownum, int colidx)
{
  uisongsel_t   * uisongsel = udata;
  ss_internal_t * ssint;
  nlistidx_t    vlseliter;
  int32_t       rowidx;
  dbidx_t       dbidx;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

  uivlStartSelectionIterator (ssint->uivl, &vlseliter);
  while ((rowidx = uivlIterateSelection (ssint->uivl, &vlseliter)) >= 0) {
    song_t      *song;
    nlistidx_t  genreidx;
    bool        clflag;

    dbidx = uivlGetRowColumnNum (ssint->uivl, rowidx, SONGSEL_COL_DBIDX);

    song = dbGetByIdx (uisongsel->musicdb, dbidx);
    genreidx = songGetNum (song, TAG_GENRE);
    clflag = genreGetClassicalFlag (ssint->genres, genreidx);
    if (clflag) {
      char        work [200];
      nlistidx_t  end;

      songGetClassicalWork (song, work, sizeof (work));
      if (*work) {
        char    twork [200];

        end = uisongsel->numrows;
        for (nlistidx_t i = rowidx + 1; i < end; ++i) {
          dbidx = songfilterGetByIdx (uisongsel->songfilter, i);
          song = dbGetByIdx (uisongsel->musicdb, dbidx);
          genreidx = songGetNum (song, TAG_GENRE);
          clflag = genreGetClassicalFlag (ssint->genres, genreidx);
          if (! clflag) {
            break;
          }

          songGetClassicalWork (song, twork, sizeof (twork));
          if (*twork && strcmp (work, twork) == 0) {
            uivlAppendSelection (ssint->uivl, i);
          } else {
            break;
          }
        }  /* check each following song for a matching classical work */
      } /* if the classical song has a 'work' */
    } /* if the song is classical */
  } /* for each selection in the virtual list */

  uisongselProcessSelectChg (uisongsel, ssint->uivl, rownum, colidx);

  logProcEnd ("");
  return;
}

static bool
uisongselKeyEvent (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;
  uiwcont_t       *keyh;

  if (uisongsel == NULL) {
    return UICB_CONT;
  }

  ssint = uisongsel->ssInternalData;
  keyh = uivlGetEventHandler (ssint->uivl);

  if (uiEventIsKeyPressEvent (keyh) &&
      uiEventIsAudioPlayKey (keyh)) {
    uisongselPlayCallback (uisongsel);
  }

  return UICB_CONT;
}

static void
uisongselProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;
  dbidx_t           dbidx = -1;

  ssint = uisongsel->ssInternalData;

  uisongsel->lastdbidx = dbidx;

  /* process the peers after the selections have been made */
  if (! uisongsel->ispeercall) {
    for (int i = 0; i < uisongsel->peercount; ++i) {
      if (uisongsel->peers [i] == NULL) {
        continue;
      }
      uisongselSetPeerFlag (uisongsel->peers [i], true);
      uisongselCopySelectList (uisongsel, uisongsel->peers [i]);
      uisongselSetPeerFlag (uisongsel->peers [i], false);
    }
  }

  if (uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    /* the song editor points to the first selected */
    dbidx = uivlGetRowColumnNum (ssint->uivl, ssint->selectListKey, SONGSEL_COL_DBIDX);
    if (dbidx >= 0) {
      callbackHandlerI (uisongsel->newselcb, dbidx);
    }
  }

  return;
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

static void
uisongselCopySelectList (uisongsel_t *uisongsel, uisongsel_t *peer)
{
  ss_internal_t   *ssint;
  uivirtlist_t    *vl_a;
  uivirtlist_t    *vl_b;

  ssint = uisongsel->ssInternalData;
  vl_a = ssint->uivl;
  ssint = peer->ssInternalData;
  vl_b = ssint->uivl;
  ssint = NULL;
  uivlCopySelectList (vl_a, vl_b);
}
