/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "grouping.h"
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
#include "uiutils.h"
#include "uivirtlist.h"
#include "uivlutil.h"

enum {
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
  bool                inapply;
  bool                inchange;
} ss_internal_t;

static bool uisongselQueueCallback (void *udata);
static void uisongselQueueHandler (uisongsel_t *uisongsel, int mqidx, int action);

static void uisongselDoubleClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uisongselRowClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void uisongselRightClickCallback (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

static bool uisongselKeyEvent (void *udata);
static void uisongselProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

static void uisongselMoveSelection (void *udata, int where);

static bool uisongselUIDanceSelectCallback (void *udata, int32_t idx, int32_t count);
static bool uisongselSongEditCallback (void *udata);
static void uisongselFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uisongselFillMark (uisongsel_t *uisongsel, ss_internal_t *ssint, dbidx_t dbidx, int32_t rownum);
static bool uisongselStartSFDialog (void *udata);

void
uisongselUIInit (uisongsel_t *uisongsel)
{
  ss_internal_t  *ssint;

  logProcBegin ();
  ssint = mdmalloc (sizeof (ss_internal_t));
  ssint->uivl = NULL;
  ssint->uisongsel = uisongsel;
  ssint->sscolorlist = slistAlloc ("ss-colors", LIST_ORDERED, NULL);
  ssint->inapply = false;
  ssint->inchange = false;
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

  uiutilsAddFavoriteClasses ();

  logProcEnd ("");
}

void
uisongselUIFree (uisongsel_t *uisongsel)
{
  ss_internal_t    *ssint;

  logProcBegin ();
  if (uisongsel == NULL || uisongsel->ssInternalData == NULL) {
    logProcEnd ("bad-ss-ssint");
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
  logProcEnd ("");
}

uiwcont_t *
uisongselBuildUI (uisongsel_t *uisongsel, uiwcont_t *parentwin)
{
  ss_internal_t     *ssint;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *hbox;
  slist_t           *sellist;
  char              tbuff [200];
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

  if (uisongsel->dispselType == DISP_SEL_SBS_SONGSEL) {
    uiwcont_t   *thbox;

    /* need a filler box to match the musicq */
    thbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (thbox);
    uiBoxPackStart (ssint->wcont [SONGSEL_W_MAIN_VBOX], thbox);

    uiwidgetp = uiCreateLabel (" ");
    uiBoxPackStart (thbox, uiwidgetp);
    uiWidgetSetMarginBottom (uiwidgetp, 5);
    uiwcontFree (uiwidgetp);
    uiwcontFree (thbox);
  }

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (ssint->wcont [SONGSEL_W_MAIN_VBOX], hbox);

  /* The side-by-side song selection does not need a select button, */
  /* as it has the left-arrow button.  Saves real estate. */
  if (uisongsel->dispselType == DISP_SEL_SONGSEL) {
    ssint->callbacks [SONGSEL_CB_SELECT] = callbackInit (
        uisongselSelectCallback, uisongsel, "songsel: select");
    uiwidgetp = uiCreateButton ("ss-select",
        /* CONTEXT: song-selection: select a song to be added to the song list */
        ssint->callbacks [SONGSEL_CB_SELECT], _("Select"), NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_SELECT] = uiwidgetp;
  }

  if (uisongsel->dispselType == DISP_SEL_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_SBS_SONGSEL ||
      uisongsel->dispselType == DISP_SEL_MM) {
    ssint->callbacks [SONGSEL_CB_EDIT_LOCAL] = callbackInit (
        uisongselSongEditCallback, uisongsel, "songsel: edit");
    uiwidgetp = uiCreateButton ("ss-edit",
        ssint->callbacks [SONGSEL_CB_EDIT_LOCAL],
        /* CONTEXT: song-selection: edit the selected song */
        _("Edit"), "button_edit");
    uiBoxPackStart (hbox, uiwidgetp);
    ssint->wcont [SONGSEL_W_BUTTON_EDIT] = uiwidgetp;
  }

  if (uisongsel->dispselType == DISP_SEL_REQUEST) {
    /* CONTEXT: (verb) song-selection: queue a song to be played */
    stpecpy (tbuff, tbuff + sizeof (tbuff), _("Queue"));
    ssint->callbacks [SONGSEL_CB_QUEUE] = callbackInit (
        uisongselQueueCallback, uisongsel, "songsel: queue");
    uiwidgetp = uiCreateButton ("ss-queue",
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
    stpecpy (tbuff, tbuff + sizeof (tbuff), _("Play"));
    ssint->callbacks [SONGSEL_CB_PLAY] = callbackInit (
        uisongselPlayCallback, uisongsel, "songsel: play");
    uiwidgetp = uiCreateButton ("ss-play",
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
      uisongselStartSFDialog, uisongsel, "songsel: filters");
  uiwidgetp = uiCreateButton ("ss-filter",
      ssint->callbacks [SONGSEL_CB_FILTER],
      /* CONTEXT: song-selection: tooltip: a button that starts the filters (narrowing down song selections) dialog */
      _("Filter Songs"), "button_filter");
  uiBoxPackEnd (hbox, uiwidgetp);
  ssint->wcont [SONGSEL_W_BUTTON_FILTER] = uiwidgetp;

  uiwcontFree (hbox);

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

  uivl = uivlCreate (uisongsel->tag, NULL, hbox, 7, 300, VL_ENABLE_KEYS);
  ssint->uivl = uivl;
  uivlSetUseListingFont (uivl);
  uivlSetAllowMultiple (uivl);
  uivlSetAllowDoubleClick (uivl);

  ssint->colcount = slistGetCount (sellist) + SONGSEL_COL_MAX;
  uivlSetNumColumns (uivl, ssint->colcount);

  uivlMakeColumn (uivl, "mark", SONGSEL_COL_MARK, VL_TYPE_LABEL);
  /* see comments in fill-mark */
  uivlSetColumnClass (uivl, SONGSEL_COL_MARK, LIST_NO_DISP);
  uivlSetColumnGrow (uivl, SONGSEL_COL_MARK, VL_COL_WIDTH_FIXED);

  uivlAddDisplayColumns (uivl, sellist, SONGSEL_COL_MAX);

  uivlSetRowFillCallback (uivl, uisongselFillRow, ssint);
  uivlSetNumRows (uivl, uisongsel->numrows);

  uivlDisplay (uivl);

  uivlSetDoubleClickCallback (ssint->uivl, uisongselDoubleClickCallback, uisongsel);
  uivlSetRowClickCallback (ssint->uivl, uisongselRowClickCallback, uisongsel);
  uivlSetRightClickCallback (ssint->uivl, uisongselRightClickCallback, uisongsel);
  uivlSetSelectChgCallback (ssint->uivl, uisongselProcessSelectChg, uisongsel);

  uisongselApplySongFilter (uisongsel);
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
  /* set-num-rows calls uivlpopulate */
  uivlSetNumRows (ssint->uivl, uisongsel->numrows);

  logProcEnd ("");
}

bool
uisongselSelectCallback (void *udata)
{
  uisongsel_t       *uisongsel = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel select");
  /* only the song selection and side-by-side song selection have */
  /* a select button to queue to the song list */
  uisongselQueueHandler (uisongsel, UISONGSEL_MQ_NOTSET, UISONGSEL_QUEUE);
  /* don't clear the selected list or the displayed selections */
  /* it's confusing for the user */
  logProcEnd ("");
  return UICB_CONT;
}

bool
uisongselNextSelection (void *udata)
{
  logProcBegin ();
  uisongselMoveSelection (udata, UISONGSEL_NEXT);
  logProcEnd ("");
  return UICB_CONT;
}

bool
uisongselPreviousSelection (void *udata)
{
  logProcBegin ();
  uisongselMoveSelection (udata, UISONGSEL_PREVIOUS);
  logProcEnd ("");
  return UICB_CONT;
}

bool
uisongselFirstSelection (void *udata)
{
  logProcBegin ();
  uisongselMoveSelection (udata, UISONGSEL_FIRST);
  logProcEnd ("");
  return UICB_CONT;
}

nlistidx_t
uisongselGetSelectLocation (uisongsel_t *uisongsel)
{
  ss_internal_t   *ssint;
  int             count;
  nlistidx_t      nidx = -1;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;
  count = uivlSelectionCount (ssint->uivl);
  if (count != 1) {
    logProcEnd ("no-sel");
    return -1;
  }

  uivlStartSelectionIterator (ssint->uivl, &ssint->vlSelectIter);
  nidx = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);
  logProcEnd ("");
  return nidx;
}

bool
uisongselApplySongFilter (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;
  ssint->inapply = true;

  /* the callback must be set every time, as the song filter is re-used */
  /* by multiple song-selection ui's */
  uisfSetApplyCallback (uisongsel->uisongfilter, uisongsel->sfapplycb);

  uidanceSetKey (uisongsel->uidance,
      songfilterGetNum (uisongsel->songfilter, SONG_FILTER_DANCE_IDX));

  uisongsel->numrows = songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);

  uisongselPopulateData (uisongsel);

  ssint->inapply = false;

  if (uisongsel->numrows > 0 && uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    uivlSetSelection (ssint->uivl, 0);
    dbidx = songfilterGetByIdx (uisongsel->songfilter, 0);
    if (dbidx >= 0) {
      callbackHandlerI (uisongsel->newselcb, dbidx);
    }
  }

  logProcEnd ("");
  return UICB_CONT;
}

/* handles the dance drop-down */
/* when a dance is selected, the song filter must be updated */
/* applies the song filter */
void
uisongselDanceSelectHandler (uisongsel_t *uisongsel, ilistidx_t danceIdx)
{
  logProcBegin ();
  uisfSetDanceIdx (uisongsel->uisongfilter, danceIdx);
  uidanceSetKey (uisongsel->uidance, danceIdx);
  uisongselApplySongFilter (uisongsel);
  logProcEnd ("");
}

/* callback for the song filter when the dance selection is changed */
/* does not apply the filter */
bool
uisongselDanceSelectCallback (void *udata, int32_t danceIdx)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin ();

  uidanceSetKey (uisongsel->uidance, danceIdx);

  logProcEnd ("");
  return UICB_CONT;
}

bool
uisongselPlayCallback (void *udata)
{
  musicqidx_t mqidx;
  uisongsel_t *uisongsel = udata;

  logProcBegin ();
  /* only song selection, side-by-side song sel and music manager */
  /* have a play button */
  /* use the hidden manage playback music queue */
  mqidx = MUSICQ_MNG_PB;
  /* the manageui callback clears the queue and plays */
  /* note that only the first of multiple selections */
  /* will use 'play', and the rest will be queued */
  uisongselQueueHandler (uisongsel, mqidx, UISONGSEL_PLAY);
  logProcEnd ("");
  return UICB_CONT;
}

void
uisongselSetPlayButtonState (uisongsel_t *uisongsel, int active)
{
  ss_internal_t  *ssint;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetSetState (ssint->wcont [SONGSEL_W_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiWidgetSetState (ssint->wcont [SONGSEL_W_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
  logProcEnd ("");
}

nlist_t *
uisongselGetSelectedList (uisongsel_t *uisongsel)
{
  ss_internal_t   * ssint;
  nlist_t         *tlist;
  nlistidx_t      vliteridx;
  dbidx_t         dbidx;
  int32_t         rowidx;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;
  tlist = nlistAlloc ("selected-list-dbidx", LIST_UNORDERED, NULL);
  nlistSetSize (tlist, uivlSelectionCount (ssint->uivl));
  uivlStartSelectionIterator (ssint->uivl, &vliteridx);
  while ((rowidx = uivlIterateSelection (ssint->uivl, &vliteridx)) >= 0) {
    dbidx = songfilterGetByIdx (uisongsel->songfilter, rowidx);
    nlistSetNum (tlist, dbidx, 0);
  }
  nlistSort (tlist);
  logProcEnd ("");
  return tlist;
}

void
uisongselSetRequestLabel (uisongsel_t *uisongsel, const char *txt)
{
  ss_internal_t  *ssint;

  logProcBegin ();
  if (uisongsel == NULL) {
    logProcEnd ("bad-ss");
    return;
  }

  ssint = uisongsel->ssInternalData;
  uiLabelSetText (ssint->wcont [SONGSEL_W_REQ_QUEUE], txt);
  logProcEnd ("");
}

void
uisongselSetSelection (uisongsel_t *uisongsel, int32_t rowidx)
{
  ss_internal_t   *ssint;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;
  uivlSetSelection (ssint->uivl, rowidx);

  if (uivlSelectionCount (ssint->uivl) == 1) {
    uisongsel->lastdbidx = songfilterGetByIdx (uisongsel->songfilter, rowidx);
  }

  logProcEnd ("");
}

void
uisongselCopySelectList (uisongsel_t *uisongsel, uisongsel_t *peer)
{
  ss_internal_t   *ssint;
  uivirtlist_t    *vl_a;
  uivirtlist_t    *vl_b;

  if (uisongsel == NULL || peer == NULL) {
    return;
  }

  logProcBegin ();
  ssint = uisongsel->ssInternalData;
  vl_a = ssint->uivl;

  ssint = peer->ssInternalData;
  vl_b = ssint->uivl;

  if (vl_b == NULL) {
    /* the peer has not yet been initialized, even though it may exist */
    logProcEnd ("bad-peer");
    return;
  }

  ssint = NULL;
  uivlCopySelectList (vl_a, vl_b);
  logProcEnd ("");
}

/* internal routines */

static void
uisongselMoveSelection (void *udata, int direction)
{
  uisongsel_t     *uisongsel = udata;
  ss_internal_t   *ssint;
  int             count;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;

  count = uivlSelectionCount (ssint->uivl);
  if (count == 0) {
    logProcEnd ("no-sel");
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

      /* the db-idx must be retrieved from the song filter, not the */
      /* virtual list, as the selection may not be currently displayed */
      dbidx = songfilterGetByIdx (uisongsel->songfilter, ssint->selectListKey);
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
  logProcEnd ("");
}


static bool
uisongselQueueCallback (void *udata)
{
  uisongsel_t *uisongsel = udata;

  logProcBegin ();
  /* queues from the request listing to a music queue in the player */
  uisongselQueueHandler (uisongsel, UISONGSEL_MQ_NOTSET, UISONGSEL_QUEUE);
  logProcEnd ("");
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
    dbidx = songfilterGetByIdx (uisongsel->songfilter, rowidx);
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

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: dance select");
  uisongselDanceSelectHandler (uisongsel, idx);
  logProcEnd ("");
  return UICB_CONT;
}

static void
uisongselDoubleClickCallback (void *udata, uivirtlist_t *vl,
    int32_t rownum, int colidx)
{
  uisongsel_t   * uisongsel = udata;

  logProcBegin ();

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
  logProcEnd ("");
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
  if (uivlSelectionCount (ssint->uivl) == 1) {
    uisongsel->lastdbidx = songfilterGetByIdx (uisongsel->songfilter, rownum);
  }

  if (ssint->favcolumn < 0 || colidx != ssint->favcolumn) {
    logProcEnd ("not-fav-col");
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: songsel: change favorite");
  dbidx = songfilterGetByIdx (uisongsel->songfilter, rownum);
  if (dbidx < 0) {
    logProcEnd ("bad-dbidx");
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
  int32_t       rowidx;

  logProcBegin ();

  ssint = uisongsel->ssInternalData;

  rowidx = uivlGetCurrSelection (ssint->uivl);
  do {
    nlistidx_t    end;
    int           selcount;
    int           maxcount;
    int32_t       tval;
    dbidx_t       seldbidx;
    dbidx_t       tdbidx;

    seldbidx = songfilterGetByIdx (uisongsel->songfilter, rowidx);

    /* returns the number of selections in the group or zero */
    maxcount = groupingCheck (uisongsel->grouping, seldbidx, seldbidx);
    if (maxcount == 0) {
      return;
    }
    /* the first is already selected */
    selcount = 1;

    end = uisongsel->numrows;

    /* often, the user selects the first item in the group */
    /* so start at rowidx + 1 */
    for (nlistidx_t i = rowidx + 1; i < end && selcount < maxcount; ++i) {
      /* check if this dbidx is in the group */
      tdbidx = songfilterGetByIdx (uisongsel->songfilter, i);
      tval = groupingCheck (uisongsel->grouping, seldbidx, tdbidx);
      if (tval == 0) {
        continue;
      }
      uivlAppendSelection (ssint->uivl, i);
      ++selcount;
    }
    /* check the rest if all the selections have not been found */
    /* the user may have selected a song in the middle of the group */
    /* or the sort order may not match the grouping */
    for (nlistidx_t i = rowidx - 1; i >= 0 && selcount < maxcount; --i) {
      tdbidx = songfilterGetByIdx (uisongsel->songfilter, i);
      /* check if this dbidx is in the group */
      tval = groupingCheck (uisongsel->grouping, seldbidx, tdbidx);
      if (tval <= 0) {
        continue;
      }
      uivlAppendSelection (ssint->uivl, i);
      ++selcount;
    }
  } while (0);

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

  logProcBegin ();
  if (uisongsel == NULL) {
    logProcEnd ("bad-ss");
    return UICB_CONT;
  }

  ssint = uisongsel->ssInternalData;
  keyh = uivlGetEventHandler (ssint->uivl);

  if (uiEventIsKeyPressEvent (keyh) &&
      uiEventIsAudioPlayKey (keyh)) {
    uisongselPlayCallback (uisongsel);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static void
uisongselProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uisongsel_t       *uisongsel = udata;
  ss_internal_t     *ssint;

  logProcBegin ();
  ssint = uisongsel->ssInternalData;

  if (ssint->inapply) {
    /* the apply-song-filter function will set the selection */
    logProcEnd ("in-apply");
    return;
  }

  if (uivlSelectionCount (ssint->uivl) == 1) {
    uisongsel->lastdbidx = songfilterGetByIdx (uisongsel->songfilter, rownum);
  }

  /* the selection has changed, reset the iterator and set the select key */
  /* always set to the first in the list */
  uivlStartSelectionIterator (ssint->uivl, &ssint->vlSelectIter);
  ssint->selectListKey = uivlIterateSelection (ssint->uivl, &ssint->vlSelectIter);

  if (uisongsel->newselcb != NULL) {
    dbidx_t   dbidx;

    dbidx = songfilterGetByIdx (uisongsel->songfilter, ssint->selectListKey);
    if (dbidx >= 0) {
      callbackHandlerI (uisongsel->newselcb, dbidx);
    }
  }

  logProcEnd ("");
  return;
}

/* have to handle the case where the user switches tabs back to the */
/* song selection tab */
static bool
uisongselSongEditCallback (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  dbidx_t         dbidx;
  int             rc = UICB_CONT;


  logProcBegin ();
  if (uisongsel->newselcb != NULL) {
    dbidx = uisongsel->lastdbidx;
    if (dbidx < 0) {
      logProcEnd ("bad-dbidx");
      return UICB_CONT;
    }
    callbackHandlerI (uisongsel->newselcb, dbidx);
  }
  if (uisongsel->editcb != NULL) {
    rc = callbackHandler (uisongsel->editcb);
  }
  logProcEnd ("");
  return rc;
}

static void
uisongselFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  ss_internal_t       *ssint = udata;
  uisongsel_t         *uisongsel = ssint->uisongsel;
  song_t              *song;
  nlist_t             *tdlist;
  slistidx_t          seliteridx;
  dbidx_t             dbidx;
  int                 dbflags;

  logProcBegin ();

  dbidx = songfilterGetByIdx (uisongsel->songfilter, rownum);
  if (dbidx < 0) {
    return;
  }
  song = dbGetByIdx (uisongsel->musicdb, dbidx);
  if (song == NULL) {
    return;
  }
  dbflags = songGetNum (song, TAG_DB_FLAGS);

  ssint->inchange = true;

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

    if (tagidx != TAG_FAVORITE && dbflags == MUSICDB_REMOVE_MARK) {
      uivlSetRowColumnClass (ssint->uivl, rownum, colidx, ERROR_CLASS);
    }
  }
  nlistFree (tdlist);

  uisongselFillMark (uisongsel, ssint, dbidx, rownum);

  ssint->inchange = false;
  logProcEnd ("");
}


static void
uisongselFillMark (uisongsel_t *uisongsel, ss_internal_t *ssint,
    dbidx_t dbidx, int32_t rownum)
{
  const char  *markstr = NULL;
  const char  *sscolor = NULL;
  const char  *classnm = NULL;

  /* very strange windows/gtk bug */
  /* symptom: scrollbar stops updating */
  /* only seems to be an issue with the mark column, not other columns */
  /* if the markstr is not static, the scrollbar stops */
  /* working after one of the marks is turned off */
  /* as a work-around, always set the markstr to the same value, */
  /* and set the column class to bdj-nodisp */
  /* i don't understand why this is happening */
  markstr = ssint->marktext;

  if (uisongsel->dispselType != DISP_SEL_MM &&
      uisongsel->songlistdbidxlist != NULL) {
    /* check and see if the song is in the song list or history, etc. */
    if (nlistGetNum (uisongsel->songlistdbidxlist, dbidx) >= 0) {
      markstr = ssint->marktext;
      classnm = MARK_CLASS;
    }
  }

  if (uisongsel->dispselType == DISP_SEL_MM) {
    sscolor = samesongGetColorByDBIdx (uisongsel->samesong, dbidx);
    if (sscolor != NULL) {
      /* skip the leading # for the class name */
      classnm = sscolor + 1;
      if (slistGetNum (ssint->sscolorlist, sscolor) < 0) {
        slistSetNum (ssint->sscolorlist, sscolor, 0);
        uiLabelAddClass (classnm, sscolor);
      }
      markstr = ssint->marktext;
    }
  }

  uivlSetRowColumnStr (ssint->uivl, rownum, SONGSEL_COL_MARK, markstr);
  if (classnm != NULL) {
    uivlSetRowColumnClass (ssint->uivl, rownum, SONGSEL_COL_MARK, classnm);
  }
}

static bool
uisongselStartSFDialog (void *udata)
{
  uisongsel_t     *uisongsel = udata;
  uisongfilter_t  *uisf = uisongsel->uisongfilter;
  bool            rc;

  uisfSetApplyCallback (uisf, uisongsel->sfapplycb);
  rc = uisfDialog (uisf);
  return rc;
}
