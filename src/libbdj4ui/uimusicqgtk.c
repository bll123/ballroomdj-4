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
#include "musicdb.h"
#include "pathbld.h"
#include "uimusicq.h"
#include "ui.h"
#include "uisong.h"
#include "uiutils.h"

enum {
  MUSICQ_COL_ELLIPSIZE,
  MUSICQ_COL_FONT,
  MUSICQ_COL_IDX,
  MUSICQ_COL_UNIQUE_IDX,
  MUSICQ_COL_DBIDX,
  MUSICQ_COL_DISP_IDX,
  MUSICQ_COL_PAUSEIND,
  MUSICQ_COL_MAX,
  MUSICQ_COL_DANCE,
  MUSICQ_COL_TITLE,
  MUSICQ_COL_ARTIST,
};

static void   uimusicqQueueDance (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqQueuePlaylist (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args);
static int    uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void   uimusicqSetMusicqDisplay (uimusicq_t *uimusicq,
    GtkTreeModel *model, GtkTreeIter *iter, song_t *song);
static int    uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);

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
  UIMUSICQ_CB_MAX,
};

typedef struct uimusicqgtk {
  uidropdown_t  dancesel;
  GtkWidget     *musicqTree;
  GtkTreeSelection  *sel;
  char          *selPathStr;
  UICallback    callback [UIMUSICQ_CB_MAX];
} uimusicqgtk_t;

void
uimusicqUIInit (uimusicq_t *uimusicq)
{
  uimusicqgtk_t   *uiw;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiw = malloc (sizeof (uimusicqgtk_t));
    uimusicq->ui [i].uiWidgets = uiw;
    uiDropDownInit (&uiw->dancesel);
    uiw->selPathStr = NULL;
    uiw->musicqTree = NULL;
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
    uiDropDownFree (&uiw->dancesel);
    if (uiw != NULL) {
      free (uiw);
    }
  }
}

