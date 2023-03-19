/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "dispsel.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "pathbld.h"
#include "ui.h"
#include "callback.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uimusicq.h"
#include "uisong.h"
#include "uitreedisp.h"

enum {
  MUSICQ_COL_ELLIPSIZE,
  MUSICQ_COL_FONT,
  MUSICQ_COL_UNIQUE_IDX,
  MUSICQ_COL_DBIDX,
  MUSICQ_COL_DISP_IDX_COLOR,
  MUSICQ_COL_DISP_IDX_COLOR_SET,
  MUSICQ_COL_DISP_IDX,
  MUSICQ_COL_PAUSEIND,
  MUSICQ_COL_MAX,
  MUSICQ_COL_DANCE,
  MUSICQ_COL_TITLE,
  MUSICQ_COL_ARTIST,
};

enum {
  MUSICQ_NEW_DISP,
  MUSICQ_UPD_DISP,
};

enum {
  MUSICQ_FORCE_LAST = 999,
};

enum {
  UIMUSICQ_CB_MOVE_TOP,
  UIMUSICQ_CB_MOVE_UP,
  UIMUSICQ_CB_MOVE_DOWN,
  UIMUSICQ_CB_TOGGLE_PAUSE,
  UIMUSICQ_CB_AUDIO_REMOVE,
  UIMUSICQ_CB_REQ_EXTERNAL,
  UIMUSICQ_CB_CLEAR_QUEUE,
  UIMUSICQ_CB_EDIT_LOCAL,
  UIMUSICQ_CB_PLAY,
  UIMUSICQ_CB_QUEUE,
  UIMUSICQ_CB_KEYB,
  UIMUSICQ_CB_MAX,
};

enum {
  UIMUSICQ_BUTTON_CLEAR_QUEUE,
  UIMUSICQ_BUTTON_EDIT,
  UIMUSICQ_BUTTON_MOVE_DOWN,
  UIMUSICQ_BUTTON_MOVE_TOP,
  UIMUSICQ_BUTTON_MOVE_UP,
  UIMUSICQ_BUTTON_PLAY,
  UIMUSICQ_BUTTON_QUEUE,
  UIMUSICQ_BUTTON_REMOVE,
  UIMUSICQ_BUTTON_TOGGLE_PAUSE,
  UIMUSICQ_BUTTON_MAX,
};

typedef struct uimusicqgtk {
  uidance_t         *uidance;
  uidance_t         *uidance5;
  uitree_t          *musicqTree;
  GtkTreeSelection  *sel;
  GtkTreeViewColumn *favColumn;
  callback_t        *callbacks [UIMUSICQ_CB_MAX];
  uibutton_t        *buttons [UIMUSICQ_BUTTON_MAX];
  int               *typelist;
  uikey_t           *uikey;
  int               colcount;        // for the display type callback
} uimusicqinternal_t;

static bool   uimusicqQueueDanceCallback (void *udata, long idx, int count);
static bool   uimusicqQueuePlaylistCallback (void *udata, long idx);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate, int newdispflag);
static void   uimusicqProcessMusicQueueDataNewCallback (int type, void *udata);
static void   uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate);
static void   uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, song_t *song, int ci);
static void   uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata);
static int    uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static bool   uimusicqPlayCallback (void *udata);
static bool   uimusicqQueueCallback (void *udata);
static dbidx_t uimusicqGetSelectionDbidx (uimusicq_t *uimusicq);
static void   uimusicqSelectionChgCallback (GtkTreeSelection *sel, gpointer udata);
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
static void   uimusicqCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path, GtkTreeViewColumn* column, gpointer udata);
static bool   uimusicqKeyEvent (void *udata);
static void   uimusicqMarkPreviousSelection (uimusicq_t *uimusicq, bool disp);
static void   uimusicqCopySelections (uimusicq_t *uimusicq, uimusicq_t *peer, int mqidx);

