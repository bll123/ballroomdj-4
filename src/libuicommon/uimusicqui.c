/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "dispsel.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "playlist.h"
#include "songfav.h"
#include "ui.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uimusicq.h"
#include "uiplaylist.h"
#include "uisong.h"
#include "uiutils.h"
#include "uivirtlist.h"
#include "uivlutil.h"

enum {
  UIMUSICQ_COL_DBIDX,
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
  MQINT_CB_DISP_CHG,
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
  UIMUSICQ_W_MAX,
};

enum {
  MQ_IDENT_INT      = 0xbbaa00746e69716d,
};

typedef struct mq_internal {
  uint64_t          ident;
  char              tag [100];
  uimusicq_t        *uimusicq;
  uidance_t         *uidance;
  uidance_t         *uidance5;
  callback_t        *callbacks [MQINT_CB_MAX];
  uiwcont_t         *wcont [UIMUSICQ_W_MAX];
  uivirtlist_t      *uivl;
  slist_t           *sellist;
  mp_musicqupdate_t *musicqupdate;
  int               colcount;
  int               favcolumn;
  int               mqidx;
  bool              inchange;
  bool              inprocess;
} mq_internal_t;

static bool   uimusicqQueueDanceCallback (void *udata, int32_t idx, int32_t count);
static int32_t uimusicqQueuePlaylistCallback (void *udata, const char *sval);

static void   uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq, int mqidx);

static bool   uimusicqPlayCallback (void *udata);
static bool   uimusicqQueueCallback (void *udata);
static void   uimusicqProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static bool   uimusicqProcessDisplayChg (void *udata);
static void   uimusicqSelectionChgProcess (uimusicq_t *uimusicq);
static void   uimusicqSelectionPreviousProcess (uimusicq_t *uimusicq, nlistidx_t loc);
static void   uimusicqRowClickCB (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);
static void   uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx);
static bool   uimusicqSongEditCallback (void *udata);

static bool   uimusicqMoveTopCallback (void *udata);
static bool   uimusicqMoveUpCallback (void *udata);
static bool   uimusicqMoveDownCallback (void *udata);
static bool   uimusicqTogglePauseCallback (void *udata);
static bool   uimusicqRemoveCallback (void *udata);
static bool   uimusicqKeyEvent (void *udata);
static void   uimusicqMarkPreviousSelection (uimusicq_t *uimusicq, bool disp);
static void   uimusicqFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);

void
uimusicqUIInit (uimusicq_t *uimusicq)
{
  mq_internal_t   *mqint;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mqint = mdmalloc (sizeof (mq_internal_t));
    mqint->ident = MQ_IDENT_INT;
    mqint->mqidx = i;
    mqint->uimusicq = uimusicq;
    uimusicq->ui [i].mqInternalData = mqint;
    mqint->uidance = NULL;
    mqint->uidance5 = NULL;
    for (int j = 0; j < MQINT_CB_MAX; ++j) {
      mqint->callbacks [j] = NULL;
    }
    for (int j = 0; j < UIMUSICQ_W_MAX; ++j) {
      mqint->wcont [j] = NULL;
    }
    mqint->uivl = NULL;
    mqint->musicqupdate = NULL;
    mqint->sellist = NULL;
    mqint->favcolumn = -1;
    mqint->inchange = false;
    mqint->inprocess = false;
    snprintf (mqint->tag, sizeof (mqint->tag), "%s-mq-%d", uimusicq->tag, i);
  }

  uiutilsAddFavoriteClasses ();
}

void
uimusicqUIFree (uimusicq_t *uimusicq)
{
  mq_internal_t   *mqint;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    mqint = uimusicq->ui [i].mqInternalData;
    if (mqint != NULL) {
      uivlFree (mqint->uivl);
      for (int j = 0; j < MQINT_CB_MAX; ++j) {
        callbackFree (mqint->callbacks [j]);
      }
      for (int j = 0; j < UIMUSICQ_W_MAX; ++j) {
        uiwcontFree (mqint->wcont [j]);
      }
      uidanceFree (mqint->uidance);
      uidanceFree (mqint->uidance5);
      mdfree (mqint);
    }
  }
}

