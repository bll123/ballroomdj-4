/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "callback.h"
#include "dispsel.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "ui.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uimusicq.h"
#include "uisong.h"
#include "uitreedisp.h"

enum {
  UIMUSICQ_COL_ELLIPSIZE,
  UIMUSICQ_COL_FONT,
  UIMUSICQ_COL_UNIQUE_IDX,
  UIMUSICQ_COL_DBIDX,
  UIMUSICQ_COL_DISP_IDX_COLOR,
  UIMUSICQ_COL_DISP_IDX_COLOR_SET,
  UIMUSICQ_COL_DISP_IDX,
  UIMUSICQ_COL_PAUSEIND,
  UIMUSICQ_COL_MAX,
};

enum {
  MQINT_CB_MOVE_TOP,
  MQINT_CB_MOVE_UP,
  MQINT_CB_MOVE_DOWN,
  MQINT_CB_TOGGLE_PAUSE,
  MQINT_CB_AUDIO_REMOVE,
  MQINT_CB_REQ_EXTERNAL,
  MQINT_CB_CLEAR_QUEUE,
  MQINT_CB_EDIT_LOCAL,
  MQINT_CB_PLAY,
  MQINT_CB_HIST_QUEUE,
  MQINT_CB_KEYB,
  MQINT_CB_CHK_FAV_CHG,
  MQINT_CB_SEL_CHG,
  MQINT_CB_INT_ITERATE,
  MQINT_CB_MAX,
};

enum {
  UIMUSICQ_W_BUTTON_CLEAR_QUEUE,
  UIMUSICQ_W_BUTTON_EDIT,
  UIMUSICQ_W_BUTTON_MOVE_DOWN,
  UIMUSICQ_W_BUTTON_MOVE_TOP,
  UIMUSICQ_W_BUTTON_MOVE_UP,
  UIMUSICQ_W_BUTTON_PLAY,
  UIMUSICQ_W_BUTTON_QUEUE,
  UIMUSICQ_W_BUTTON_REMOVE,
  UIMUSICQ_W_BUTTON_TOGGLE_PAUSE,
  UIMUSICQ_W_REQ_QUEUE,
  UIMUSICQ_W_KEY_HNDLR,
  UIMUSICQ_W_MAX,
};

typedef struct mq_internal {
  uidance_t         *uidance;
  uidance_t         *uidance5;
  uitree_t          *musicqTree;
  callback_t        *callbacks [MQINT_CB_MAX];
  uiwcont_t         *wcont [UIMUSICQ_W_MAX];
  int               *typelist;
  int               colcount;         // for the display type callback
  int               rowcount;         // current size of tree view storage
} mq_internal_t;

static bool   uimusicqQueueDanceCallback (void *udata, long idx, int count);
static bool   uimusicqQueuePlaylistCallback (void *udata, long idx);
static void   uimusicqProcessMusicQueueDataNewCallback (int type, void *udata);
static void   uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate);
static void   uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, song_t *song, int ci);
static void   uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata);
static bool   uimusicqIterateCallback (void *udata);
static bool   uimusicqPlayCallback (void *udata);
static bool   uimusicqQueueCallback (void *udata);
static bool   uimusicqSelectionChgCallback (void *udata);
static void   uimusicqSelectionChgProcess (uimusicq_t *uimusicq);
static void   uimusicqSelectionPreviousProcess (uimusicq_t *uimusicq, long loc);
static void   uimusicqSetDefaultSelection (uimusicq_t *uimusicq);
static void   uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx);
static bool   uimusicqSongEditCallback (void *udata);
static bool   uimusicqMoveTopCallback (void *udata);
static bool   uimusicqMoveUpCallback (void *udata);
static bool   uimusicqMoveDownCallback (void *udata);
static bool   uimusicqTogglePauseCallback (void *udata);
static bool   uimusicqRemoveCallback (void *udata);
static bool   uimusicqCheckFavChgCallback (void *udata, long col);
static bool   uimusicqKeyEvent (void *udata);
static void   uimusicqMarkPreviousSelection (uimusicq_t *uimusicq, bool disp);
static void   uimusicqCopySelections (uimusicq_t *uimusicq, uimusicq_t *peer, int mqidx);


void
uimusicqUIInit (uimusicq_t *uimusicq)
{
  mq_internal_t   *mqint;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mqint = mdmalloc (sizeof (mq_internal_t));
    uimusicq->ui [i].mqInternalData = mqint;
    mqint->uidance = NULL;
    mqint->uidance5 = NULL;
    mqint->musicqTree = NULL;
    for (int j = 0; j < MQINT_CB_MAX; ++j) {
      mqint->callbacks [j] = NULL;
    }
    for (int j = 0; j < UIMUSICQ_W_MAX; ++j) {
      mqint->wcont [j] = NULL;
    }
  }
}