void
uimusicqUIInit (uimusicq_t *uimusicq)
{
  uimusicqinternal_t   *uiw;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiw = mdmalloc (sizeof (uimusicqinternal_t));
    uimusicq->ui [i].uiWidgets = uiw;
    uiw->uidance = NULL;
    uiw->uidance5 = NULL;
    uiw->musicqTree = NULL;
    uiw->favColumn = NULL;
    uiw->uikey = NULL;
    for (int j = 0; j < UIMUSICQ_BUTTON_MAX; ++j) {
      uiw->buttons [j] = NULL;
    }
    for (int j = 0; j < UIMUSICQ_CB_MAX; ++j) {
      uiw->callbacks [j] = NULL;
    }
  }
}

void
uimusicqUIFree (uimusicq_t *uimusicq)
{
  uimusicqinternal_t   *uiw;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiw = uimusicq->ui [i].uiWidgets;
    if (uiw != NULL) {
      for (int j = 0; j < UIMUSICQ_CB_MAX; ++j) {
        callbackFree (uiw->callbacks [j]);
      }
      for (int j = 0; j < UIMUSICQ_BUTTON_MAX; ++j) {
        uiButtonFree (uiw->buttons [j]);
      }
      uidanceFree (uiw->uidance);
      uidanceFree (uiw->uidance5);
      uiTreeViewFree (uiw->musicqTree);
      uiKeyFree (uiw->uikey);
      mdfree (uiw);
    }
  }
}

