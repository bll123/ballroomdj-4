#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "dispsel.h"
#include "log.h"
#include "msgparse.h"
#include "musicdb.h"
#include "pathbld.h"
#include "uimusicq.h"
#include "ui.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uisong.h"

enum {
  MUSICQ_COL_ELLIPSIZE,
  MUSICQ_COL_FONT,
  MUSICQ_COL_UNIQUE_IDX,
  MUSICQ_COL_DBIDX,
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

static bool   uimusicqQueueDanceCallback (void *udata, long idx);
static bool   uimusicqQueuePlaylistCallback (void *udata, long idx);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate, int newdispflag);
static void   uimusicqProcessMusicQueueDataNewCallback (int type, void *udata);
static void   uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate);
static void   uimusicqSetMusicqDisplay (uimusicq_t *uimusicq,
    GtkTreeModel *model, GtkTreeIter *iter, song_t *song, int ci);
static void   uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata);
static int    uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static bool   uimusicqPlayCallback (void *udata);
static dbidx_t uimusicqGetSelectionDbidx (uimusicq_t *uimusicq);
static void   uimusicqSelectionChgCallback (GtkTreeSelection *sel, gpointer udata);
static void   uimusicqSetDefaultSelection (uimusicq_t *uimusicq);
static void   uimusicqSetSelection (uimusicq_t *uimusicq, int mqidx);
static bool   uimusicqSongEditCallback (void *udata);
static bool   uimusicqMoveTopCallback (void *udata);
static bool   uimusicqMoveUpCallback (void *udata);
static bool   uimusicqMoveDownCallback (void *udata);
static bool   uimusicqTogglePauseCallback (void *udata);
static bool   uimusicqRemoveCallback (void *udata);
static void   uimusicqCheckFavChgSignal (GtkTreeView* tv, GtkTreePath* path, GtkTreeViewColumn* column, gpointer udata);

enum {
  UIMUSICQ_CB_MOVE_TOP,
  UIMUSICQ_CB_MOVE_UP_PRESS,
  UIMUSICQ_CB_MOVE_UP_RELEASE,
  UIMUSICQ_CB_MOVE_DOWN_PRESS,
  UIMUSICQ_CB_MOVE_DOWN_RELEASE,
  UIMUSICQ_CB_TOGGLE_PAUSE,
  UIMUSICQ_CB_AUDIO_REMOVE,
  UIMUSICQ_CB_REQ_EXTERNAL,
  UIMUSICQ_CB_CLEAR_QUEUE,
  UIMUSICQ_CB_EDIT_LOCAL,
  UIMUSICQ_CB_PLAY,
  UIMUSICQ_CB_MAX,
};

typedef struct uimusicqgtk {
  uidance_t     *uidance;
  GtkWidget     *musicqTree;
  GtkTreeSelection  *sel;
  GtkTreeViewColumn   *favColumn;
  char          *selPathStr;
  UICallback    callback [UIMUSICQ_CB_MAX];
  UIWidget      playbutton;
  GtkTreeModel  *model;
  GtkTreeIter   *iter;
  GType         *typelist;
  int           col;        // for the display type callback
} uimusicqgtk_t;

void
uimusicqUIInit (uimusicq_t *uimusicq)
{
  uimusicqgtk_t   *uiw;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiw = malloc (sizeof (uimusicqgtk_t));
    uimusicq->ui [i].uiWidgets = uiw;
    uiw->uidance = NULL;
    uiw->selPathStr = NULL;
    uiw->musicqTree = NULL;
    uiw->favColumn = NULL;
    uiutilsUIWidgetInit (&uiw->playbutton);
  }
}

void
uimusicqUIFree (uimusicq_t *uimusicq)
{
  uimusicqgtk_t   *uiw;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiw = uimusicq->ui [i].uiWidgets;
    if (uiw->selPathStr != NULL) {
      free (uiw->selPathStr);
    }
    uidanceFree (uiw->uidance);
    if (uiw != NULL) {
      free (uiw);
    }
  }
}