uiwcont_t *
uimusicqBuildUI (uimusicq_t *uimusicq, uiwcont_t *parentwin, int ci,
    uiwcont_t *errorMsg, uiwcont_t *statusMsg, uientryval_t validateFunc)
{
  int               saveci;
  int               startcol = 0;
  uiwcont_t         *hbox;
  uiwcont_t         *uiwidgetp;
  slist_t           *sellist;
  slistidx_t        iteridx;
  int               colidx;
  int               tagidx;
  mq_internal_t     *mqint;
  uivirtlist_t      *uivl;


  logProcBegin ();

  saveci = uimusicq->musicqManageIdx;
  /* temporary */
  uimusicq->musicqManageIdx = ci;
  mqint = uimusicq->ui [ci].mqInternalData;
  uimusicq->errorMsg = errorMsg;
  uimusicq->statusMsg = statusMsg;

  uimusicq->ui [ci].hasui = true;

  uimusicq->parentwin = parentwin;

  uimusicq->ui [ci].mainbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (uimusicq->ui [ci].mainbox);
  uiWidgetExpandVert (uimusicq->ui [ci].mainbox);

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    hbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (uimusicq->ui [ci].mainbox, hbox);
    uiWidgetSetMarginTop (hbox, 1);

    /* CONTEXT: music queue: label for song list name */
    uiwidgetp = uiCreateColonLabel (_("Song List"));
    uiBoxPackStart (hbox, uiwidgetp);
    uiWidgetSetMarginStart (uiwidgetp, 2);
    uiWidgetSetMarginEnd (uiwidgetp, 2);
    uiwcontFree (uiwidgetp);

    uiwidgetp = uiEntryInit (30, 100);
    uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
    if (uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
      uiBoxPackStartExpand (hbox, uiwidgetp);
      uiWidgetExpandHoriz (uiwidgetp);
      uiWidgetAlignHorizFill (uiwidgetp);
    }
    if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST) {
      uiBoxPackStart (hbox, uiwidgetp);
    }
    uimusicq->ui [ci].slname = uiwidgetp;
    if (validateFunc != NULL) {
      /* CONTEXT: music queue: song list name */
      uiEntrySetValidate (uiwidgetp, _("Song List"),
          validateFunc, errorMsg, UIENTRY_IMMEDIATE);
    }

    uiwcontFree (hbox);
  }

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uimusicq->ui [ci].mainbox, hbox);
  uiWidgetSetMarginTop (hbox, 1);

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
        NULL, "button_movetop", _("Move to Top"));
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_TOP] = uiwidgetp;

    mqint->callbacks [MQINT_CB_MOVE_UP] = callbackInit (
        uimusicqMoveUpCallback, uimusicq, "musicq: move-up");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_MOVE_UP],
        /* CONTEXT: music queue: button: move the selected song up in the queue */
        NULL, "button_up", _("Move Up"));
    uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_MOVE_UP] = uiwidgetp;

    mqint->callbacks [MQINT_CB_MOVE_DOWN] = callbackInit (
        uimusicqMoveDownCallback, uimusicq, "musicq: move-down");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_MOVE_DOWN],
        /* CONTEXT: music queue: button: move the selected song down in the queue */
        NULL, "button_down", _("Move Down"));
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
        NULL, "button_pause", _("Toggle Pause"));
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
        NULL, "button_audioremove", _("Remove"));
    uiBoxPackStart (hbox, uiwidgetp);
    uiWidgetSetMarginStart (uiwidgetp, 3);
    uiWidgetSetMarginEnd (uiwidgetp, 2);
    mqint->wcont [UIMUSICQ_W_BUTTON_REMOVE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    mqint->callbacks [MQINT_CB_CLEAR_QUEUE] = callbackInit (
        uimusicqTruncateQueueCallback, uimusicq, "musicq: clear-queue");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_CLEAR_QUEUE],
        /* CONTEXT: music queue: button: clear the queue */
        _("Clear Queue"), NULL, NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    uiWidgetSetMarginStart (uiwidgetp, 2);
    mqint->wcont [UIMUSICQ_W_BUTTON_CLEAR_QUEUE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->callbacks [MQINT_CB_EDIT_LOCAL] = callbackInit (
        uimusicqSongEditCallback, uimusicq, "musicq: edit");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_EDIT_LOCAL],
        /* CONTEXT: music queue: edit the selected song */
        NULL, "button_edit", _("Edit"));
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_EDIT] = uiwidgetp;

    mqint->callbacks [MQINT_CB_PLAY] = callbackInit (
        uimusicqPlayCallback, uimusicq, "musicq: play");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_PLAY],
        /* CONTEXT: music queue: tooltip: play the selected song */
        NULL, "button_play", _("Play"));
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_PLAY] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_HISTORY) {
    mqint->callbacks [MQINT_CB_HIST_QUEUE] = callbackInit (
        uimusicqQueueCallback, uimusicq, "musicq: queue");
    uiwidgetp = uiCreateButton (
        mqint->callbacks [MQINT_CB_HIST_QUEUE],
        /* CONTEXT: (verb) history: re-queue the selected song: suggested: 'put song in queue' */
        _("Queue"), NULL, NULL);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_BUTTON_QUEUE] = uiwidgetp;

    uiwidgetp = uiCreateLabel ("");
    uiWidgetAddClass (uiwidgetp, DARKACCENT_CLASS);
    uiBoxPackStart (hbox, uiwidgetp);
    mqint->wcont [UIMUSICQ_W_REQ_QUEUE] = uiwidgetp;
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    if (uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST] == NULL) {
      uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST] = callbackInitS (
          uimusicqQueuePlaylistCallback, uimusicq);
    }
    uimusicq->ui [ci].playlistsel = uiplaylistCreate (parentwin,
        /* CONTEXT: (verb) music queue: button: queue a playlist for playback: suggested 'put playlist in queue' */
        hbox, PL_LIST_NORMAL, _("Queue Playlist"), UIPL_PACK_END, UIPL_FLAG_NONE);
    uiplaylistSetSelectCallback (uimusicq->ui [ci].playlistsel,
        uimusicq->callbacks [UIMUSICQ_CB_QUEUE_PLAYLIST]);

    if (bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, ci)) {
      if (uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE] == NULL) {
        uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE] = callbackInitII (
            uimusicqQueueDanceCallback, uimusicq);
      }
      mqint->uidance5 = uidanceCreate (hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue 5 dances for playback: suggested '5 in queue' (this button gets context from the 'queue dance' button) */
          UIDANCE_NONE, _("Queue 5"), UIDANCE_PACK_END, 5);
      uidanceSetCallback (mqint->uidance5, uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE]);

      mqint->uidance = uidanceCreate (hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue a dance for playback: suggested: put dance in queue */
          UIDANCE_NONE, _("Queue Dance"), UIDANCE_PACK_END, 1);
      uidanceSetCallback (mqint->uidance, uimusicq->callbacks [UIMUSICQ_CB_QUEUE_DANCE]);
    }
  }

  /* musicq listing display */

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  mqint->sellist = sellist;

  colidx = 0;
  slistStartIterator (sellist, &iteridx);
  while ((tagidx = slistIterateValueNum (sellist, &iteridx)) >= 0) {
    if (tagidx == TAG_FAVORITE) {
      mqint->favcolumn = UIMUSICQ_COL_MAX + colidx;
      break;
    }
    ++colidx;
  }

  uivl = uivlCreate (mqint->tag, NULL,
      uimusicq->ui [ci].mainbox, 7, 300, VL_ENABLE_KEYS);
  mqint->uivl = uivl;
  uivlSetUseListingFont (uivl);

  startcol = UIMUSICQ_COL_MAX;
  mqint->colcount = UIMUSICQ_COL_MAX + slistGetCount (sellist);
  uivlSetNumColumns (uivl, mqint->colcount);

  uivlMakeColumn (uivl, "dbidx", UIMUSICQ_COL_DBIDX, VL_TYPE_INTERNAL_NUMERIC);
  uivlMakeColumn (uivl, "dispidx", UIMUSICQ_COL_DISP_IDX, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, UIMUSICQ_COL_DISP_IDX, VL_COL_WIDTH_GROW_ONLY);
  uivlSetColumnAlignEnd (uivl, UIMUSICQ_COL_DISP_IDX);

  /* always create the pause-indicator column */
  uivlMakeColumn (uivl, "pauseind", UIMUSICQ_COL_PAUSEIND, VL_TYPE_IMAGE);
  uivlSetColumnGrow (uivl, UIMUSICQ_COL_PAUSEIND, VL_COL_WIDTH_GROW_ONLY);
  if (uimusicq->ui [ci].dispselType != DISP_SEL_MUSICQ) {
    uivlSetColumnDisplay (uivl, UIMUSICQ_COL_PAUSEIND, VL_COL_DISABLE);
  }

  uivlAddDisplayColumns (uivl, sellist, startcol);

  uivlSetRowFillCallback (uivl, uimusicqFillRow, mqint);
  uivlSetNumRows (mqint->uivl, 0);

  uivlDisplay (mqint->uivl);

  uivlSetSelectChgCallback (mqint->uivl, uimusicqProcessSelectChg, uimusicq);
  mqint->callbacks [MQINT_CB_DISP_CHG] = callbackInit (
        uimusicqProcessDisplayChg, uimusicq, NULL);
  uivlSetDisplayChgCallback (mqint->uivl, mqint->callbacks [MQINT_CB_DISP_CHG]);
  uivlSetRowClickCallback (mqint->uivl, uimusicqRowClickCB, uimusicq);

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SBS_SONGLIST) {
    mqint->callbacks [MQINT_CB_KEYB] = callbackInit (
        uimusicqKeyEvent, uimusicq, NULL);
    uivlSetKeyCallback (mqint->uivl, mqint->callbacks [MQINT_CB_KEYB]);
  }

  uiwcontFree (hbox);

  uimusicq->musicqManageIdx = saveci;
  uimusicq->cbci = ci;

  logProcEnd ("");
  return uimusicq->ui [ci].mainbox;
}