UIWidget *
uimusicqBuildUI (uimusicq_t *uimusicq, UIWidget *parentwin, int ci,
    UIWidget *statusMsg, uientryval_t validateFunc)
{
  int               saveci;
  char              tbuff [MAXPATHLEN];
  UIWidget          hbox;
  UIWidget          uiwidget;
  UIWidget          *uitreewidgetp;
  uibutton_t        *uibutton;
  UIWidget          *uiwidgetp;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  slist_t           *sellist;
  uimusicqinternal_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqBuildUI");

  saveci = uimusicq->musicqManageIdx;
  /* temporary */
  uimusicq->musicqManageIdx = ci;
  uiw = uimusicq->ui [ci].uiWidgets;
  uimusicq->statusMsg = statusMsg;

  uimusicq->ui [ci].hasui = true;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uimusicq->pausePixbuf, tbuff);
  uiImageGetPixbuf (&uimusicq->pausePixbuf);
  uiWidgetMakePersistent (&uimusicq->pausePixbuf);

  uiCreateVertBox (&uimusicq->ui [ci].mainbox);
  uiWidgetExpandHoriz (&uimusicq->ui [ci].mainbox);
  uiWidgetExpandVert (&uimusicq->ui [ci].mainbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, 1);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uimusicq->ui [ci].mainbox, &hbox);

  /* dispseltype can be a music queue, history, a song list or ez song list */

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiw->callbacks [UIMUSICQ_CB_MOVE_TOP] = callbackInit (
        uimusicqMoveTopCallback, uimusicq, "musicq: move-to-top");
    uibutton = uiCreateButton (
        uiw->callbacks [UIMUSICQ_CB_MOVE_TOP],
        /* CONTEXT: music queue: button: move the selected song to the top of the queue */
        _("Move to Top"), "button_movetop");
    uiw->buttons [UIMUSICQ_BUTTON_MOVE_TOP] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);

    uiw->callbacks [UIMUSICQ_CB_MOVE_UP] = callbackInit (
        uimusicqMoveUpCallback, uimusicq, "musicq: move-up");
    uibutton = uiCreateButton (uiw->callbacks [UIMUSICQ_CB_MOVE_UP],
        /* CONTEXT: music queue: button: move the selected song up in the queue */
        _("Move Up"), "button_up");
    uiw->buttons [UIMUSICQ_BUTTON_MOVE_UP] = uibutton;
    uiButtonSetRepeat (uibutton, UIMUSICQ_REPEAT_TIME);
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);

    uiw->callbacks [UIMUSICQ_CB_MOVE_DOWN] = callbackInit (
        uimusicqMoveDownCallback, uimusicq, "musicq: move-down");
    uibutton = uiCreateButton (uiw->callbacks [UIMUSICQ_CB_MOVE_DOWN],
        /* CONTEXT: music queue: button: move the selected song down in the queue */
        _("Move Down"), "button_down");
    uiw->buttons [UIMUSICQ_BUTTON_MOVE_DOWN] = uibutton;
    uiButtonSetRepeat (uibutton, UIMUSICQ_REPEAT_TIME);
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiw->callbacks [UIMUSICQ_CB_TOGGLE_PAUSE] = callbackInit (
        uimusicqTogglePauseCallback, uimusicq, "musicq: toggle-pause");
    uibutton = uiCreateButton (
        uiw->callbacks [UIMUSICQ_CB_TOGGLE_PAUSE],
        /* CONTEXT: music queue: button: set playback to pause after the selected song is played (toggle) */
        _("Toggle Pause"), "button_pause");
    uiw->buttons [UIMUSICQ_BUTTON_TOGGLE_PAUSE] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiw->callbacks [UIMUSICQ_CB_AUDIO_REMOVE] = callbackInit (
        uimusicqRemoveCallback, uimusicq, "musicq: remove-from-queue");
    uibutton = uiCreateButton (
        uiw->callbacks [UIMUSICQ_CB_AUDIO_REMOVE],
        /* CONTEXT: music queue: button: remove the song from the queue */
        _("Remove"), "button_audioremove");
    uiw->buttons [UIMUSICQ_BUTTON_REMOVE] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiWidgetSetMarginStart (uiwidgetp, 3);
    uiWidgetSetMarginEnd (uiwidgetp, 2);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiw->callbacks [UIMUSICQ_CB_CLEAR_QUEUE] = callbackInit (
        uimusicqTruncateQueueCallback, uimusicq, "musicq: clear-queue");
    uibutton = uiCreateButton (
        uiw->callbacks [UIMUSICQ_CB_CLEAR_QUEUE],
        /* CONTEXT: music queue: button: clear the queue */
        _("Clear Queue"), NULL);
    uiw->buttons [UIMUSICQ_BUTTON_CLEAR_QUEUE] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiWidgetSetMarginStart (uiwidgetp, 2);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiw->callbacks [UIMUSICQ_CB_EDIT_LOCAL] = callbackInit (
        uimusicqSongEditCallback, uimusicq, "musicq: edit");
    uibutton = uiCreateButton (uiw->callbacks [UIMUSICQ_CB_EDIT_LOCAL],
        /* CONTEXT: music queue: edit the selected song */
        _("Edit"), "button_edit");
    uiw->buttons [UIMUSICQ_BUTTON_EDIT] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);

    uiw->callbacks [UIMUSICQ_CB_PLAY] = callbackInit (
        uimusicqPlayCallback, uimusicq, "musicq: play");
    uibutton = uiCreateButton (uiw->callbacks [UIMUSICQ_CB_PLAY],
        /* CONTEXT: music queue: play the selected song */
        _("Play"), NULL);
    uiw->buttons [UIMUSICQ_BUTTON_PLAY] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_HISTORY) {
    uiw->callbacks [UIMUSICQ_CB_QUEUE] = callbackInit (
        uimusicqQueueCallback, uimusicq, "musicq: queue");
    uibutton = uiCreateButton (uiw->callbacks [UIMUSICQ_CB_QUEUE],
        /* CONTEXT: (verb) history: re-queue the selected song */
        _("Queue"), NULL);
    uiw->buttons [UIMUSICQ_BUTTON_QUEUE] = uibutton;
    uiwidgetp = uiButtonGetUIWidget (uibutton);
    uiBoxPackStart (&hbox, uiwidgetp);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    if (uimusicq->queueplcb == NULL) {
      uimusicq->queueplcb = callbackInitLong (
          uimusicqQueuePlaylistCallback, uimusicq);
    }
    uiwidgetp = uiDropDownCreate (parentwin,
        /* CONTEXT: (verb) music queue: button: queue a playlist for playback */
        _("Queue Playlist"), uimusicq->queueplcb,
        uimusicq->ui [ci].playlistsel, uimusicq);
    uiBoxPackEnd (&hbox, uiwidgetp);
    uimusicqCreatePlaylistList (uimusicq);

    if (bdjoptGetNumPerQueue (OPT_Q_SHOW_QUEUE_DANCE, ci)) {
      if (uimusicq->queuedancecb == NULL) {
        uimusicq->queuedancecb = callbackInitLongInt (
            uimusicqQueueDanceCallback, uimusicq);
      }
      uiw->uidance5 = uidanceDropDownCreate (&hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue 5 dances for playback */
          UIDANCE_NONE, _("Queue 5"), UIDANCE_PACK_END, 5);
      uidanceSetCallback (uiw->uidance5, uimusicq->queuedancecb);

      uiw->uidance = uidanceDropDownCreate (&hbox, parentwin,
          /* CONTEXT: (verb) music queue: button: queue a dance for playback */
          UIDANCE_NONE, _("Queue Dance"), UIDANCE_PACK_END, 1);
      uidanceSetCallback (uiw->uidance, uimusicq->queuedancecb);
    }
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uientry_t   *entryp;

    entryp = uimusicq->ui [ci].slname;
    uiEntryCreate (entryp);
    if (validateFunc != NULL) {
      uiEntrySetValidate (entryp, validateFunc, statusMsg, UIENTRY_IMMEDIATE);
    }
    uiWidgetSetClass (uiEntryGetUIWidget (entryp), ACCENT_CLASS);
    if (uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
      uiWidgetExpandHoriz (uiEntryGetUIWidget (entryp));
      uiWidgetAlignHorizFill (uiEntryGetUIWidget (entryp));
      uiBoxPackEndExpand (&hbox, uiEntryGetUIWidget (entryp));
    }
    if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST) {
      uiBoxPackEnd (&hbox, uiEntryGetUIWidget (entryp));
    }

    /* CONTEXT: music queue: label for song list name */
    uiCreateColonLabel (&uiwidget, _("Song List"));
    uiWidgetSetMarginStart (&uiwidget, 6);
    uiBoxPackEnd (&hbox, &uiwidget);
  }

  /* musicq tree view */

  uiCreateScrolledWindow (&uiwidget, 400);
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStartExpand (&uimusicq->ui [ci].mainbox, &uiwidget);

  uiw->musicqTree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);

  uiw->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uitreewidgetp->widget));

  uiTreeViewEnableHeaders (uiw->musicqTree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetExpandHoriz (uitreewidgetp);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&uiwidget, uitreewidgetp);

  g_signal_connect (uitreewidgetp->widget, "row-activated",
      G_CALLBACK (uimusicqCheckFavChgSignal), uimusicq);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  /* it appears that foreground-set must appear in the attributes */
  /* before foreground */
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MUSICQ_COL_DISP_IDX,
      "font", MUSICQ_COL_FONT,
      "foreground-set", MUSICQ_COL_DISP_IDX_COLOR_SET,
      "foreground", MUSICQ_COL_DISP_IDX_COLOR,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "pixbuf", MUSICQ_COL_PAUSEIND, NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uitreedispAddDisplayColumns (uiw->musicqTree, sellist,
      MUSICQ_COL_MAX, MUSICQ_COL_FONT, MUSICQ_COL_ELLIPSIZE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uitreewidgetp->widget), NULL);

  g_signal_connect ((GtkWidget *) uiw->sel, "changed",
      G_CALLBACK (uimusicqSelectionChgCallback), uimusicq);

  uimusicq->musicqManageIdx = saveci;

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiw->uikey = uiKeyAlloc ();
    uiw->callbacks [UIMUSICQ_CB_KEYB] = callbackInit (
        uimusicqKeyEvent, uimusicq, NULL);
    uiKeySetKeyCallback (uiw->uikey, uitreewidgetp,
        uiw->callbacks [UIMUSICQ_CB_KEYB]);
  }

  logProcEnd (LOG_PROC, "uimusicqBuildUI", "");
  return &uimusicq->ui [ci].mainbox;
}

