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

static void   uimusicqMoveTopProcessSignal (GtkButton *b, gpointer udata);
static void   uimusicqMoveUpProcessSignal (GtkButton *b, gpointer udata);
static void   uimusicqMoveDownProcessSignal (GtkButton *b, gpointer udata);
static void   uimusicqTogglePauseProcessSignal (GtkButton *b, gpointer udata);
static void   uimusicqStopRepeatSignal (GtkButton *b, gpointer udata);
static void   uimusicqClearQueueProcessSignal (GtkButton *b, gpointer udata);
static void   uimusicqQueueDanceProcessSignal (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqQueuePlaylistProcessSignal (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args);
static int    uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void   uimusicqSetMusicqDisplay (uimusicq_t *uimusicq,
    GtkListStore *store, GtkTreeIter *iter, song_t *song);
static int    uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata);

GtkWidget *
uimusicqBuildUI (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci)
{
  int                   tci;
  char                  tbuff [MAXPATHLEN];
  GtkWidget             *widget = NULL;
  GtkWidget             *hbox = NULL;
  GtkCellRenderer       *renderer = NULL;
  GtkTreeViewColumn     *column = NULL;
  slist_t               *sellist;


  logProcBegin (LOG_PROC, "uimusicqBuildUI");

  /* temporary */
  tci = uimusicq->musicqManageIdx;
  uimusicq->musicqManageIdx = ci;

  uimusicq->ui [ci].active = true;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uimusicq->pausePixbuf, tbuff);
  uiImageGetPixbuf (&uimusicq->pausePixbuf);
  uiWidgetMakePersistent (&uimusicq->pausePixbuf);

  uimusicq->ui [ci].box = uiCreateVertBox ();
  assert (uimusicq->ui [ci].box != NULL);
  uiWidgetExpandHoriz (uimusicq->ui [ci].box);
  uiWidgetExpandVert (uimusicq->ui [ci].box);

  hbox = uiCreateHorizBox ();
  assert (hbox != NULL);
  uiWidgetSetMarginTop (hbox, uiBaseMarginSz);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uimusicq->ui [ci].box, hbox);

  /* CONTEXT: button: move the selected song to the top of the queue */
  widget = uiCreateButton (NULL, NULL, _("Move to Top"), "button_movetop",
      uimusicqMoveTopProcessSignal, uimusicq);
  uiBoxPackStart (hbox, widget);

  /* CONTEXT: button: move the selected song up in the queue */
  widget = uiCreateButton (NULL, NULL, _("Move Up"), "button_up",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveUpProcessSignal), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeatSignal), uimusicq);
  uiBoxPackStart (hbox, widget);

  /* CONTEXT: button: move the selected song down in the queue */
  widget = uiCreateButton (NULL, NULL, _("Move Down"), "button_down",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveDownProcessSignal), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeatSignal), uimusicq);
  uiBoxPackStart (hbox, widget);

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) !=
      UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) {
    /* CONTEXT: button: set playback to pause after the selected song is played (toggle) */
    widget = uiCreateButton (NULL, NULL, _("Toggle Pause"), "button_pause",
        uimusicqTogglePauseProcessSignal, uimusicq);
    uiBoxPackStart (hbox, widget);
  }

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_REMOVE) !=
      UIMUSICQ_FLAGS_NO_REMOVE) {
    /* CONTEXT: button: remove the song from the queue */
    widget = uiCreateButton (NULL, NULL, _("Remove"), "button_audioremove",
        uimusicqRemoveProcessSignal, uimusicq);
    uiBoxPackStart (hbox, widget);
  }

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_QUEUE) != UIMUSICQ_FLAGS_NO_QUEUE) {
    /* CONTEXT: button: request playback of a song external to BDJ4 (not in the database) */
    widget = uiCreateButton (NULL, NULL, _("Request External"), NULL,
        NULL, uimusicq);
    uiWidgetDisable (widget);
    uiBoxPackEnd (hbox, widget);
  // ### TODO create code to handle the request external button

    widget = uiDropDownCreate (parentwin,
        /* CONTEXT: button: queue a playlist for playback */
        _("Queue Playlist"), uimusicqQueuePlaylistProcessSignal,
        &uimusicq->ui [ci].playlistsel, uimusicq);
    uiBoxPackEnd (hbox, widget);
    uimusicqCreatePlaylistList (uimusicq);

    widget = uiDropDownCreate (parentwin,
        /* CONTEXT: button: queue a dance for playback */
        _("Queue Dance"), uimusicqQueueDanceProcessSignal,
        &uimusicq->ui [ci].dancesel, uimusicq);
    uiutilsCreateDanceList (&uimusicq->ui [ci].dancesel, NULL);
    uiBoxPackEnd (hbox, widget);
  }

  if (uimusicq->dispselType == DISP_SEL_SONGLIST ||
      uimusicq->dispselType == DISP_SEL_EZSONGLIST) {
    widget = uiEntryCreate (&uimusicq->ui [ci].slname);
    snprintf (tbuff, sizeof (tbuff),
        "entry { color: %s; }",
        bdjoptGetStr (OPT_P_UI_ACCENT_COL));
    uiSetCss (widget, tbuff);
    uiBoxPackEnd (hbox, widget);
  }

  if (uimusicq->dispselType == DISP_SEL_MUSICQ) {
    /* CONTEXT: button: clear the queue */
    widget = uiCreateButton (NULL, NULL, _("Clear Queue"), NULL,
        uimusicqClearQueueProcessSignal, uimusicq);
    uiBoxPackEnd (hbox, widget);
  }

  /* musicq tree view */

  widget = uiCreateScrolledWindow (400);
  uiWidgetExpandHoriz (widget);
  uiBoxPackStartExpand (uimusicq->ui [ci].box, widget);

  uimusicq->ui [ci].musicqTree = uiCreateTreeView ();
  assert (uimusicq->ui [ci].musicqTree != NULL);
  uimusicq->ui [ci].sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), TRUE);
  uiWidgetAlignHorizFill (uimusicq->ui [ci].musicqTree);
  uiWidgetExpandHoriz (uimusicq->ui [ci].musicqTree);
  uiWidgetExpandVert (uimusicq->ui [ci].musicqTree);
  uiBoxPackInWindow (widget, uimusicq->ui [ci].musicqTree);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MUSICQ_COL_DISP_IDX,
      "font", MUSICQ_COL_FONT,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "pixbuf", MUSICQ_COL_PAUSEIND,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  uiAddDisplayColumns (uimusicq->ui [ci].musicqTree, sellist,
      MUSICQ_COL_MAX, MUSICQ_COL_FONT, MUSICQ_COL_ELLIPSIZE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), NULL);

  uimusicq->musicqManageIdx = tci;

  logProcEnd (LOG_PROC, "uimusicqBuildUI", "");
  return uimusicq->ui [ci].box;
}