void
uimusicqUIFree (uimusicq_t *uimusicq)
{
  mq_internal_t   *mqint;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mqint = uimusicq->ui [i].mqInternalData;
    if (mqint != NULL) {
      for (int j = 0; j < MQINT_CB_MAX; ++j) {
        callbackFree (mqint->callbacks [j]);
      }
      for (int j = 0; j < UIMUSICQ_W_MAX; ++j) {
        uiwcontFree (mqint->wcont [j]);
      }
      uidanceFree (mqint->uidance);
      uidanceFree (mqint->uidance5);
      uiTreeViewFree (mqint->musicqTree);
      mdfree (mqint);
    }
  }
}

uiwcont_t *
uimusicqBuildUI (uimusicq_t *uimusicq, uiwcont_t *parentwin, int ci,
    uiwcont_t *statusMsg, uientryval_t validateFunc)
{
  int               saveci;
  uiwcont_t         *hbox;
  uiwcont_t         *scwin;
  uiwcont_t         *uitreewidgetp;
  uiwcont_t         *uiwidgetp;
  slist_t           *sellist;
  mq_internal_t     *mqint;


  logProcBegin (LOG_PROC, "uimusicqBuildUI");

  saveci = uimusicq->musicqManageIdx;
  /* temporary */
  uimusicq->musicqManageIdx = ci;
  mqint = uimusicq->ui [ci].mqInternalData;
  uimusicq->statusMsg = statusMsg;

  uimusicq->ui [ci].hasui = true;

  uimusicq->parentwin = parentwin;

  uimusicq->ui [ci].mainbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (uimusicq->ui [ci].mainbox);
  uiWidgetExpandVert (uimusicq->ui [ci].mainbox);

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    hbox = uiCreateHorizBox ();
    uiWidgetSetMarginTop (hbox, 1);
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (uimusicq->ui [ci].mainbox, hbox);

    /* CONTEXT: music queue: label for song list name */
    uiwidgetp = uiCreateColonLabel (_("Song List"));
    uiWidgetSetMarginStart (uiwidgetp, 2);
    uiWidgetSetMarginEnd (uiwidgetp, 2);
    uiBoxPackStart (hbox, uiwidgetp);
    uiwcontFree (uiwidgetp);

    uiwidgetp = uiEntryInit (20, 100);
    uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
    if (uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
      uiWidgetExpandHoriz (uiwidgetp);
      uiWidgetAlignHorizFill (uiwidgetp);
      uiBoxPackStartExpand (hbox, uiwidgetp);
    }
    if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST) {
      uiBoxPackStart (hbox, uiwidgetp);
    }
    uimusicq->ui [ci].slname = uiwidgetp;
    if (validateFunc != NULL) {
      uiEntrySetValidate (uiwidgetp, validateFunc, statusMsg, UIENTRY_IMMEDIATE);
    }

    uiwcontFree (hbox);
  }

  hbox = uiCreateHorizBox ();
  uiWidgetSetMarginTop (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uimusicq->ui [ci].mainbox, hbox);

  /* dispseltype can be a music queue, history, */
  /* a song list or side-by-side song list */

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->callbacks [MQINT_CB_MOVE_TOP] = callbackInit (
        uimusicqMoveTopCallback, uimusicq, "musicq: move-to-top");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_MOVE_TOP],
        /* CONTEXT: music queue: button: move the selected song to the top of the queue */
        _("Move to Top"), "button_movetop");
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_TOP] = uiwidgetp;

    mqint->callbacks [MQINT_CB_MOVE_UP] = callbackInit (
        uimusicqMoveUpCallback, uimusicq, "musicq: move-up");
    uiwidgetp = uiCreateButton (mqint->callbacks [MQINT_CB_MOVE_UP],
        /* CONTEXT: music queue: button: move the selected song up in the queue */
        _("Move Up"), "button_up");
    uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_UP] = uiwidgetp;

    mqint->callbacks [MQINT_CB_MOVE_DOWN] = callbackInit (
        uimusicqMoveDownCallback, uimusicq, "musicq: move-down");
    uiwidgetp = uiCreateButton (mqint->callbacks [MQINT_CB_MOVE_DOWN],
        /* CONTEXT: music queue: button: move the selected song down in the queue */
        _("Move Down"), "button_down");
    uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_DOWN] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    mqint->callbacks [MQINT_CB_TOGGLE_PAUSE] = callbackInit (
        uimusicqTogglePauseCallback, uimusicq, "musicq: toggle-pause");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_TOGGLE_PAUSE],
        /* CONTEXT: music queue: button: set playback to pause after the selected song is played (toggle) */
        _("Toggle Pause"), "button_pause");
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_TOGGLE_PAUSE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->callbacks [MQINT_CB_AUDIO_REMOVE] = callbackInit (
        uimusicqRemoveCallback, uimusicq, "musicq: remove-from-queue");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_AUDIO_REMOVE],
        /* CONTEXT: music queue: button: remove the song from the queue */
        _("Remove"), "button_audioremove");
    uiWidgetSetMarginStart (uiwidgetp, 3);
    uiWidgetSetMarginEnd (uiwidgetp, 2);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_REMOVE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    mqint->callbacks [MQINT_CB_CLEAR_QUEUE] = callbackInit (
        uimusicqTruncateQueueCallback, uimusicq, "musicq: clear-queue");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_CLEAR_QUEUE],
        /* CONTEXT: music queue: button: clear the queue */
        _("Clear Queue"), NULL);
    uiWidgetSetMarginStart (uiwidgetp, 2);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_CLEAR_QUEUE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->callbacks [MQINT_CB_EDIT_LOCAL] = callbackInit (
        uimusicqSongEditCallback, uimusicq, "musicq: edit");
    uiwidgetp = uiCreateButton (mqint->callbacks [MQINT_CB_EDIT_LOCAL],
        /* CONTEXT: music queue: edit the selected song */
        _("Edit"), "button_edit");
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_EDIT] = uiwidgetp;

    mqint->callbacks [MQINT_CB_PLAY] = callbackInit (
        uimusicqPlayCallback, uimusicq, "musicq: play");
    uiwidgetp = uiCreateButton (mqint->callbacks [MQINT_CB_PLAY],
        /* CONTEXT: music queue: tooltip: play the selected song */
        _("Play"), "button_play");
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_PLAY] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_HISTORY) {
    mqint->callbacks [MQINT_CB_HIST_QUEUE] = callbackInit (
        uimusicqQueueCallback, uimusicq, "musicq: queue");
    uiwidgetp = uiCreateButton (mqint->callbacks [MQINT_CB_HIST_QUEUE],
        /* CONTEXT: (verb) history: re-queue the selected song: suggested: 'put song in queue' */
        _("Queue"), NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_QUEUE] = uiwidgetp;

    uiwidgetp = uiCreateLabel ("");
    uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_REQ_QUEUE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    if (uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST] == NULL) {
      uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST] = callbackInitLong (
          uimusicqQueuePlaylistCallback, uimusicq);
    }
    uiwidgetp = uiDropDownInit ();
    uimusicq->ui [ci].playlistsel = uiwidgetp;

    uiwidgetp = uiDropDownCreate (uimusicq->ui [ci].playlistsel, parentwin,
        /* CONTEXT: (verb) music queue: button: queue a playlist for playback: suggested 'put playlist in queue' */
        _("Queue Playlist"), uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST], uimusicq);
    uiBoxPackEnd (hbox, uiwidgetp);
    uimusicqCreatePlaylistList (uimusicq);

    if (bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, ci)) {
      if (uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE] == NULL) {
        uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE] = callbackInitLongInt (
            uimusicqQueueDanceCallback, uimusicq);
      }
      mqint->uidance5 = uidanceDropDownCreate (hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue 5 dances for playback: suggested '5 in queue' (this button gets context from the 'queue dance' button) */
          UIDANCE_NONE, _("Queue 5"), UIDANCE_PACK_END, 5);
      uidanceSetCallback (mqint->uidance5, uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE]);

      mqint->uidance = uidanceDropDownCreate (hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue a dance for playback: suggested: put dance in queue */
          UIDANCE_NONE, _("Queue Dance"), UIDANCE_PACK_END, 1);
      uidanceSetCallback (mqint->uidance, uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE]);
    }
  }

  /* musicq tree view */

  scwin = uiCreateScrolledWindow (400);
  uiWidgetExpandHoriz (scwin);
  uiBoxPackStartExpand (uimusicq->ui [ci].mainbox, scwin);

  mqint->musicqTree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetWidgetContainer (mqint->musicqTree);

  uiTreeViewEnableHeaders (mqint->musicqTree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetExpandHoriz (uitreewidgetp);
  uiWidgetExpandVert (uitreewidgetp);
  uiWindowPackInWindow (scwin, uitreewidgetp);
  uiwcontFree (scwin);

  mqint->callbacks [MQINT_CB_CHK_FAV_CHG] = callbackInitLong (
        uimusicqCheckFavChgCallback, uimusicq);
  uiTreeViewSetRowActivatedCallback (mqint->musicqTree,
        mqint->callbacks [MQINT_CB_CHK_FAV_CHG]);

  uiTreeViewAppendColumn (mqint->musicqTree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_RIGHT,
      TREE_COL_DISP_GROW, "",
      TREE_COL_TYPE_TEXT, UIMUSICQ_COL_DISP_IDX,
      TREE_COL_TYPE_FONT, UIMUSICQ_COL_FONT,
      TREE_COL_TYPE_FOREGROUND_SET, UIMUSICQ_COL_DISP_IDX_COLOR_SET,
      TREE_COL_TYPE_FOREGROUND, UIMUSICQ_COL_DISP_IDX_COLOR,
      TREE_COL_TYPE_END);

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiTreeViewAppendColumn (mqint->musicqTree, TREE_NO_COLUMN,
        TREE_WIDGET_IMAGE, TREE_ALIGN_CENTER,
        TREE_COL_DISP_GROW, "",
        TREE_COL_TYPE_IMAGE, UIMUSICQ_COL_PAUSEIND,
        TREE_COL_TYPE_END);
  }

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uitreedispAddDisplayColumns (mqint->musicqTree, sellist,
      UIMUSICQ_COL_MAX, UIMUSICQ_COL_FONT, UIMUSICQ_COL_ELLIPSIZE,
      TREE_NO_COLUMN, TREE_NO_COLUMN);

  uimusicq->musicqManageIdx = saveci;

  mqint->callbacks [MQINT_CB_SEL_CHG] = callbackInit (
        uimusicqSelectionChgCallback, uimusicq, NULL);
  uiTreeViewSetSelectChangedCallback (mqint->musicqTree,
        mqint->callbacks [MQINT_CB_SEL_CHG]);

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->wcont [UIMUSICQ_W_KEY_HNDLR] = uiKeyAlloc ();
    mqint->callbacks [MQINT_CB_KEYB] = callbackInit (
        uimusicqKeyEvent, uimusicq, NULL);
    uiKeySetKeyCallback (mqint->wcont [UIMUSICQ_W_KEY_HNDLR], uitreewidgetp,
        mqint->callbacks [MQINT_CB_KEYB]);
  }

  mqint->callbacks [MQINT_CB_INT_ITERATE] = callbackInit (
        uimusicqIterateCallback, uimusicq, NULL);

  uiwcontFree (hbox);

  /* initialize the musicq storage */

  mqint->typelist = mdmalloc (sizeof (int) * UIMUSICQ_COL_MAX);
  mqint->colcount = 0;
  mqint->rowcount = 0;
  mqint->typelist [UIMUSICQ_COL_ELLIPSIZE] = TREE_TYPE_ELLIPSIZE;
  mqint->typelist [UIMUSICQ_COL_FONT] = TREE_TYPE_STRING;
  mqint->typelist [UIMUSICQ_COL_UNIQUE_IDX] = TREE_TYPE_NUM;
  mqint->typelist [UIMUSICQ_COL_DBIDX] = TREE_TYPE_NUM;
  mqint->typelist [UIMUSICQ_COL_DISP_IDX_COLOR] = TREE_TYPE_STRING;
  mqint->typelist [UIMUSICQ_COL_DISP_IDX_COLOR_SET] = TREE_TYPE_BOOLEAN;
  mqint->typelist [UIMUSICQ_COL_DISP_IDX] = TREE_TYPE_NUM;
  mqint->typelist [UIMUSICQ_COL_PAUSEIND] = TREE_TYPE_IMAGE;
  mqint->colcount = UIMUSICQ_COL_MAX;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uimusicq->cbci = ci;
  uisongAddDisplayTypes (sellist,
      uimusicqProcessMusicQueueDataNewCallback, uimusicq);

  uiTreeViewCreateValueStoreFromList (mqint->musicqTree, mqint->colcount, mqint->typelist);
  mdfree (mqint->typelist);

  logProcEnd (LOG_PROC, "uimusicqBuildUI", "");
  return uimusicq->ui [ci].mainbox;
}