void
uimusicqUIMainLoop (uimusicq_t *uimusicq)
{
  uimusicqinternal_t *uiw;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  uiButtonCheckRepeat (uiw->buttons [UIMUSICQ_BUTTON_MOVE_DOWN]);
  uiButtonCheckRepeat (uiw->buttons [UIMUSICQ_BUTTON_MOVE_UP]);
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
  uimusicqinternal_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");
  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "no-ui");
    return;
  }
  uiw = uimusicq->ui [mqidx].uiWidgets;

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "count != 1");
    return;
  }

  switch (which) {
    case UIMUSICQ_SEL_CURR: {
      break;
    }
    case UIMUSICQ_SEL_PREV: {
      uiTreeViewSelectPrevious (uiw->musicqTree);
      break;
    }
    case UIMUSICQ_SEL_NEXT: {
      uiTreeViewSelectNext (uiw->musicqTree);
      break;
    }
    case UIMUSICQ_SEL_TOP: {
      uiTreeViewSelectFirst (uiw->musicqTree);
      break;
    }
  }

  if (valid) {
    GtkTreePath *path = NULL;
    UIWidget    *uiwidgetp;
    GtkTreeModel      *model;
    GtkTreeIter       iter;

    uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
    gtk_tree_selection_select_iter (uiw->sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    mdextalloc (path);
    if (path != NULL) {
      uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
      if (uimusicq->ui [mqidx].count > 0 &&
          GTK_IS_TREE_VIEW (uiwidgetp->widget)) {
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiwidgetp->widget),
            path, NULL, FALSE, 0.0, 0.0);
      }
      mdextfree (path);
      gtk_tree_path_free (path);
    }
  }
  logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "");
}