void
uimusicqDragDropSetURICallback (uimusicq_t *uimusicq, int ci, callback_t *cb)
{
  if (ci < 0 || ci >= MUSICQ_DISP_MAX) {
    return;
  }

  uiDragDropSetDestURICallback (uimusicq->parentwin, cb);
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
  int               count = 0;
  mq_internal_t     *mqint;
  int32_t           idx;


  logProcBegin ();
  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd ("no-ui");
    return;
  }
  mqint = uimusicq->ui [mqidx].mqInternalData;

  count = uivlSelectionCount (mqint->uivl);
  if (count != 1) {
    logProcEnd ("count-not-1");
    return;
  }

  idx = uimusicqGetSelectLocation (uimusicq, mqidx);

  switch (which) {
    case UIMUSICQ_SEL_CURR: {
      break;
    }
    case UIMUSICQ_SEL_PREV: {
      uivlSetSelection (mqint->uivl, idx - 1);
      break;
    }
    case UIMUSICQ_SEL_NEXT: {
      uivlSetSelection (mqint->uivl, idx + 1);
      break;
    }
    case UIMUSICQ_SEL_TOP: {
      uivlSetSelection (mqint->uivl, 0);
      break;
    }
  }

  logProcEnd ("");
}

nlistidx_t
uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx)
{
  mq_internal_t *mqint;
  nlistidx_t    loc;

  mqint = uimusicq->ui [mqidx].mqInternalData;

  loc = QUEUE_LOC_LAST;
  if (mqint->uivl == NULL) {
    return loc;
  }

  loc = uivlGetCurrSelection (mqint->uivl);
  return loc;
}