void
uimusicqDragDropSetURICallback (uimusicq_t *uimusicq, int ci, callback_t *cb)
{
  mq_internal_t     *mqint;

  if (ci < 0 || ci >= MUSICQ_DISP_MAX) {
    return;
  }

  mqint = uimusicq->ui [ci].mqInternalData;
  uiDragDropSetDestURICallback (uiTreeViewGetWidgetContainer (mqint->musicqTree), cb);
}

void
uimusicqUIMainLoop (uimusicq_t *uimusicq)
{
  mq_internal_t *mqint;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  uiButtonCheckRepeat (mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_DOWN]);
  uiButtonCheckRepeat (mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_UP]);
  return;
}


void
uimusicqSetSelectionFirst (uimusicq_t *uimusicq, int mqidx)
{
  uimusicqSetSelectLocation (uimusicq, mqidx, 0);
}

void
uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int mqidx, int which)
{
  int               valid = false;
  int               count = 0;
  mq_internal_t     *mqint;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");
  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "no-ui");
    return;
  }
  mqint = uimusicq->ui [mqidx].mqInternalData;

  count = uiTreeViewSelectGetCount (mqint->musicqTree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "count != 1");
    return;
  }

  switch (which) {
    case UIMUSICQ_SEL_CURR: {
      break;
    }
    case UIMUSICQ_SEL_PREV: {
      uiTreeViewSelectPrevious (mqint->musicqTree);
      break;
    }
    case UIMUSICQ_SEL_NEXT: {
      uiTreeViewSelectNext (mqint->musicqTree);
      break;
    }
    case UIMUSICQ_SEL_TOP: {
      uiTreeViewSelectFirst (mqint->musicqTree);
      break;
    }
  }

  if (valid) {
    if (uimusicq->ui [mqidx].count > 0) {
      uiTreeViewScrollToCell (mqint->musicqTree);
    }
  }
  logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "");
}