UIWidget *
uimusicqBuildUI (uimusicq_t *uimusicq, UIWidget *parentwin, int ci,
    UIWidget *statusMsg, uientryval_t validateFunc)
{
  int               tci;
  char              tbuff [MAXPATHLEN];
  UIWidget          hbox;
  UIWidget          uiwidget;
  UIWidget          *uiwidgetp;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  slist_t           *sellist;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqBuildUI");

  /* temporary */
  tci = uimusicq->musicqManageIdx;
  uimusicq->musicqManageIdx = ci;
  uiw = uimusicq->ui [ci].uiWidgets;
  uimusicq->statusMsg = statusMsg;

  uimusicq->ui [ci].hasui = true;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uimusicq->pausePixbuf, tbuff);
  uiImageGetPixbuf (&uimusicq->pausePixbuf);
  uiWidgetMakePersistent (&uimusicq->pausePixbuf);

  uiCreateVertBox (&uimusicq->ui [ci].mainbox);
  uiWidgetExpandHoriz (&uimusicq->ui [ci].mainbox);
  uiWidgetExpandVert (&uimusicq->ui [ci].mainbox);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginTop (&hbox, uiBaseMarginSz);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uimusicq->ui [ci].mainbox, &hbox);

  /* dispseltype can be a music queue, history, a song list or ez song list */

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_MOVE_TOP],
        uimusicqMoveTopCallback, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_MOVE_TOP],
        /* CONTEXT: music queue: button: move the selected song to the top of the queue */
        _("Move to Top"), "button_movetop");
    uiBoxPackStart (&hbox, &uiwidget);

    /* CONTEXT: music queue: button: move the selected song up in the queue */
    uiCreateButton (&uiwidget, NULL, _("Move Up"), "button_up");
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_MOVE_UP_PRESS],
        uimusicqMoveUpCallback, uimusicq);
    uiButtonSetPressCallback (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_MOVE_UP_PRESS]);
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_MOVE_UP_RELEASE],
        uimusicqStopRepeat, uimusicq);
    uiButtonSetReleaseCallback (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_MOVE_UP_RELEASE]);
    uiBoxPackStart (&hbox, &uiwidget);

    /* CONTEXT: music queue: button: move the selected song down in the queue */
    uiCreateButton (&uiwidget, NULL, _("Move Down"), "button_down");
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_PRESS],
        uimusicqMoveDownCallback, uimusicq);
    uiButtonSetPressCallback (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_PRESS]);
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_RELEASE],
        uimusicqStopRepeat, uimusicq);
    uiButtonSetReleaseCallback (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_RELEASE]);
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_TOGGLE_PAUSE],
        uimusicqTogglePauseCallback, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_TOGGLE_PAUSE],
        /* CONTEXT: music queue: button: set playback to pause after the selected song is played (toggle) */
        _("Toggle Pause"), "button_pause");
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ ||
      uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_AUDIO_REMOVE],
        uimusicqRemoveCallback, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_AUDIO_REMOVE],
        /* CONTEXT: music queue: button: remove the song from the queue */
        _("Remove"), "button_audioremove");
    uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 3);
    uiWidgetSetMarginEnd (&uiwidget, uiBaseMarginSz * 2);
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uiutilsUICallbackInit (&uiw->callback [UIMUSICQ_CB_EDIT_LOCAL],
        uimusicqSongEditCallback, uimusicq);
    uiCreateButton (&uiwidget, &uiw->callback [UIMUSICQ_CB_EDIT_LOCAL],
        /* CONTEXT: music queue: edit the selected song */
        _("Edit"), "button_edit");
    uiBoxPackStart (&hbox, &uiwidget);

    uiutilsUICallbackInit (&uiw->callback [UIMUSICQ_CB_PLAY],
        uimusicqPlayCallback, uimusicq);
    uiCreateButton (&uiwidget, &uiw->callback [UIMUSICQ_CB_PLAY],
        /* CONTEXT: music queue: play the selected song */
        _("Play"), NULL);
    uiBoxPackStart (&hbox, &uiwidget);
    uiutilsUIWidgetCopy (&uiw->playbutton, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_HISTORY) {
// ### TODO change this to a queue callback
    uiutilsUICallbackInit (&uiw->callback [UIMUSICQ_CB_PLAY],
        uimusicqPlayCallback, uimusicq);
    uiCreateButton (&uiwidget, &uiw->callback [UIMUSICQ_CB_PLAY],
        /* CONTEXT: history: re-queue the selected song */
        _("Queue"), NULL);
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_REQ_EXTERNAL],
        NULL, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_REQ_EXTERNAL],
        /* CONTEXT: music queue: button: request playback of a song external to BDJ4 (not in the database) */
        _("Request External"), NULL);
    uiWidgetDisable (&uiwidget);
    uiBoxPackEnd (&hbox, &uiwidget);