void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate)
{
  int               ci;
  GtkTreeModel      *model = NULL;
  uimusicqinternal_t     *uiw = NULL;
  int               newdispflag;
  UIWidget          *uiwidgetp;

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
  uiw = uimusicq->ui [ci].uiWidgets;

  uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
  if (uiwidgetp->widget != NULL) {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  }
  newdispflag = MUSICQ_UPD_DISP;
  if (model == NULL) {
    newdispflag = MUSICQ_NEW_DISP;
  }
  uimusicqProcessMusicQueueDataUpdate (uimusicq, musicqupdate, newdispflag);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "");
}

void
uimusicqIterate (uimusicq_t *uimusicq, uimusicqiteratecb_t cb, musicqidx_t mqidx)
{
  GtkTreeModel  *model;
  uimusicqinternal_t *uiw;
  UIWidget      *uiwidgetp;


  uimusicq->iteratecb = cb;
  uiw = uimusicq->ui [mqidx].uiWidgets;
  uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  gtk_tree_model_foreach (model, uimusicqIterateCallback, uimusicq);
}

long
uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx)
{
  uimusicqinternal_t *uiw;
  long          loc;
  int           count = 0;
  char          *pathstr;

  uiw = uimusicq->ui [mqidx].uiWidgets;

  loc = MUSICQ_FORCE_LAST;
  if (uiw->musicqTree == NULL) {
    return loc;
  }

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
  if (count == 1) {
    GtkTreeModel  *model;
    GtkTreeIter   iter;
    GtkTreePath   *path;
    int           valid;

    valid = gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
    if (valid) {
      path = gtk_tree_model_get_path (model, &iter);
      mdextalloc (path);
      if (path != NULL) {
        pathstr = gtk_tree_path_to_string (path);
        mdextalloc (pathstr);
        loc = atol (pathstr);
        mdextfree (path);
        gtk_tree_path_free (path);
        mdfree (pathstr);       // allocated by gtk
      }
    }
  }

  return loc;
}