nlist_t *
uimusicqGetDBIdxList (uimusicq_t *uimusicq, musicqidx_t mqidx)
{
  mq_internal_t   *mqint;

  mqint = uimusicq->ui [mqidx].mqInternalData;

  uimusicq->cbcopy [UIMUSICQ_CBC_ITERATE] =
      uimusicq->callbacks [UIMUSICQ_CB_SAVE_LIST];
  nlistFree (uimusicq->savelist);
  uimusicq->savelist = nlistAlloc ("savelist", LIST_UNORDERED, NULL);
  uiTreeViewValueIteratorSet (mqint->musicqTree, 0);
  uiTreeViewForeach (mqint->musicqTree,
      mqint->callbacks [MQINT_CB_INT_ITERATE]);
  uiTreeViewValueIteratorClear (mqint->musicqTree);

  return uimusicq->savelist;
}

long
uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx)
{
  mq_internal_t *mqint;
  long          loc;

  mqint = uimusicq->ui [mqidx].mqInternalData;

  loc = QUEUE_LOC_LAST;
  if (mqint->musicqTree == NULL) {
    return loc;
  }

  uiTreeViewSelectCurrent (mqint->musicqTree);
  loc = uiTreeViewSelectGetIndex (mqint->musicqTree);

  return loc;
}