UIWidget *
uimusicqBuildUI (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci)
{
  int               tci;
  char              tbuff [MAXPATHLEN];
  GtkWidget         *widget = NULL;
  UIWidget          hbox;
  UIWidget          uiwidget;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  slist_t           *sellist;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqBuildUI");

  /* temporary */
  tci = uimusicq->musicqManageIdx;
  uimusicq->musicqManageIdx = ci;
  uiw = uimusicq->ui [ci].uiWidgets;

  uimusicq->ui [ci].active = true;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
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

  uiutilsUICallbackInit (
      &uiw->callback [UIMUSICQ_CB_MOVE_TOP],
      uimusicqMoveTop, uimusicq);
  uiCreateButton (&uiwidget,
      &uiw->callback [UIMUSICQ_CB_MOVE_TOP],
      /* CONTEXT: button: move the selected song to the top of the queue */
      _("Move to Top"), "button_movetop");
  uiBoxPackStart (&hbox, &uiwidget);

  /* CONTEXT: button: move the selected song up in the queue */
  uiCreateButton (&uiwidget, NULL, _("Move Up"), "button_up");
  uiutilsUICallbackInit (
      &uiw->callback [UIMUSICQ_CB_MOVE_UP_PRESS],
      uimusicqMoveUp, uimusicq);
  uiButtonSetPressCallback (&uiwidget,
      &uiw->callback [UIMUSICQ_CB_MOVE_UP_PRESS]);
  uiutilsUICallbackInit (
      &uiw->callback [UIMUSICQ_CB_MOVE_UP_RELEASE],
      uimusicqStopRepeat, uimusicq);
  uiButtonSetReleaseCallback (&uiwidget,
      &uiw->callback [UIMUSICQ_CB_MOVE_UP_RELEASE]);
  uiBoxPackStart (&hbox, &uiwidget);

  /* CONTEXT: button: move the selected song down in the queue */
  uiCreateButton (&uiwidget, NULL, _("Move Down"), "button_down");
  uiutilsUICallbackInit (
      &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_PRESS],
      uimusicqMoveDown, uimusicq);
  uiButtonSetPressCallback (&uiwidget,
      &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_PRESS]);
  uiutilsUICallbackInit (
      &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_RELEASE],
      uimusicqStopRepeat, uimusicq);
  uiButtonSetReleaseCallback (&uiwidget,
      &uiw->callback [UIMUSICQ_CB_MOVE_DOWN_RELEASE]);
  uiBoxPackStart (&hbox, &uiwidget);

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) !=
      UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_TOGGLE_PAUSE],
        uimusicqTogglePause, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_TOGGLE_PAUSE],
        /* CONTEXT: button: set playback to pause after the selected song is played (toggle) */
        _("Toggle Pause"), "button_pause");
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_REMOVE) !=
      UIMUSICQ_FLAGS_NO_REMOVE) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_AUDIO_REMOVE],
        uimusicqRemove, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_AUDIO_REMOVE],
        /* CONTEXT: button: remove the song from the queue */
        _("Remove"), "button_audioremove");
    uiBoxPackStart (&hbox, &uiwidget);
  }

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_QUEUE) != UIMUSICQ_FLAGS_NO_QUEUE) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_REQ_EXTERNAL],
        NULL, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_REQ_EXTERNAL],
        /* CONTEXT: button: request playback of a song external to BDJ4 (not in the database) */
        _("Request External"), NULL);
    uiWidgetDisable (&uiwidget);
    uiBoxPackEnd (&hbox, &uiwidget);
  // ### TODO create code to handle the request external button

    widget = uiDropDownCreate (parentwin,
        /* CONTEXT: button: queue a playlist for playback */
        _("Queue Playlist"), uimusicqQueuePlaylist,
        &uimusicq->ui [ci].playlistsel, uimusicq);
    uiBoxPackEndUW (&hbox, widget);
    uimusicqCreatePlaylistList (uimusicq);

    widget = uiDropDownCreate (parentwin,
        /* CONTEXT: button: queue a dance for playback */
        _("Queue Dance"), uimusicqQueueDance,
        &uiw->dancesel, uimusicq);
    uiutilsCreateDanceList (&uiw->dancesel, NULL);
    uiBoxPackEndUW (&hbox, widget);
  }

  if (uimusicq->dispselType == DISP_SEL_SONGLIST ||
      uimusicq->dispselType == DISP_SEL_EZSONGLIST) {
    uientry_t   *entryp;
    entryp = uimusicq->ui [ci].slname;
    uiEntryCreate (entryp);
    uiEntrySetColor (entryp, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
    uiBoxPackEnd (&hbox, uiEntryGetUIWidget (entryp));

    /* CONTEXT: label for song list name */
    uiCreateColonLabel (&uiwidget, _("Song List"));
    uiBoxPackEnd (&hbox, &uiwidget);
  }

  if (uimusicq->dispselType == DISP_SEL_MUSICQ) {
    uiutilsUICallbackInit (
        &uiw->callback [UIMUSICQ_CB_CLEAR_QUEUE],
        uimusicqClearQueue, uimusicq);
    uiCreateButton (&uiwidget,
        &uiw->callback [UIMUSICQ_CB_CLEAR_QUEUE],
        /* CONTEXT: button: clear the queue */
        _("Clear Queue"), NULL);
    uiBoxPackEnd (&hbox, &uiwidget);
  }

  /* musicq tree view */

  uiCreateScrolledWindow (&uiwidget, 400);
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStartExpand (&uimusicq->ui [ci].mainbox, &uiwidget);

  uiw->musicqTree = uiCreateTreeView ();
  assert (uiw->musicqTree != NULL);
  uiw->sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uiw->musicqTree));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiw->musicqTree), TRUE);
  uiWidgetAlignHorizFillW (uiw->musicqTree);
  uiWidgetExpandHorizW (uiw->musicqTree);
  uiWidgetExpandVertW (uiw->musicqTree);
  uiBoxPackInWindowUW (&uiwidget, uiw->musicqTree);

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

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  uiAddDisplayColumns (uiw->musicqTree, sellist,
      MUSICQ_COL_MAX, MUSICQ_COL_FONT, MUSICQ_COL_ELLIPSIZE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->musicqTree), NULL);

  uimusicq->musicqManageIdx = tci;

  logProcEnd (LOG_PROC, "uimusicqBuildUI", "");
  return &uimusicq->ui [ci].mainbox;
}