void
uimusicqSetSelection (uimusicq_t *uimusicq, char *pathstr)
{
  int               ci;
  GtkTreePath       *path;


  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqSetSelection", "not-active");
    return;
  }

  path = gtk_tree_path_new_from_string (pathstr);
  if (path != NULL &&
      GTK_IS_TREE_VIEW (uimusicq->ui [ci].musicqTree)) {
    gtk_tree_selection_select_path (uimusicq->ui [ci].sel, path);
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
  gint              count;


  logProcBegin (LOG_PROC, "uimusicqGetSelection");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqGetSelection", "not-active");
    return -1;
  }

  uimusicq->ui [ci].sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  count = gtk_tree_selection_count_selected_rows (uimusicq->ui [ci].sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqGetSelection", "count != 1");
    return -1;
  }
  gtk_tree_selection_get_selected (uimusicq->ui [ci].sel, &model, &iter);
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
  gint              count;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "not-active");
    return;
  }

  count = gtk_tree_selection_count_selected_rows (uimusicq->ui [ci].sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (uimusicq->ui [ci].sel, &model, &iter);

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

    gtk_tree_selection_select_iter (uimusicq->ui [ci].sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    uimusicq->ui [ci].selPathStr = gtk_tree_path_to_string (path);

    if (GTK_IS_TREE_VIEW (uimusicq->ui [ci].musicqTree)) {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
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


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueData");

  ci = uimusicq->musicqManageIdx;
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    return;
  }

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  if (model == NULL) {
    uimusicqProcessMusicQueueDataNew (uimusicq, args);
  } else {
    uimusicqProcessMusicQueueDataUpdate (uimusicq, args);
  }
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "");
}

void
uimusicqRemoveProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqRemoveProcess (uimusicq);
}

void
uimusicqIterate (uimusicq_t *uimusicq, uimusicqiteratecb_t cb, musicqidx_t mqidx)
{
  GtkTreeModel  *model;

  uimusicq->iteratecb = cb;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [mqidx].musicqTree));
  gtk_tree_model_foreach (model, uimusicqIterateCallback, uimusicq);
}