// ### TODO create code to handle the request external button

    uiutilsUICallbackLongInit (&uimusicq->queueplcb,
        uimusicqQueuePlaylistCallback, uimusicq);
    uiwidgetp = uiDropDownCreate (parentwin,
        /* CONTEXT: music queue: button: queue a playlist for playback */
        _("Queue Playlist"), &uimusicq->queueplcb,
        uimusicq->ui [ci].playlistsel, uimusicq);
    uiBoxPackEnd (&hbox, uiwidgetp);
    uimusicqCreatePlaylistList (uimusicq);

    uiutilsUICallbackLongInit (&uimusicq->queuedancecb,
        uimusicqQueueDanceCallback, uimusicq);
    uiw->uidance = uidanceDropDownCreate (&hbox, parentwin,
        /* CONTEXT: music queue: button: queue a dance for playback */
        UIDANCE_NONE, _("Queue Dance"), UIDANCE_PACK_END);
    uidanceSetCallback (uiw->uidance, &uimusicq->queuedancecb);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_SONGLIST ||
      uimusicq->ui [ci].dispselType == DISP_SEL_EZSONGLIST) {
    uientry_t   *entryp;

    entryp = uimusicq->ui [ci].slname;
    uiEntryCreate (entryp);
    if (validateFunc != NULL) {
      uiEntrySetValidate (entryp, validateFunc, statusMsg, false);
    }
    uiEntrySetColor (entryp, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
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
    uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 6);
    uiBoxPackEnd (&hbox, &uiwidget);
  }

  if (uimusicq->ui [ci].dispselType == DISP_SEL_MUSICQ) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_CLEAR_QUEUE],
        uimusicqTruncateQueueCallback, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_CLEAR_QUEUE],
        /* CONTEXT: music queue: button: clear the queue */
        _("Clear Queue"), NULL);
    uiBoxPackEnd (&hbox, &uiwidget);
  }

  /* musicq tree view */

  uiCreateScrolledWindow (&uiwidget, 400);
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStartExpand (&uimusicq->ui [ci].mainbox, &uiwidget);

  uiw->musicqTree = uiCreateTreeView ();
  assert (uiw->musicqTree != NULL);

  uiw->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uiw->musicqTree));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiw->musicqTree), TRUE);
  uiWidgetAlignHorizFillW (uiw->musicqTree);
  uiWidgetExpandHorizW (uiw->musicqTree);
  uiWidgetExpandVertW (uiw->musicqTree);
  uiBoxPackInWindowUW (&uiwidget, uiw->musicqTree);
  g_signal_connect (uiw->musicqTree, "row-activated",
      G_CALLBACK (uimusicqCheckFavChgSignal), uimusicq);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MUSICQ_COL_DISP_IDX,
      "font", MUSICQ_COL_FONT,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiw->musicqTree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "pixbuf", MUSICQ_COL_PAUSEIND,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiw->musicqTree), column);

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uiw->favColumn = uiAddDisplayColumns (uiw->musicqTree, sellist,
      MUSICQ_COL_MAX, MUSICQ_COL_FONT, MUSICQ_COL_ELLIPSIZE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->musicqTree), NULL);

  uimusicq->musicqManageIdx = tci;

  g_signal_connect ((GtkWidget *) uiw->sel, "changed",
      G_CALLBACK (uimusicqSelectionChgCallback), uimusicq);

  logProcEnd (LOG_PROC, "uimusicqBuildUI", "");
  return &uimusicq->ui [ci].mainbox;
}