void
uimusicqSetSelectLocation (uimusicq_t *uimusicq, int mqidx, nlistidx_t loc)
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
  nlistidx_t    idx;


  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: %s: clear queue", uimusicq->tag);

  ci = uimusicq->musicqManageIdx;

  if (uimusicq->usercb [UIMUSICQ_USER_CB_CLEAR_QUEUE] != NULL) {
    callbackHandler (uimusicq->usercb [UIMUSICQ_USER_CB_CLEAR_QUEUE]);
  }

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    /* if nothing is selected, clear everything */
    idx = 0;
    if (uimusicq->musicqManageIdx == uimusicq->musicqPlayIdx) {
      idx = 1;
    }
  }

  uimusicqTruncateQueue (uimusicq, ci, idx);
  logProcEnd ("");
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

/* only stores the musicqupdate structure pointer */
void
uimusicqSetMusicQueueData (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate)
{
  int            ci;
  mq_internal_t  *mqint;


  logProcBegin ();

  ci = musicqupdate->mqidx;
  if (ci < 0 || ci >= MUSICQ_MAX) {
    logProcEnd ("bad-mq-idx");
    return;
  }

  if (! uimusicq->ui [ci].hasui) {
    logProcEnd ("no-ui");
    return;
  }

  ci = musicqupdate->mqidx;
  mqint = uimusicq->ui [ci].mqInternalData;

  mqint->musicqupdate = musicqupdate;

  logProcEnd ("");
}