/* internal routines */

static void
uimusicqMoveTopProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;


  uimusicqMoveTopProcess (uimusicq);
}

static void
uimusicqMoveUpProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveUpProcess (uimusicq);
  logProcEnd (LOG_PROC, "uimusicqMoveUpProcess", "");
}

static void
uimusicqMoveDownProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveDownProcess (uimusicq);
}

static void
uimusicqTogglePauseProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqTogglePauseProcess (uimusicq);
}

static void
uimusicqStopRepeatSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqStopRepeat (uimusicq);
}

static void
uimusicqClearQueueProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqClearQueueProcess (uimusicq);
}

static void
uimusicqQueueDanceProcessSignal (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  int           ci;

  ci = uimusicq->musicqManageIdx;
  idx = uiDropDownSelectionGet (&uimusicq->ui [ci].dancesel, path);
  uimusicqQueueDanceProcess (uimusicq, idx);
}

static void
uimusicqQueuePlaylistProcessSignal (GtkTreeView *tv, GtkTreePath *path,
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


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataNew");
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }

  musicqstoretypes = malloc (sizeof (GType) * MUSICQ_COL_MAX);
  musicqcolcount = 0;
  /* attributes: ellipsize/align/font */
  musicqstoretypes [musicqcolcount++] = G_TYPE_INT;
  musicqstoretypes [musicqcolcount++] = G_TYPE_STRING;
  /* internal idx/unique idx/dbidx*/
  musicqstoretypes [musicqcolcount++] = G_TYPE_ULONG;
  musicqstoretypes [musicqcolcount++] = G_TYPE_ULONG;
  musicqstoretypes [musicqcolcount++] = G_TYPE_ULONG;
  /* display disp idx/pause ind*/
  musicqstoretypes [musicqcolcount++] = G_TYPE_ULONG;
  musicqstoretypes [musicqcolcount++] = GDK_TYPE_PIXBUF;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  musicqstoretypes = uiAddDisplayTypes (musicqstoretypes, sellist, &musicqcolcount);

  store = gtk_list_store_newv (musicqcolcount, musicqstoretypes);
  assert (store != NULL);
  free (musicqstoretypes);

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
        MUSICQ_COL_IDX, (gulong) musicqupdate->idx,
        MUSICQ_COL_UNIQUE_IDX, (gulong) musicqupdate->uniqueidx,
        MUSICQ_COL_DBIDX, (gulong) musicqupdate->dbidx,
        MUSICQ_COL_DISP_IDX, (gulong) musicqupdate->dispidx,
        MUSICQ_COL_PAUSEIND, pixbuf,
        -1);

    uimusicqSetMusicqDisplay (uimusicq, store, &iter, song);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), GTK_TREE_MODEL (store));

  g_object_unref (G_OBJECT (store));
  uimusicqMusicQueueDataFree (uimusicq);
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


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate");
  ci = uimusicqMusicQueueDataParse (uimusicq, args);
  if (! uimusicq->ui [ci].active) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "not-active");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }

  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  uimusicq->workList = nlistAlloc ("temp-musicq-work", LIST_UNORDERED, NULL);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));

  if (model == NULL) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "null-model");
    uimusicqMusicQueueDataFree (uimusicq);
    return;
  }

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
          MUSICQ_COL_IDX, (gulong) musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, (gulong) musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (gulong) musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, (gulong) musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
      uimusicqSetMusicqDisplay (uimusicq, GTK_LIST_STORE (model), &iter, song);
    } else {
      /* all data must be updated */
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_IDX, (gulong) musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, (gulong) musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, (gulong) musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, (gulong) musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
      uimusicqSetMusicqDisplay (uimusicq, GTK_LIST_STORE (model), &iter, song);
    }

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
uimusicqSetMusicqDisplay (uimusicq_t *uimusicq, GtkListStore *store,
    GtkTreeIter *iter, song_t *song)
{
  slist_t     *sellist;

  sellist = dispselGetList (uimusicq->dispsel, uimusicq->dispselType);
  uiSetDisplayColumns (store, iter, sellist, song, MUSICQ_COL_MAX);
}

static int
uimusicqIterateCallback (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uimusicq_t  *uimusicq = udata;
  gulong      dbidx;

  gtk_tree_model_get (model, iter, MUSICQ_COL_DBIDX, &dbidx, -1);
  uimusicq->iteratecb (uimusicq, dbidx);
  return FALSE;
}