void
uimusicqUIMainLoop (uimusicq_t *uimusicq)
{
  if (uimusicq->repeatButton != UIMUSICQ_REPEAT_NONE) {
    if (mstimeCheck (&uimusicq->repeatTimer)) {
      if (uimusicq->repeatButton == UIMUSICQ_REPEAT_UP) {
        uimusicqMoveUpCallback (uimusicq);
      }
      if (uimusicq->repeatButton == UIMUSICQ_REPEAT_DOWN) {
        uimusicqMoveDownCallback (uimusicq);
      }
    }
  }

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
  GtkTreeModel      *model;
  gboolean          valid = false;
  GtkTreeIter       iter;
  int               count;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");
  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "no-ui");
    return;
  }
  uiw = uimusicq->ui [mqidx].uiWidgets;

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (uiw->sel, &model, &iter);

  switch (which) {
    case UIMUSICQ_SEL_CURR: {
      valid = true;
      break;
    }
    case UIMUSICQ_SEL_PREV: {
      valid = gtk_tree_model_iter_previous (model, &iter);
      break;
    }
    case UIMUSICQ_SEL_NEXT: {
      valid = gtk_tree_model_iter_next (model, &iter);
      break;
    }
    case UIMUSICQ_SEL_TOP: {
      valid = gtk_tree_model_get_iter_first (model, &iter);
      break;
    }
  }

  if (valid) {
    GtkTreePath *path = NULL;

    gtk_tree_selection_select_iter (uiw->sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    if (path != NULL) {
      uiw->selPathStr = gtk_tree_path_to_string (path);

      if (uimusicq->ui [mqidx].count > 0 &&
          GTK_IS_TREE_VIEW (uiw->musicqTree)) {
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiw->musicqTree),
            path, NULL, FALSE, 0.0, 0.0);
      }
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
  uimusicqgtk_t     *uiw = NULL;
  int               newdispflag;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueData");

  ci = musicqupdate->mqidx;
  if (! uimusicq->ui [ci].hasui) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "no-ui");
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));
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
  uimusicqgtk_t *uiw;

  uimusicq->iteratecb = cb;
  uiw = uimusicq->ui [mqidx].uiWidgets;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));
  gtk_tree_model_foreach (model, uimusicqIterateCallback, uimusicq);
}

long
uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx)
{
  uimusicqgtk_t *uiw;
  long          loc;
  int           count;
  char          *pathstr;

  uiw = uimusicq->ui [mqidx].uiWidgets;

  loc = 99;

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count == 1) {
    GtkTreeModel  *model;
    GtkTreeIter   iter;
    GtkTreePath   *path;

    gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    if (path != NULL) {
      pathstr = gtk_tree_path_to_string (path);
      loc = atol (pathstr);
      gtk_tree_path_free (path);
      free (pathstr);
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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: clear queue");

  ci = uimusicq->musicqManageIdx;

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
  uimusicqgtk_t *uiw;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetDisable (&uiw->playbutton);
  } else {
    uiWidgetEnable (&uiw->playbutton);
  }
}

/* internal routines */

static bool
uimusicqQueueDanceCallback (void *udata, long idx)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqQueueDanceProcess (uimusicq, idx);
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
  GtkListStore      *store = NULL;
  slist_t           *sellist;
  uimusicqgtk_t     *uiw;
  int               ci;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate");

  ci = musicqupdate->mqidx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (newdispflag == MUSICQ_NEW_DISP) {
    uiw->typelist = malloc (sizeof (GType) * MUSICQ_COL_MAX);
    uiw->col = 0;
    /* attributes: ellipsize / align / font */
    uiw->typelist [uiw->col++] = G_TYPE_INT;
    uiw->typelist [uiw->col++] = G_TYPE_STRING;
    /* unique idx / dbidx */
    uiw->typelist [uiw->col++] = G_TYPE_LONG;
    uiw->typelist [uiw->col++] = G_TYPE_LONG;
    /* display disp idx / pause ind */
    uiw->typelist [uiw->col++] = G_TYPE_LONG;
    uiw->typelist [uiw->col++] = GDK_TYPE_PIXBUF;

    sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
    uimusicq->cbci = ci;
    uisongAddDisplayTypes (sellist,
        uimusicqProcessMusicQueueDataNewCallback, uimusicq);

    store = gtk_list_store_newv (uiw->col, uiw->typelist);
    free (uiw->typelist);
    gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->musicqTree), GTK_TREE_MODEL (store));
    g_object_unref (G_OBJECT (store));
  }

  uimusicqProcessMusicQueueDisplay (uimusicq, musicqupdate);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "");
}