void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, int mqidx)
{
  logProcBegin ();

  if (uimusicq == NULL) {
    logProcEnd ("null");
    return;
  }
  if (mqidx < 0 || mqidx >= MUSICQ_MAX) {
    logProcEnd ("bad-mq-idx");
    return;
  }

  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd ("no-ui");
    return;
  }

  uimusicqProcessMusicQueueDisplay (uimusicq, mqidx);
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
uimusicqGetSelectionDBidx (uimusicq_t *uimusicq)
{
  int             ci;
  mq_internal_t   *mqint;
  dbidx_t         dbidx = -1;
  int             count;
  int32_t         idx;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  if (mqint == NULL) {
    return -1;
  }

  count = uivlSelectionCount (mqint->uivl);
  if (count != 1) {
    return -1;
  }

  idx = uivlGetCurrSelection (mqint->uivl);
  if (idx < 0) {
    return -1;
  }
  dbidx = uivlGetRowColumnNum (mqint->uivl, idx, UIMUSICQ_COL_DBIDX);
  return dbidx;
}

void
uimusicqCopySelectList (uimusicq_t *uimusicq, uimusicq_t *peer)
{
  mq_internal_t   *mqint;
  int             ci_a;
  int             ci_b;
  uivirtlist_t    *vl_a;
  uivirtlist_t    *vl_b;

  ci_a = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci_a].mqInternalData;
  vl_a = mqint->uivl;

  ci_b = peer->musicqManageIdx;
  mqint = peer->ui [ci_b].mqInternalData;
  vl_b = mqint->uivl;

  if (vl_b == NULL) {
    /* the peer has not yet been initialized, even though it may exist */
    return;
  }

  mqint = NULL;
  uivlCopySelectList (vl_a, vl_b);
  uimusicq->ui [ci_b].currSelection = uimusicq->ui [ci_a].prevSelection;
  uimusicqSelectionPreviousProcess (peer, uimusicq->ui [ci_a].currSelection);
}

/* internal routines */

static bool
uimusicqQueueDanceCallback (void *udata, int32_t idx, int32_t count)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqQueueDanceProcess (uimusicq, idx, count);
  return UICB_CONT;
}

static int32_t
uimusicqQueuePlaylistCallback (void *udata, const char *sval)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqQueuePlaylistProcess (uimusicq, sval);
  return UICB_CONT;
}