void
uimusicqSetSelectLocation (uimusicq_t *uimusicq, int mqidx, long loc)
{
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

  if (uimusicq->clearqueuecb != NULL) {
    callbackHandler (uimusicq->clearqueuecb);
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
  uimusicqinternal_t *uiw;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  /* if the player is active, disable the button */
  if (active) {
    uiButtonSetState (uiw->buttons [UIMUSICQ_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiButtonSetState (uiw->buttons [UIMUSICQ_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
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
uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq,
    mp_musicqupdate_t *musicqupdate, int newdispflag)
{
  slist_t           *sellist;
  uimusicqinternal_t     *uiw;
  int               ci;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate");

  ci = musicqupdate->mqidx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (newdispflag == MUSICQ_NEW_DISP) {
    uiw->typelist = mdmalloc (sizeof (int) * MUSICQ_COL_MAX);
    uiw->colcount = 0;
    /* attributes: ellipsize / font */
    uiw->typelist [uiw->colcount++] = TREE_TYPE_ELLIPSIZE;
    uiw->typelist [uiw->colcount++] = TREE_TYPE_STRING;
    /* unique idx / dbidx */
    uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
    uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
    /* disp idx color / disp idx color set / display disp idx / pause ind */
    uiw->typelist [uiw->colcount++] = TREE_TYPE_STRING;
    uiw->typelist [uiw->colcount++] = TREE_TYPE_BOOLEAN;
    uiw->typelist [uiw->colcount++] = TREE_TYPE_NUM;
    uiw->typelist [uiw->colcount++] = TREE_TYPE_IMAGE;
    if (uiw->colcount != MUSICQ_COL_MAX) {
      fprintf (stderr, "ERR: mismatched MUSICQ_COL_MAX %d\n", uiw->colcount);
    }

    sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
    uimusicq->cbci = ci;
    uisongAddDisplayTypes (sellist,
        uimusicqProcessMusicQueueDataNewCallback, uimusicq);

    uiTreeViewCreateValueStoreFromList (uiw->musicqTree, uiw->colcount, uiw->typelist);
    mdfree (uiw->typelist);
  }

  uimusicqProcessMusicQueueDisplay (uimusicq, musicqupdate);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "");
}

static void
uimusicqProcessMusicQueueDataNewCallback (int type, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqinternal_t *uiw;

  uiw = uimusicq->ui [uimusicq->cbci].uiWidgets;
  uiw->typelist = mdrealloc (uiw->typelist, sizeof (int) * (uiw->colcount + 1));
  uiw->typelist [uiw->colcount] = type;
  ++uiw->colcount;
}

static void
uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq,
    mp_musicqupdate_t *musicqupdate)
{
  GtkTreeModel  *model = NULL;
  GtkTreeIter   iter;
  bool          valid;
  const char    *listingFont;
  mp_musicqupditem_t *musicqupditem;
  nlistidx_t    iteridx;
  uimusicqinternal_t *uiw;
  int           ci;
  UIWidget      *uiwidgetp;
  int           row;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDisplay");

  ci = musicqupdate->mqidx;
  uiw = uimusicq->ui [ci].uiWidgets;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
  if (uiwidgetp->widget != NULL) {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  }
  if (model == NULL) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDisplay", "null-model");
    return;
  }

  /* gtk selections will change internally quite a bit, bypass any change */
  /* signals for the moment */
  uimusicq->ui [ci].selchgbypass = true;

  valid = gtk_tree_model_get_iter_first (model, &iter);

  uiTreeViewSelectSave (uiw->musicqTree);

  row = 0;
  uimusicq->ui [ci].count = nlistGetCount (musicqupdate->dispList);
  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    song_t        *song;
    GdkPixbuf     *pixbuf;

    pixbuf = NULL;
    if (musicqupditem->pauseind) {
      pixbuf = uimusicq->pausePixbuf.pixbuf;
    }

    song = dbGetByIdx (uimusicq->musicdb, musicqupditem->dbidx);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (! valid) {
      uiTreeViewValueAppend (uiw->musicqTree);
      uiTreeViewSetValueEllipsize (uiw->musicqTree, MUSICQ_COL_ELLIPSIZE);
      uiTreeViewSetValues (uiw->musicqTree,
          MUSICQ_COL_FONT, listingFont,
          MUSICQ_COL_DISP_IDX_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
          MUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) FALSE,
          MUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          TREE_VALUE_END);
#if 0
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_ELLIPSIZE, (treeint_t) TREE_ELLIPSIZE_END,
          MUSICQ_COL_FONT, (treeint_t) listingFont,
          MUSICQ_COL_DISP_IDX_COLOR, bdjoptGetStr (OPT_P_UI_ACCENT_COL),
          MUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) FALSE,
          MUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
#endif
    } else {
      uiTreeViewSelectSet (uiw->musicqTree, row);
      /* all data must be updated, except the font and ellipsize */
      uiTreeViewSetValues (uiw->musicqTree,
          MUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) FALSE,
          MUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          TREE_VALUE_END);
#if 0
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) FALSE,
          MUSICQ_COL_DISP_IDX, (treenum_t) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (treenum_t) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (treenum_t) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
#endif
    }
    uimusicqSetMusicqDisplay (uimusicq, song, ci);

    ++row;
  }

  /* remove any other rows past the end of the display */