static void
uimusicqProcessMusicQueueDataNewCallback (int type, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqgtk_t *uiw;

  uiw = uimusicq->ui [uimusicq->cbci].uiWidgets;
  uiw->typelist = uiTreeViewAddDisplayType (uiw->typelist, type, uiw->col);
  ++uiw->col;
}

static void
uimusicqProcessMusicQueueDisplay (uimusicq_t *uimusicq,
    mp_musicqupdate_t *musicqupdate)
{
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  bool          valid;
  const char    *listingFont;
  mp_musicqupditem_t *musicqupditem;
  nlistidx_t    iteridx;
  uimusicqgtk_t *uiw;
  int           ci;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDisplay");

  ci = musicqupdate->mqidx;
  uiw = uimusicq->ui [ci].uiWidgets;
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));
  if (model == NULL) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDisplay", "null-model");
    return;
  }

  valid = gtk_tree_model_get_iter_first (model, &iter);

  uimusicq->ui [ci].count = nlistGetCount (musicqupdate->dispList);
  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    song_t        *song;
    GdkPixbuf     *pixbuf;

    pixbuf = NULL;
    if (musicqupditem->pflag) {
      pixbuf = uimusicq->pausePixbuf.pixbuf;
    }

    song = dbGetByIdx (uimusicq->musicdb, musicqupditem->dbidx);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (! valid) {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
          MUSICQ_COL_FONT, listingFont,
          MUSICQ_COL_DISP_IDX, (glong) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (glong) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (glong) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
    } else {
      /* all data must be updated */
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_DISP_IDX, (glong) musicqupditem->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (glong) musicqupditem->uniqueidx,
          MUSICQ_COL_DBIDX, (glong) musicqupditem->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
    }
    uimusicqSetMusicqDisplay (uimusicq, model, &iter, song, ci);

    if (valid) {
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }

  /* remove any other rows past the end of the display */
  while (valid) {
    valid = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  }

  if (uimusicq->ui [ci].haveselloc) {
    uimusicqSetSelection (uimusicq, ci);
    uimusicq->ui [ci].haveselloc = false;
  }

  uimusicqSetDefaultSelection (uimusicq);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDisplay", "");
}


static void
uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, GtkTreeModel *model,
    GtkTreeIter *iter, song_t *song, int ci)
{
  uimusicqgtk_t *uiw;
  slist_t       *sellist;

  uiw = uimusicq->ui [ci].uiWidgets;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->ui [ci].dispselType);
  uimusicq->cbci = ci;
  uiw->model = model;
  uiw->iter = iter;
  uisongSetDisplayColumns (sellist, song, MUSICQ_COL_MAX,
      uimusicqSetMusicqDisplayCallback, uimusicq);
}

static void
uimusicqSetMusicqDisplayCallback (int col, long num, const char *str, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqgtk_t *uiw;
  int           ci;

  ci = uimusicq->cbci;
  uiw = uimusicq->ui [ci].uiWidgets;

  uiTreeViewSetDisplayColumn (uiw->model, uiw->iter, col, num, str);
}

static int
uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uimusicq_t  *uimusicq = udata;
  long        dbidx;

  gtk_tree_model_get (model, iter, MUSICQ_COL_DBIDX, &dbidx, -1);
  uimusicq->iteratecb (uimusicq, dbidx);
  return FALSE;
}

/* used by song list editor */
static bool
uimusicqPlayCallback (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  uimusicqgtk_t *uiw;
  musicqidx_t   ci;
  dbidx_t       dbidx;
  int           count;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: play");
  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    return UICB_CONT;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);

  /* queue to the hidden music queue */
  uimusicqPlay (uimusicq, MUSICQ_MNG_PB, dbidx);
  return UICB_CONT;
}