static void
uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq, int mqidx)
{
  mq_internal_t *mqint;

  logProcBegin ();

  mqint = uimusicq->ui [mqidx].mqInternalData;
  if (mqint->musicqupdate == NULL) {
    return;
  }

  mqint->inprocess = true;

  uimusicq->ui [mqidx].rowcount = nlistGetCount (mqint->musicqupdate->dispList);
  /* set-num-rows calls uivlpopulate */
  uivlSetNumRows (mqint->uivl, uimusicq->ui [mqidx].rowcount);

  if (uimusicq->ui [mqidx].haveselloc) {
    /* this happens on an insert */
    uimusicqSetSelection (uimusicq, mqidx);
    uimusicq->ui [mqidx].haveselloc = false;
  } else {
    /* want to keep the selected location, not the specific song */
    uimusicq->ui [mqidx].selectLocation = uimusicqGetSelectLocation (uimusicq, mqidx);
    uimusicqSetSelection (uimusicq, mqidx);
  }

  mqint->inprocess = false;

  /* the selection may have been changed, */
  /* Make sure the ui is notified about any change. */
  uimusicqSelectionChgProcess (uimusicq);

  uimusicq->changed = true;
  logProcEnd ("");
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

  count = uivlSelectionCount (mqint->uivl);
  if (count != 1) {
    return UICB_CONT;
  }

  dbidx = uimusicqGetSelectionDBidx (uimusicq);

  /* queue to the hidden music queue */
  uimusicqPlay (uimusicq, MUSICQ_MNG_PB, dbidx);
  return UICB_CONT;
}

/* used by history */
static bool
uimusicqQueueCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  dbidx_t       dbidx;

  dbidx = uimusicqGetSelectionDBidx (uimusicq);
  if (dbidx < 0) {
    return UICB_CONT;
  }

  if (uimusicq->usercb [UIMUSICQ_USER_CB_QUEUE] != NULL) {
    callbackHandlerI (uimusicq->usercb [UIMUSICQ_USER_CB_QUEUE], dbidx);
  }
  return UICB_CONT;
}

static void
uimusicqProcessSelectChg (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uimusicq_t      *uimusicq = udata;

  uimusicqSelectionChgProcess (uimusicq);
}

static void
uimusicqSelectionChgProcess (uimusicq_t *uimusicq)
{
  dbidx_t         dbidx;
  int             ci;
  nlistidx_t      loc;
  mq_internal_t   *mqint;

  if (uimusicq == NULL) {
    return;
  }

  ci = uimusicq->musicqManageIdx;
  if (ci < 0 || ci >= MUSICQ_MAX) {
    return;
  }

  mqint = uimusicq->ui [ci].mqInternalData;

  if (mqint->inprocess) {
    return;
  }

  loc = uimusicqGetSelectLocation (uimusicq, ci);
  if (loc < 0) {
    return;
  }

  uimusicqSelectionPreviousProcess (uimusicq, loc);

  if (uimusicq->ui [ci].lastLocation == loc) {
    return;
  }

  dbidx = uimusicqGetSelectionDBidx (uimusicq);
  if (dbidx >= 0 &&
      uimusicq->usercb [UIMUSICQ_USER_CB_NEW_SEL] != NULL) {
    callbackHandlerI (uimusicq->usercb [UIMUSICQ_USER_CB_NEW_SEL], dbidx);
  }

  if (loc != QUEUE_LOC_LAST) {
    uimusicq->ui [ci].lastLocation = loc;
  }
}

static bool
uimusicqProcessDisplayChg (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  mq_internal_t     *mqint;
  int               ci;

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;

  if (mqint->inchange) {
    return UICB_CONT;
  }

  return UICB_CONT;
}

/* saves and marks the previous selection */
void
uimusicqSelectionPreviousProcess (uimusicq_t *uimusicq, nlistidx_t loc)
{
  int           ci;
  bool          sameprev = false;

  ci = uimusicq->musicqManageIdx;

  if (uimusicq->ui [ci].currSelection == loc) {
    sameprev = true;
  }

  uimusicqMarkPreviousSelection (uimusicq, false);
  if (! sameprev) {
    uimusicq->ui [ci].prevSelection = uimusicq->ui [ci].currSelection;
    uimusicq->ui [ci].currSelection = loc;
  }
  uimusicqMarkPreviousSelection (uimusicq, true);
}