void
uimusicqSetSelectLocation (uimusicq_t *uimusicq, int mqidx, long loc)
{
  if (loc < 0) {
    return;
  }

  uimusicq->ui [mqidx].selectLocation = loc;
  uimusicqSetSelection (uimusicq, mqidx);
}

bool
uimusicqTruncateQueueCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  long          idx;


  logProcBegin (LOG_PROC, "uimusicqTruncateQueueCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: %s: clear queue", uimusicq->tag);

  ci = uimusicq->musicqManageIdx;

  if (uimusicq->cbcopy [UIMUSICQ_CBC_CLEAR_QUEUE] != NULL) {
    callbackHandler (uimusicq->cbcopy [UIMUSICQ_CBC_CLEAR_QUEUE]);
  }

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    /* if nothing is selected, clear everything */
    idx = 0;
    if (uimusicq->musicqManageIdx == uimusicq->musicqPlayIdx) {
      idx = 1;
    }
  } else {
    ++idx;
  }

  uimusicqTruncateQueue (uimusicq, ci, idx);
  logProcEnd (LOG_PROC, "uimusicqTruncateQueueCallback", "");
  return UICB_CONT;
}

void
uimusicqSetPlayButtonState (uimusicq_t *uimusicq, int active)
{
  mq_internal_t *mqint;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetSetState (mqint->wcont [UIMUSICQ_W_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiWidgetSetState (mqint->wcont [UIMUSICQ_W_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
}

void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate)
{
  int               ci;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueData");

  ci = musicqupdate->mqidx;
  if (ci < 0 || ci >= MUSICQ_MAX) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "bad-mq-idx");
    return;
  }

  if (! uimusicq->ui [ci].hasui) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "no-ui");
    return;
  }

  uimusicqProcessMusicQueueDisplay (uimusicq, musicqupdate);

  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "");
}

void
uimusicqSetRequestLabel (uimusicq_t *uimusicq, const char *txt)
{
  mq_internal_t  *mqint;

  if (uimusicq == NULL) {
    return;
  }

  mqint = uimusicq->ui [MUSICQ_HISTORY].mqInternalData;
  uiLabelSetText (mqint->wcont [UIMUSICQ_W_REQ_QUEUE], txt);
}

dbidx_t
uimusicqGetSelectionDbidx (uimusicq_t *uimusicq)
{
  int             ci;
  mq_internal_t   *mqint;
  dbidx_t         dbidx = -1;
  int             count;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  if (mqint == NULL) {
    return -1;
  }

  count = uiTreeViewSelectGetCount (mqint->musicqTree);
  if (count != 1) {
    return -1;
  }

  dbidx = uiTreeViewGetValue (mqint->musicqTree, UIMUSICQ_COL_DBIDX);
  return dbidx;
}

/* internal routines */

static bool
uimusicqQueueDanceCallback (void *udata, long idx, int count)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqQueueDanceProcess (uimusicq, idx, count);
  return UICB_CONT;
}

static bool
uimusicqQueuePlaylistCallback (void *udata, long idx)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqQueuePlaylistProcess (uimusicq, idx);
  return UICB_CONT;
}

static void
uimusicqProcessMusicQueueDataNewCallback (int type, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  mq_internal_t *mqint;

  mqint = uimusicq->ui [uimusicq->cbci].mqInternalData;
  mqint->typelist = mdrealloc (mqint->typelist, sizeof (int) * (mqint->colcount + 1));
  mqint->typelist [mqint->colcount] = type;
  ++mqint->colcount;
}