//  while (valid) {
//    valid = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
//  }

  /* now the selection change signal can be processed */
  uimusicq->ui [ci].selchgbypass = false;

  uiTreeViewSelectRestore (uiw->musicqTree);

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
  uimusicqinternal_t *uiw;
  slist_t       *sellist;

  uiw = uimusicq->ui [ci].uiWidgets;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uimusicq->cbci = ci;
  uisongSetDisplayColumns (sellist, song, MUSICQ_COL_MAX,
      uimusicqSetMusicqDisplayCallback, uimusicq);
}

static void
uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqinternal_t *uiw;
  int           ci;

  ci = uimusicq->cbci;
  uiw = uimusicq->ui [ci].uiWidgets;

  uitreedispSetDisplayColumn (uiw->musicqTree, col, num, str);
}

static int
uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uimusicq_t  *uimusicq = udata;
  glong       dbidx;

  gtk_tree_model_get (model, iter, MUSICQ_COL_DBIDX, &dbidx, -1);
  uimusicq->iteratecb (uimusicq, dbidx);
  return FALSE;
}

/* used by song list editor */
static bool
uimusicqPlayCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqinternal_t *uiw;
  musicqidx_t   ci;
  dbidx_t       dbidx;
  int           count;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
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
  uimusicqinternal_t *uiw;
  musicqidx_t   ci;
  dbidx_t       dbidx;
  int           count;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
  if (count != 1) {
    return UICB_CONT;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);

  if (uimusicq->queuecb != NULL) {
    callbackHandlerLongInt (uimusicq->queuecb, dbidx, MUSICQ_LAST);
  }
  return UICB_CONT;
}

static dbidx_t
uimusicqGetSelectionDbidx (uimusicq_t *uimusicq)
{
  int             ci;
  uimusicqinternal_t   *uiw;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  glong           dbidx = -1;
  int             count;
  int             valid;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (uiw == NULL || uiw->sel == NULL) {
    return -1;
  }

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
  if (count != 1) {
    return -1;
  }

  valid = gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
  if (valid) {
    gtk_tree_model_get (model, &iter, MUSICQ_COL_DBIDX, &dbidx, -1);
  }
  return dbidx;
}

static void
uimusicqSelectionChgCallback (GtkTreeSelection *sel, gpointer udata)
{
  uimusicq_t      *uimusicq = udata;

  uimusicqSelectionChgProcess (uimusicq);
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
      uimusicq->newselcb != NULL) {
    callbackHandlerLong (uimusicq->newselcb, dbidx);
  }

  if (loc != MUSICQ_FORCE_LAST) {
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
    uimusicqMarkPreviousSelection (uimusicq, FALSE);
    uimusicq->ui [ci].prevSelection = uimusicq->ui [ci].currSelection;
    uimusicq->ui [ci].currSelection = loc;
  }
  uimusicqMarkPreviousSelection (uimusicq, TRUE);

  for (int i = 0; i < uimusicq->peercount; ++i) {
    if (uimusicq->peers [i] == NULL) {
      continue;
    }
    uimusicqSetPeerFlag (uimusicq->peers [i], true);
    if (! sameprev) {
      uimusicqCopySelections (uimusicq, uimusicq->peers [i], ci);
    }
    uimusicqMarkPreviousSelection (uimusicq->peers [i], TRUE);
    uimusicqSetPeerFlag (uimusicq->peers [i], false);
  }
}

static void
uimusicqSetDefaultSelection (uimusicq_t *uimusicq)
{
  uimusicqinternal_t  *uiw;
  int             ci;
  int             count;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (uiw->sel == NULL) {
    return;
  }

  count = uiTreeViewSelectGetCount (uiw->musicqTree);
  if (uimusicq->ui [ci].count > 0 && count < 1) {
    GtkTreeModel  *model;
    GtkTreeIter   iter;
    int           valid;
    UIWidget      *uiwidgetp;

    uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
    valid = gtk_tree_model_get_iter_first (model, &iter);
    if (valid) {
      gtk_tree_selection_select_iter (uiw->sel, &iter);
    }
  }

  return;
}