static void
uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx)
{
  mq_internal_t *mqint;

  logProcBegin ();

  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd ("no-ui");
    return;
  }
  mqint = uimusicq->ui [mqidx].mqInternalData;

  if (uimusicq->ui [mqidx].selectLocation == QUEUE_LOC_LAST) {
    logProcEnd ("select-999");
    return;
  }
  if (uimusicq->ui [mqidx].selectLocation < 0) {
    logProcEnd ("select-not-set");
    return;
  }

  uivlSetSelection (mqint->uivl, uimusicq->ui [mqidx].selectLocation);
  logProcEnd ("");
}

/* have to handle the case where the user switches tabs back to the */
/* music queue (song list) tab */
static bool
uimusicqSongEditCallback (void *udata)
{
  uimusicq_t      *uimusicq = udata;
  dbidx_t         dbidx;


  if (uimusicq->usercb [UIMUSICQ_USER_CB_NEW_SEL] != NULL) {
    dbidx = uimusicqGetSelectionDBidx (uimusicq);
    if (dbidx < 0) {
      return UICB_CONT;
    }
    callbackHandlerI (uimusicq->usercb [UIMUSICQ_USER_CB_NEW_SEL], dbidx);
  }
  return callbackHandler (uimusicq->usercb [UIMUSICQ_USER_CB_EDIT]);
}

static bool
uimusicqMoveTopCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  nlistidx_t  idx;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd ("bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_TOP);
  uimusicqMoveTop (uimusicq, ci, idx);
  return UICB_CONT;
}

static bool
uimusicqMoveUpCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  nlistidx_t  idx;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd ("bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
  uimusicqMoveUp (uimusicq, ci, idx);
  return UICB_CONT;
}

static bool
uimusicqMoveDownCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  nlistidx_t  idx;

  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd ("bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_NEXT);
  uimusicqMoveDown (uimusicq, ci, idx);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uimusicqTogglePauseCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  nlistidx_t    idx;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd ("bad-idx");
    return UICB_STOP;
  }

  uimusicqTogglePause (uimusicq, ci, idx);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uimusicqRemoveCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  nlistidx_t    idx;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelectLocation (uimusicq, ci);
  if (idx < 0) {
    logProcEnd ("bad-idx");
    return UICB_STOP;
  }

  uimusicqRemove (uimusicq, ci, idx);
  logProcEnd ("");
  return UICB_CONT;
}

static void
uimusicqRowClickCB (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uimusicq_t    * uimusicq = udata;
  mq_internal_t *mqint;
  dbidx_t       dbidx;
  song_t        *song;
  int           ci;

  logProcBegin ();

  if (colidx < 0) {
    return;
  }

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;
  if (mqint->favcolumn < 0 || colidx != mqint->favcolumn) {
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: %s: change favorite", uimusicq->tag);
  dbidx = uimusicqGetSelectionDBidx (uimusicq);
  if (dbidx < 0) {
    return;
  }

  song = dbGetByIdx (uimusicq->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uimusicq->usercb [UIMUSICQ_USER_CB_SONG_SAVE] != NULL) {
      callbackHandlerI (uimusicq->usercb [UIMUSICQ_USER_CB_SONG_SAVE], dbidx);
    }
  }

  return;
}

static bool
uimusicqKeyEvent (void *udata)
{
  uimusicq_t      *uimusicq = udata;
  mq_internal_t   *mqint;
  int             ci;
  uiwcont_t       *keyh;

  if (uimusicq == NULL) {
    return UICB_CONT;
  }

  ci = uimusicq->musicqManageIdx;
  mqint = uimusicq->ui [ci].mqInternalData;
  keyh = uivlGetEventHandler (mqint->uivl);

  if (uiEventIsKeyPressEvent (keyh)) {
    if (uiEventIsAudioPlayKey (keyh)) {
      uimusicqPlayCallback (uimusicq);
      return UICB_STOP;
    }
    if (uiEventIsControlPressed (keyh) && uiEventIsKey (keyh, 'S')) {
      uimusicqSwap (uimusicq, ci);
      return UICB_STOP;
    }
  }

  return UICB_CONT;
}