void
uimusicqSetSelection (uimusicq_t *uimusicq, char *pathstr)
{
  int               ci;
  GtkTreePath       *path;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "not-active");
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  path = gtk_tree_path_new_from_string (pathstr);
  if (path != NULL &&
      GTK_IS_TREE_VIEW (uiw->musicqTree)) {
    gtk_tree_selection_select_path (uiw->sel, path);
    uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_CURR);
  }
  if (path != NULL) {
    gtk_tree_path_free (path);
  }
  logProcEnd (LOG_PROC, "uimusicqSetSelection", "");
}


ssize_t
uimusicqGetSelection (uimusicq_t *uimusicq)
{
  int               ci;
  GtkTreeIter       iter;
  GtkTreeModel      *model;
  ssize_t           idx;
  unsigned long     tidx;
  int               count;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqGetSelection");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqGetSelection", "not-active");
    return -1;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  uiw->sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uiw->musicqTree));
  count = gtk_tree_selection_count_selected_rows (uiw->sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqGetSelection", "count != 1");
    return -1;
  }
  gtk_tree_selection_get_selected (uiw->sel, &model, &iter);
  gtk_tree_model_get (model, &iter, MUSICQ_COL_IDX, &tidx, -1);
  idx = tidx;

  logProcEnd (LOG_PROC, "uimusicqGetSelection", "");
  return idx;
}

void
uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which)
{
  GtkTreeModel      *model;
  gboolean          valid;
  GtkTreeIter       iter;
  int               count;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "not-active");
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

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
    GtkTreePath *path;

    gtk_tree_selection_select_iter (uiw->sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    uiw->selPathStr = gtk_tree_path_to_string (path);

    if (GTK_IS_TREE_VIEW (uiw->musicqTree)) {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiw->musicqTree),
          path, NULL, FALSE, 0.0, 0.0);
    }
    gtk_tree_path_free (path);
  }
  logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "");
}


void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  GtkTreeModel      *model;
  uimusicqgtk_t     *uiw;

  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueData");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));
  if (model == NULL) {
    uimusicqProcessMusicQueueDataNew (uimusicq, args);
  } else {
    uimusicqProcessMusicQueueDataUpdate (uimusicq, args);
  }
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

/* internal routines */

static void
uimusicqQueueDance (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  int           ci;
  uimusicqgtk_t *uiw;

  ci = uimusicq->musicqManageIdx;
  uiw = uimusicq->ui [ci].uiWidgets;
  idx = uiDropDownSelectionGet (&uiw->dancesel, path);
  uimusicqQueueDanceProcess (uimusicq, idx);
}

static void
uimusicqQueuePlaylist (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  idx = uiDropDownSelectionGet (&uimusicq->ui [ci].playlistsel, path);
  uimusicqQueuePlaylistProcess (uimusicq, idx);
}

static void
uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  song_t            *song = NULL;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GdkPixbuf         *pixbuf = NULL;
  nlistidx_t        iteridx;
  musicqupdate_t    *musicqupdate = NULL;
  char              *listingFont;
  int               musicqcolcount;
  GType             *musicqstoretypes;
  slist_t           *sellist;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataNew");
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  musicqstoretypes = malloc (sizeof (GType) * MUSICQ_COL_MAX);
  musicqcolcount = 0;
  /* attributes: ellipsize/align/font */
  musicqstoretypes [musicqcolcount++] = G_TYPE_INT;
  musicqstoretypes [musicqcolcount++] = G_TYPE_STRING;
  /* internal idx/unique idx/dbidx*/
  musicqstoretypes [musicqcolcount++] = G_TYPE_LONG;
  musicqstoretypes [musicqcolcount++] = G_TYPE_LONG;
  musicqstoretypes [musicqcolcount++] = G_TYPE_LONG;
  /* display disp idx/pause ind*/
  musicqstoretypes [musicqcolcount++] = G_TYPE_LONG;
  musicqstoretypes [musicqcolcount++] = GDK_TYPE_PIXBUF;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  musicqstoretypes = uisongAddDisplayTypes (musicqstoretypes, sellist, &musicqcolcount);

  store = gtk_list_store_newv (musicqcolcount, musicqstoretypes);
  assert (store != NULL);
  free (musicqstoretypes);
  gtk_tree_view_set_model (GTK_TREE_VIEW (uiw->musicqTree), GTK_TREE_MODEL (store));

  uimusicqProcessMusicQueueDataUpdate (uimusicq, args);