static void
uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq,
    mp_musicqupdate_t *musicqupdate)
{
  const char    *listingFont;
  mp_musicqupditem_t *musicqupditem;
  nlistidx_t    iteridx;
  mq_internal_t *mqint;
  int           ci;
  int           row;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDisplay");

  ci = musicqupdate->mqidx;
  mqint = uimusicq->ui [ci].mqInternalData;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  /* gtk selections will change internally quite a bit, bypass any change */
  /* signals for the moment */
  uimusicq->ui [ci].selchgbypass = true;

  uiTreeViewSelectSave (mqint->musicqTree);
  uiTreeViewSelectFirst (mqint->musicqTree);

  row = 0;
  uimusicq->ui [ci].count = nlistGetCount (musicqupdate->dispList);
  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    song_t        *song;
    void          *pixbuf;

    pixbuf = NULL;
    if (musicqupditem->pauseind) {
      pixbuf = uiImageGetPixbuf (uimusicq->pausePixbuf);
    }

    song = dbGetByIdx (uimusicq->musicdb, musicqupditem->dbidx);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (row >= mqint->rowcount) {
      uiTreeViewValueAppend (mqint->musicqTree);
      uiTreeViewSetValueEllipsize (mqint->musicqTree, UIMUSICQ_COL_ELLIPSIZE);
      uiTreeViewSetValues (mqint->musicqTree,
          UIMUSICQ_COL_FONT, listingFont,
          UIMUSICQ_COL_DISP_IDX_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
          UIMUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) false,
          UIMUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          UIMUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          UIMUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          UIMUSICQ_COL_PAUSEIND, pixbuf,
          TREE_VALUE_END);
    } else {
      uiTreeViewSelectSet (mqint->musicqTree, row);
      /* all data must be updated, except the font and ellipsize */
      uiTreeViewSetValues (mqint->musicqTree,
          UIMUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) false,
          UIMUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          UIMUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          UIMUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          UIMUSICQ_COL_PAUSEIND, pixbuf,
          TREE_VALUE_END);
    }
    uimusicqSetMusicqDisplay (uimusicq, song, ci);

    ++row;
  }

  /* remove any other rows past the end of the display */
  uiTreeViewValueClear (mqint->musicqTree, row);

  mqint->rowcount = row;

  /* now the selection change signal can be processed */
  uimusicq->ui [ci].selchgbypass = false;

  uiTreeViewSelectRestore (mqint->musicqTree);

  if (uimusicq->ui [ci].haveselloc) {
    uimusicqSetSelection (uimusicq, ci);
    uimusicq->ui [ci].haveselloc = false;
  } else {
    uimusicq->ui [ci].selectLocation = uimusicqGetSelectLocation (uimusicq, ci);
    uimusicqSetSelection (uimusicq, ci);
  }

  uimusicqSetDefaultSelection (uimusicq);

  /* the selection may have been changed, but the chg processing was */
  /* purposely bypassed.  Make sure the ui is notified about any change. */
  uimusicqSelectionChgProcess (uimusicq);

  uimusicq->changed = true;
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDisplay", "");
}


static void
uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, song_t *song, int ci)
{
  slist_t       *sellist;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uimusicq->cbci = ci;
  uisongSetDisplayColumns (sellist, song, UIMUSICQ_COL_MAX,
      uimusicqSetMusicqDisplayCallback, uimusicq);
}

static void
uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  mq_internal_t *mqint;
  int           ci;

  ci = uimusicq->cbci;
  mqint = uimusicq->ui [ci].mqInternalData;

  uitreedispSetDisplayColumn (mqint->musicqTree, col, num, str);
}

static bool
uimusicqIterateCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  mq_internal_t *mqint;
  int           ci;
  dbidx_t       dbidx;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  if (mqint == NULL) {
    return UICB_CONT;
  }

  dbidx = uiTreeViewGetValue (mqint->musicqTree, UIMUSICQ_COL_DBIDX);
  if (uimusicq->cbcopy [UIMUSICQ_CBC_ITERATE] != NULL) {
    callbackHandlerLong (uimusicq->cbcopy [UIMUSICQ_CBC_ITERATE], dbidx);
  }
  return UICB_CONT;
}

/* used by song list editor */
static bool
uimusicqPlayCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  mq_internal_t *mqint;
  musicqidx_t   ci;
  dbidx_t       dbidx;
  int           count;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  count = uiTreeViewSelectGetCount (mqint->musicqTree);
  if (count != 1) {
    return UICB_CONT;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);

  /* queue to the hidden music queue */
  uimusicqPlay (uimusicq, MUSICQ_MNG_PB, dbidx);
  return UICB_CONT;
}

/* used by history */
static bool
uimusicqQueueCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  mq_internal_t *mqint;
  musicqidx_t   ci;
  dbidx_t       dbidx;
  int           count;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  count = uiTreeViewSelectGetCount (mqint->musicqTree);
  if (count != 1) {
    return UICB_CONT;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);

  if (uimusicq->cbcopy [UIMUSICQ_CBC_QUEUE] != NULL) {
    callbackHandlerLong (uimusicq->cbcopy [UIMUSICQ_CBC_QUEUE], dbidx);
  }
  return UICB_CONT;
}