static dbidx_t
uimusicqGetSelectionDbidx (uimusicq_t *uimusicq)
{
  int             ci;
  uimusicqgtk_t   *uiw;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  dbidx_t         dbidx;
  int             count;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    return -1;
  }

  /* get the selection from the song list (queue A) */
  gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
  gtk_tree_model_get (model, &iter, MUSICQ_COL_DBIDX, &dbidx, -1);
  return dbidx;
}

static void
uimusicqSelectionChgCallback (GtkTreeSelection *sel, gpointer udata)
{
  uimusicq_t      *uimusicq = udata;
  dbidx_t         dbidx;
  int             ci;
  long            idx;

  ci = uimusicq->musicqManageIdx;
  dbidx = uimusicqGetSelectionDbidx (uimusicq);
  if (dbidx >= 0 &&
      uimusicq->newselcb != NULL) {
    uiutilsCallbackLongHandler (uimusicq->newselcb, dbidx);
  }

  if (uimusicq->ispeercall) {
    return;
  }

  idx = uimusicqGetSelectLocation (uimusicq, ci);

  for (int i = 0; i < uimusicq->peercount; ++i) {
    if (uimusicq->peers [i] == NULL) {
      continue;
    }
    uimusicqSetPeerFlag (uimusicq->peers [i], true);
    uimusicqSetSelectLocation (uimusicq->peers [i], ci, idx);
    uimusicqSetPeerFlag (uimusicq->peers [i], false);
  }
}

static void
uimusicqSetDefaultSelection (uimusicq_t *uimusicq)
{
  uimusicqgtk_t  *uiw;
  int             ci;
  int             count;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;

  if (uiw->sel == NULL) {
    return;
  }

  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (uimusicq->ui [ci].count > 0 && count < 1) {
    GtkTreeModel  *model;
    GtkTreeIter   iter;
    int           valid;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));
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
  uimusicqgtk_t *uiw;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  if (! uimusicq->ui [mqidx].hasui) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "no-ui");
    return;
  }
  uiw = uimusicq->ui [mqidx].uiWidgets;

  snprintf (tbuff, sizeof (tbuff), "%ld", uimusicq->ui [mqidx].selectLocation);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL &&
      GTK_IS_TREE_VIEW (uiw->musicqTree)) {
    gtk_tree_selection_select_path (uiw->sel, path);
    uimusicqMusicQueueSetSelected (uimusicq, mqidx, UIMUSICQ_SEL_CURR);
    if (uimusicq->ui [mqidx].count > 0 &&
        GTK_IS_TREE_VIEW (uiw->musicqTree)) {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiw->musicqTree),
          path, NULL, FALSE, 0.0, 0.0);
    }
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


  logMsg (LOG_DBG, LOG_ACTIONS, "= action: edit");
  if (uimusicq->newselcb != NULL) {
    dbidx = uimusicqGetSelectionDbidx (uimusicq);
    if (dbidx < 0) {
      return UICB_CONT;
    }
    uiutilsCallbackLongHandler (uimusicq->newselcb, dbidx);
  }
  return uiutilsCallbackHandler (uimusicq->editcb);
}

static bool
uimusicqMoveTopCallback (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;
  long        idx;


  logProcBegin (LOG_PROC, "uimusicqMoveTopProcess");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: move top");

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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: move up");

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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: move down");

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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: toggle pause");

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
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: remove song from musicq");

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
  uimusicqgtk_t * uiw;
  int           ci;
  dbidx_t       dbidx;
  song_t        *song;

  logProcBegin (LOG_PROC, "uimusicqCheckFavChgSignal");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: musicq: change favorite");

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;
  if (column != uiw->favColumn) {
    logProcEnd (LOG_PROC, "uimusicqCheckFavChgSignal", "not-fav-col");
    return;
  }

  dbidx = uimusicqGetSelectionDbidx (uimusicq);
  song = dbGetByIdx (uimusicq->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uimusicq->songsavecb != NULL) {
      uiutilsCallbackLongHandler (uimusicq->songsavecb, dbidx);
    }
  }
}