#if 0
  uimusicq->count = nlistGetCount (uimusicq->dispList);
  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song = dbGetByIdx (uimusicq->musicdb, musicqupdate->dbidx);

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pausePixbuf.pixbuf;
    }

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MUSICQ_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
        MUSICQ_COL_FONT, listingFont,
        MUSICQ_COL_IDX, (glong) musicqupdate->idx,
        MUSICQ_COL_UNIQUE_IDX, (glong) musicqupdate->uniqueidx,
        MUSICQ_COL_DBIDX, (glong) musicqupdate->dbidx,
        MUSICQ_COL_DISP_IDX, (glong) musicqupdate->dispidx,
        MUSICQ_COL_PAUSEIND, pixbuf,
        -1);

    uimusicqSetMusicqDisplay (uimusicq, GTK_TREE_MODEL (store), &iter, song);
  }
#endif

  g_object_unref (G_OBJECT (store));

//  uimusicqMusicQueueDataFree (uimusicq);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataNew", "");
}


static void
uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeRowReference *rowref;
  musicqupdate_t    *musicqupdate;
  nlistidx_t        iteridx;
  gboolean          valid;
  char              *listingFont;
  uimusicqgtk_t     *uiw;


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate");
  ci = uimusicqMusicQueueDataParse (uimusicq, args);
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }
  uiw = uimusicq->ui [ci].uiWidgets;

  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  uimusicq->workList = nlistAlloc ("temp-musicq-work", LIST_UNORDERED, NULL);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiw->musicqTree));

  if (model == NULL) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "null-model");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }

#if 0
  gtk_tree_model_foreach (model,
      uimusicqMusicQueueDataFindRemovals, uimusicq);

  /* process removals */
  nlistStartIterator (uimusicq->workList, &iteridx);
  while ((rowref = nlistIterateValueData (uimusicq->workList, &iteridx)) != NULL) {
    GtkTreeIter iter;
    GtkTreePath *path;

    path = gtk_tree_row_reference_get_path (rowref);
    if (path) {
      if (gtk_tree_model_get_iter (model, &iter, path)) {
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      }
      gtk_tree_path_free (path);
    }
  }
#endif

  nlistFree (uimusicq->workList);
  uimusicq->workList = NULL;

  valid = gtk_tree_model_get_iter_first (model, &iter);

  uimusicq->count = nlistGetCount (uimusicq->dispList);
  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song_t        *song;
    GdkPixbuf     *pixbuf;

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pausePixbuf.pixbuf;
    }

    song = dbGetByIdx (uimusicq->musicdb, musicqupdate->dbidx);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (! valid) {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
          MUSICQ_COL_FONT, listingFont,
          MUSICQ_COL_IDX, (glong) musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, (glong) musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (glong) musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, (glong) musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
    } else {
      /* all data must be updated */
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_IDX, (glong) musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, (glong) musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (glong) musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, (glong) musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
    }
    uimusicqSetMusicqDisplay (uimusicq, model, &iter, song);

    if (valid) {
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }

  uimusicqMusicQueueDataFree (uimusicq);
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "");
}

static int
uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uimusicq_t      *uimusicq = udata;
  unsigned long   uniqueidx;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueDataFindRemovals");

  gtk_tree_model_get (model, iter, MUSICQ_COL_UNIQUE_IDX, &uniqueidx, -1);
  if (nlistGetData (uimusicq->uniqueList, uniqueidx) == NULL) {
    GtkTreeRowReference *rowref;

    rowref = gtk_tree_row_reference_new (model, path);
    nlistSetData (uimusicq->workList, uniqueidx, rowref);
  }

  logProcEnd (LOG_PROC, "uimusicqMusicQueueDataFindRemovals", "");
  return FALSE;
}

static void
uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, GtkTreeModel *model,
    GtkTreeIter *iter, song_t *song)
{
  slist_t     *sellist;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  uisongSetDisplayColumns (sellist, song, MUSICQ_COL_MAX, model, iter);
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