static bool
uimusicqSelectionChgCallback (void *udata)
{
  uimusicq_t      *uimusicq = udata;

  uimusicqSelectionChgProcess (uimusicq);
  return UICB_CONT;
}

static void
uimusicqSelectionChgProcess (uimusicq_t *uimusicq)
{
  dbidx_t         dbidx;
  int             ci;
  long            loc;

  ci = uimusicq->musicqManageIdx;

  if (uimusicq->ispeercall) {
    return;
  }

  if (uimusicq->ui [ci].selchgbypass) {
    return;
  }

  loc = uimusicqGetSelectLocation (uimusicq, ci);

  uimusicqSelectionPreviousProcess (uimusicq, loc);

  if (uimusicq->ui [ci].lastLocation == loc) {
    return;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);
  if (dbidx >= 0 &&
      uimusicq->cbcopy [UIMUSICQ_CBC_NEW_SEL] != NULL) {
    callbackHandlerLong (uimusicq->cbcopy [UIMUSICQ_CBC_NEW_SEL], dbidx);
  }

  if (loc != QUEUE_LOC_LAST) {
    uimusicq->ui [ci].lastLocation = loc;
  }

  for (int i = 0; i < uimusicq->peercount; ++i) {
    if (uimusicq->peers [i] == NULL) {
      continue;
    }
    uimusicqSetPeerFlag (uimusicq->peers [i], true);
    uimusicqSetSelectLocation (uimusicq->peers [i], ci, loc);
    uimusicqSetPeerFlag (uimusicq->peers [i], false);
  }
}

void
uimusicqSelectionPreviousProcess (uimusicq_t *uimusicq, long loc)
{
  int   ci;
  bool  sameprev = false;

  ci = uimusicq->musicqManageIdx;

  if (uimusicq->ui [ci].currSelection == loc) {
    sameprev = true;
  }

  if (! sameprev) {
    uimusicqMarkPreviousSelection (uimusicq, false);
    uimusicq->ui [ci].prevSelection = uimusicq->ui [ci].currSelection;
    uimusicq->ui [ci].currSelection = loc;
  }
  uimusicqMarkPreviousSelection (uimusicq, true);

  for (int i = 0; i < uimusicq->peercount; ++i) {
    if (uimusicq->peers [i] == NULL) {
      continue;
    }
    uimusicqSetPeerFlag (uimusicq->peers [i], true);
    if (! sameprev) {
      uimusicqCopySelections (uimusicq, uimusicq->peers [i], ci);
    }
    uimusicqMarkPreviousSelection (uimusicq->peers [i], true);
    uimusicqSetPeerFlag (uimusicq->peers [i], false);
  }
}

static void
uimusicqSetDefaultSelection (uimusicq_t *uimusicq)
{
  mq_internal_t  *mqint;
  int             ci;
  int             count;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  count = uiTreeViewSelectGetCount (mqint->musicqTree);
  if (uimusicq->ui [ci].count > 0 && count < 1) {
    uiTreeViewSelectFirst (mqint->musicqTree);
  }

  return;
}

static void
uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx)
{
  mq_internal_t *mqint;

  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "no-ui");
    return;
  }
  mqint = uimusicq->ui [mqidx].mqInternalData;

  if (uimusicq->ui [mqidx].selectLocation == QUEUE_LOC_LAST) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "select-999");
    return;
  }
  if (uimusicq->ui [mqidx].selectLocation < 0) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "select-not-set");
    return;
  }

  uiTreeViewSelectSet (mqint->musicqTree, uimusicq->ui [mqidx].selectLocation);

  if (uimusicq->ui [mqidx].count > 0) {
    uiTreeViewScrollToCell (mqint->musicqTree);
  }
  logProcEnd (LOG_PROC, "uimusicqSetSelection", "");
}

/* have to handle the case where the user switches tabs back to the */
/* music queue (song list) tab */
static bool
uimusicqSongEditCallback (void *udata)
{
  uimusicq_t      *uimusicq = udata;
  dbidx_t         dbidx;


  if (uimusicq->cbcopy [UIMUSICQ_CBC_NEW_SEL] != NULL) {
    dbidx = uimusicqGetSelectionDbidx (uimusicq);
    if (dbidx < 0) {
      return UICB_CONT;
    }
    callbackHandlerLong (uimusicq->cbcopy [UIMUSICQ_CBC_NEW_SEL], dbidx);
  }
  return callbackHandler (uimusicq->cbcopy [UIMUSICQ_CBC_EDIT]);
}

static bool
uimusicqMoveTopCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  long        idx;


  logProcBegin (LOG_PROC, "uimusicqMoveTopProcess");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveTopProcess", "bad-idx");
    return UICB_STOP;
  }
  ++idx;

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_TOP);
  uimusicqMoveTop (uimusicq, ci, idx);
  return UICB_CONT;
}