static void
uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx)
{
  GtkTreePath   *path;
  uimusicqinternal_t *uiw;
  char          tbuff [40];
  UIWidget      *uiwidgetp;

  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "no-ui");
    return;
  }
  uiw = uimusicq->ui [mqidx].uiWidgets;

  if (uimusicq->ui [mqidx].selectLocation == MUSICQ_FORCE_LAST) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "select-999");
    return;
  }

  uiTreeViewSelectSet (uiw->musicqTree, uimusicq->ui [mqidx].selectLocation);
  snprintf (tbuff, sizeof (tbuff), "%ld", uimusicq->ui [mqidx].selectLocation);
  path = gtk_tree_path_new_from_string (tbuff);
  mdextalloc (path);
  uiwidgetp = uiTreeViewGetUIWidget (uiw->musicqTree);
  if (path != NULL && GTK_IS_TREE_VIEW (uiwidgetp->widget)) {
    uimusicqMusicQueueSetSelected (uimusicq, mqidx, UIMUSICQ_SEL_CURR);
    if (uimusicq->ui [mqidx].count > 0 &&
        GTK_IS_TREE_VIEW (uiwidgetp->widget)) {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiwidgetp->widget),
          path, NULL, FALSE, 0.0, 0.0);
    }
  }
  if (path != NULL) {
    mdextfree (path);
    gtk_tree_path_free (path);
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


  if (uimusicq->newselcb != NULL) {
    dbidx = uimusicqGetSelectionDbidx (uimusicq);
    if (dbidx < 0) {
      return UICB_CONT;
    }
    callbackHandlerLong (uimusicq->newselcb, dbidx);
  }
  return callbackHandler (uimusicq->editcb);
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

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
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

static void
uimusicqCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uimusicq_t    * uimusicq = udata;
  uimusicqinternal_t * uiw;
  int           ci;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin (LOG_PROC, "uimusicqCheckFavChgSignal");

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;
  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uimusicqCheckFavChgSignal", "not-fav-col");
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: %s: change favorite", uimusicq->tag);
  dbidx = uimusicqGetSelectionDbidx (uimusicq);
  song = dbGetByIdx (uimusicq->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uimusicq->songsavecb != NULL) {
      callbackHandlerLong (uimusicq->songsavecb, dbidx);
    }
  }
}

static bool
uimusicqKeyEvent (void *udata)
{
  uimusicq_t      *uimusicq = udata;
  uimusicqinternal_t   *uiw;
  int             ci;

  if (uimusicq == NULL) {
    return UICB_CONT;
  }

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (uiKeyIsPressEvent (uiw->uikey)) {
    if (uiKeyIsAudioPlayKey (uiw->uikey)) {
      uimusicqPlayCallback (uimusicq);
    }
    if (uiKeyIsKey (uiw->uikey, 'S')) {
      uimusicqSwap (uimusicq, ci);
    }
  }

  if (uiKeyIsControlPressed (uiw->uikey) &&
      uiKeyIsMovementKey (uiw->uikey)) {
    if (uiKeyIsPressEvent (uiw->uikey)) {
      if (uiKeyIsUpKey (uiw->uikey)) {
        uimusicqMoveUpCallback (uimusicq);
      }
      if (uiKeyIsDownKey (uiw->uikey)) {
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
  uimusicqinternal_t *uiw;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (! uimusicq->ui [ci].hasui) {
    return;
  }
  if (uimusicq->ui [ci].prevSelection < 0) {
    return;
  }
  if (uimusicq->ui [ci].prevSelection >= uimusicq->ui [ci].count) {
    return;
  }

  uiTreeViewValueIteratorSet (uiw->musicqTree, uimusicq->ui [ci].prevSelection);
  uiTreeViewSetValues (uiw->musicqTree,
      MUSICQ_COL_DISP_IDX_COLOR_SET, (treebool_t) disp,
      TREE_VALUE_END);
  uiTreeViewValueIteratorClear (uiw->musicqTree);
}

static void
uimusicqCopySelections (uimusicq_t *uimusicq, uimusicq_t *peer, int mqidx)
{
  uimusicqMarkPreviousSelection (peer, FALSE);
  peer->ui [mqidx].prevSelection = uimusicq->ui [mqidx].prevSelection;
  peer->ui [mqidx].currSelection = uimusicq->ui [mqidx].currSelection;
}