static void
uimusicqMarkPreviousSelection (uimusicq_t *uimusicq, bool disp)
{
  int           ci;
  mq_internal_t *mqint;
  uimusicqui_t  *mqui;

  ci = uimusicq->musicqManageIdx;
  mqui = &uimusicq->ui [ci];
  if (ci != MUSICQ_SL) {
    /* only the song list queue has the previous selection marked */
    return;
  }

  mqint = mqui->mqInternalData;

  if (! mqui->hasui) {
    return;
  }
  if (mqui->prevSelection < 0) {
    return;
  }
  if (mqui->prevSelection >= mqui->rowcount) {
    return;
  }

  if (! disp) {
    uivlClearRowColumnClass (mqint->uivl, mqui->prevSelection,
        UIMUSICQ_COL_DISP_IDX);
  }
  if (disp) {
    uivlSetRowColumnClass (mqint->uivl, mqui->prevSelection,
        UIMUSICQ_COL_DISP_IDX, ACCENT_CLASS);
  }
}

static void
uimusicqFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  mq_internal_t       *mqint = udata;
  uimusicq_t          *uimusicq = mqint->uimusicq;
  mp_musicqupditem_t  *musicqupditem;
  song_t              *song;
  uiwcont_t           *pauseimg = NULL;
  nlist_t             *tdlist;
  slistidx_t          seliteridx;
  char                tmp [40];

  if (mqint == NULL || uimusicq == NULL) {
    return;
  }

  if (mqint->musicqupdate == NULL) {
    return;
  }

  mqint->inchange = true;

  musicqupditem = nlistGetData (mqint->musicqupdate->dispList, rownum);
  if (musicqupditem == NULL) {
    mqint->inchange = false;
    return;
  }

  song = dbGetByIdx (uimusicq->musicdb, musicqupditem->dbidx);
  if (song == NULL) {
    mqint->inchange = false;
    return;
  }

  uivlSetRowColumnNum (mqint->uivl, rownum, UIMUSICQ_COL_DBIDX,
      musicqupditem->dbidx);
  snprintf (tmp, sizeof (tmp), "%d", musicqupditem->dispidx);
  uivlSetRowColumnStr (mqint->uivl, rownum, UIMUSICQ_COL_DISP_IDX, tmp);

  if (musicqupditem->pauseind) {
    pauseimg = uimusicq->pauseImage;
  }
  uivlSetRowColumnImage (mqint->uivl, rownum, UIMUSICQ_COL_PAUSEIND,
      pauseimg, 20);

  tdlist = uisongGetDisplayList (mqint->sellist, NULL, song);
  slistStartIterator (mqint->sellist, &seliteridx);
  for (int colidx = UIMUSICQ_COL_MAX; colidx < mqint->colcount; ++colidx) {
    int         tagidx;
    const char  *str;

    tagidx = slistIterateValueNum (mqint->sellist, &seliteridx);
    str = nlistGetStr (tdlist, tagidx);
    if (str == NULL) {
      str = "";
    }
    uivlSetRowColumnStr (mqint->uivl, rownum, colidx, str);

    if (tagidx == TAG_FAVORITE) {
      songfav_t   *songfav;
      int         favidx;
      const char  *name;
      char        tbuff [80];

      songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
      favidx = songGetNum (song, TAG_FAVORITE);
      name = songFavoriteGetStr (songfav, favidx, SONGFAV_NAME);
      snprintf (tbuff, sizeof (tbuff), "label.%s", name);
      uivlSetRowColumnClass (mqint->uivl, rownum, colidx, name);
    }
  }
  nlistFree (tdlist);

  mqint->inchange = false;
}