static bool
uimusicqMoveUpCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  long        idx;


  logProcBegin (LOG_PROC, "uimusicqMoveUp");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveUp", "bad-idx");
    return UICB_STOP;
  }
  ++idx;

  if (idx > 1) {
    uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
  }
  uimusicqMoveUp (uimusicq, ci, idx);
  return UICB_CONT;
}

static bool
uimusicqMoveDownCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  long        idx;

  logProcBegin (LOG_PROC, "uimusicqMoveDown");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveDown", "bad-idx");
    return UICB_STOP;
  }
  ++idx;

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_NEXT);
  uimusicqMoveDown (uimusicq, ci, idx);
  logProcEnd (LOG_PROC, "uimusicqMoveDown", "");
  return UICB_CONT;
}

static bool
uimusicqTogglePauseCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  long          idx;


  logProcBegin (LOG_PROC, "uimusicqTogglePause");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqTogglePause", "bad-idx");
    return UICB_STOP;
  }
  ++idx;

  uimusicqTogglePause (uimusicq, ci, idx);
  logProcEnd (LOG_PROC, "uimusicqTogglePause", "");
  return UICB_CONT;
}

static bool
uimusicqRemoveCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  long          idx;


  logProcBegin (LOG_PROC, "uimusicqRemove");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqRemove", "bad-idx");
    return UICB_STOP;
  }
  ++idx;

  uimusicqRemove (uimusicq, ci, idx);
  logProcEnd (LOG_PROC, "uimusicqRemove", "");
  return UICB_CONT;
}

static bool
uimusicqCheckFavChgCallback (void *udata, long col)
{
  uimusicq_t    * uimusicq = udata;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin (LOG_PROC, "uimusicqCheckFavChgCallback");

  if (col == TREE_NO_COLUMN) {
    logProcEnd (LOG_PROC, "uimusicqCheckFavChgCallback", "not-fav-col");
    return UICB_CONT;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: %s: change favorite", uimusicq->tag);
  dbidx = uimusicqGetSelectionDbidx (uimusicq);
  song = dbGetByIdx (uimusicq->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uimusicq->cbcopy [UIMUSICQ_CBC_SONG_SAVE] != NULL) {
      callbackHandlerLong (uimusicq->cbcopy [UIMUSICQ_CBC_SONG_SAVE], dbidx);
    }
  }

  return UICB_CONT;
}

static bool
uimusicqKeyEvent (void *udata)
{
  uimusicq_t      *uimusicq = udata;
  mq_internal_t   *mqint;
  int             ci;

  if (uimusicq == NULL) {
    return UICB_CONT;
  }

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  if (uiKeyIsPressEvent (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
    if (uiKeyIsAudioPlayKey (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
      uimusicqPlayCallback (uimusicq);
    }
    if (uiKeyIsKey (mqint->wcont [UIMUSICQ_W_KEY_HNDLR], 'S')) {
      uimusicqSwap (uimusicq, ci);
    }
  }

  if (uiKeyIsControlPressed (mqint->wcont [UIMUSICQ_W_KEY_HNDLR]) &&
      uiKeyIsMovementKey (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
    if (uiKeyIsPressEvent (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
      if (uiKeyIsUpKey (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
        uimusicqMoveUpCallback (uimusicq);
      }
      if (uiKeyIsDownKey (mqint->wcont [UIMUSICQ_W_KEY_HNDLR])) {
        uimusicqMoveDownCallback (uimusicq);
      }
    }

    /* control-up/down is handled internally */
    return UICB_STOP;
  }

  return UICB_CONT;
}

static void
uimusicqMarkPreviousSelection (uimusicq_t *uimusicq, bool disp)
{
  int           ci;
  mq_internal_t *mqint;

  ci = uimusicq->musicqManageIdx;
  if (ci != MUSICQ_SL) {
    /* only the song list queue has the previous selection marked */
    return;
  }

  mqint = uimusicq->ui [ci].mqInternalData;

  if (! uimusicq->ui [ci].hasui) {
    return;
  }
  if (uimusicq->ui [ci].prevSelection < 0) {
    return;
  }
  if (uimusicq->ui [ci].prevSelection >= uimusicq->ui [ci].count) {
    return;
  }

  uiTreeViewValueIteratorSet (mqint->musicqTree, uimusicq->ui [ci].prevSelection);
  uiTreeViewSetValues (mqint->musicqTree,
      UIMUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) disp,
      TREE_VALUE_END);
  uiTreeViewValueIteratorClear (mqint->musicqTree);
}

static void
uimusicqCopySelections (uimusicq_t *uimusicq, uimusicq_t *peer, int mqidx)
{
  uimusicqMarkPreviousSelection (peer, false);
  peer->ui [mqidx].prevSelection = uimusicq->ui [mqidx].prevSelection;
  peer->ui [mqidx].currSelection = uimusicq->ui [mqidx].currSelection;
}

