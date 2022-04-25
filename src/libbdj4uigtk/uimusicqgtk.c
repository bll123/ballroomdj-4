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
static void   uimusicqRemoveProcessSignal (GtkButton *b, gpointer udata);
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

GtkWidget *
uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci)
{
  int                   tci;
  char                  tbuff [MAXPATHLEN];
  GtkWidget             *image = NULL;
  GtkWidget             *widget = NULL;
  GtkWidget             *hbox = NULL;
  GtkCellRenderer       *renderer = NULL;
  GtkTreeViewColumn     *column = NULL;
  slist_t               *sellist;


  logProcBegin (LOG_PROC, "uimusicqActivate");

  /* temporary */
  tci = uimusicq->musicqManageIdx;
  uimusicq->musicqManageIdx = ci;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uimusicq->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));

  uimusicq->ui [ci].box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uimusicq->ui [ci].box != NULL);
  g_object_set_data (G_OBJECT (uimusicq->ui [ci].box),
      "bdj4tabtype", GUINT_TO_POINTER (UI_TAB_MUSICQ));
  gtk_widget_set_hexpand (uimusicq->ui [ci].box, TRUE);
  gtk_widget_set_vexpand (uimusicq->ui [ci].box, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_margin_top (hbox, 2);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uimusicq->ui [ci].box), hbox,
      FALSE, FALSE, 0);

  /* CONTEXT: button: move the selected song to the top of the queue */
  widget = uiutilsCreateButton (_("Move to Top"), "button_movetop",
      uimusicqMoveTopProcessSignal, uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  /* CONTEXT: button: move the selected song up in the queue */
  widget = uiutilsCreateButton (_("Move Up"), "button_up",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveUpProcessSignal), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeatSignal), uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  /* CONTEXT: button: move the selected song down in the queue */
  widget = uiutilsCreateButton (_("Move Down"), "button_down",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveDownProcessSignal), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeatSignal), uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) !=
      UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE) {
    /* CONTEXT: button: set playback to pause after the selected song is played (toggle) */
    widget = uiutilsCreateButton (_("Toggle Pause"), "button_pause",
        uimusicqTogglePauseProcessSignal, uimusicq);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  }

  /* CONTEXT: button: remove the song from the queue */
  widget = uiutilsCreateButton (_("Remove from queue"), "button_audioremove",
      uimusicqRemoveProcessSignal, uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  // ### TODO create code to handle this.
  if ((uimusicq->uimusicqflags & UIMUSICQ_FLAGS_NO_QUEUE) != UIMUSICQ_FLAGS_NO_QUEUE) {
    /* CONTEXT: button: request playback of a song external to BDJ4 (not in the database) */
    widget = uiutilsCreateButton (_("Request External"), NULL,
        NULL, uimusicq);
    gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    widget = uiutilsDropDownCreate (parentwin,
        /* CONTEXT: button: queue a playlist for playback */
        _("Queue Playlist"), uimusicqQueuePlaylistProcessSignal,
        &uimusicq->ui [ci].playlistsel, uimusicq);
    gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    uimusicqCreatePlaylistList (uimusicq);

    widget = uiutilsDropDownCreate (parentwin,
        /* CONTEXT: button: queue a dance for playback */
        _("Queue Dance"), uimusicqQueueDanceProcessSignal,
        &uimusicq->ui [ci].dancesel, uimusicq);
    uiutilsCreateDanceList (&uimusicq->ui [ci].dancesel, NULL);
    gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  }

  /* CONTEXT: button: clear the queue */
  widget = uiutilsCreateButton (_("Clear Queue"), NULL,
      uimusicqClearQueueProcessSignal, uimusicq);
  gtk_box_pack_end (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  /* musicq tree view */

  widget = uiutilsCreateScrolledWindow ();
  gtk_box_pack_start (GTK_BOX (uimusicq->ui [ci].box), widget,
      FALSE, FALSE, 0);

  uimusicq->ui [ci].musicqTree = uiutilsCreateTreeView ();
  assert (uimusicq->ui [ci].musicqTree != NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), TRUE);
  gtk_widget_set_halign (uimusicq->ui [ci].musicqTree, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (uimusicq->ui [ci].musicqTree, TRUE);
  gtk_widget_set_vexpand (uimusicq->ui [ci].musicqTree, TRUE);
  gtk_container_add (GTK_CONTAINER (widget), uimusicq->ui [ci].musicqTree);

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
  uiutilsAddDisplayColumns (uimusicq->ui [ci].musicqTree, sellist, MUSICQ_COL_MAX,
      MUSICQ_COL_FONT, MUSICQ_COL_ELLIPSIZE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), NULL);

  uimusicq->musicqManageIdx = tci;

  logProcEnd (LOG_PROC, "uimusicqActivate", "");
  return uimusicq->ui [ci].box;
}

void
uimusicqSetSelection (uimusicq_t *uimusicq, char *pathstr)
{
  int               ci;
  GtkTreePath       *path;


  logProcBegin (LOG_PROC, "uimusicqSetSelection");

  ci = uimusicq->musicqManageIdx;

  path = gtk_tree_path_new_from_string (pathstr);
  if (path != NULL) {
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
        path, NULL, FALSE);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
        path, NULL, FALSE, 0.0, 0.0);
    gtk_tree_path_free (path);
  }
  logProcEnd (LOG_PROC, "uimusicqSetSelection", "");
}


ssize_t
uimusicqGetSelection (uimusicq_t *uimusicq)
{
  int               ci;
  GtkTreeSelection  *sel;
  GtkTreeIter       iter;
  GtkTreeModel      *model;
  ssize_t           idx;
  unsigned long     tidx;
  gint              count;


  logProcBegin (LOG_PROC, "uimusicqGetSelection");

  ci = uimusicq->musicqManageIdx;

  sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqGetSelection", "count != 1");
    return -1;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
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
  GtkTreeSelection  *sel;
  GtkTreeIter       iter;
  gint              count;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueSetSelected");

  sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "uimusicqMusicQueueSetSelected", "count != 1");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);

  switch (which) {
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

    gtk_tree_selection_select_iter (sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    uimusicq->ui [ci].selPathStr = gtk_tree_path_to_string (path);
    /* macos loses the selection */
    /* set a timer to re-select it */
    mstimeset (&uimusicq->ui [ci].rowChangeTimer, 50);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
        path, NULL, FALSE, 0.0, 0.0);
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

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  if (model == NULL) {
    uimusicqProcessMusicQueueDataNew (uimusicq, args);
  } else {
    uimusicqProcessMusicQueueDataUpdate (uimusicq, args);
  }
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueData", "");
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
uimusicqRemoveProcessSignal (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  uimusicqRemoveProcess (uimusicq);
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
  idx = uiutilsDropDownSelectionGet (&uimusicq->ui [ci].dancesel, path);
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
  idx = uiutilsDropDownSelectionGet (&uimusicq->ui [ci].playlistsel, path);
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
  musicqstoretypes = uiutilsAddDisplayTypes (musicqstoretypes, sellist, &musicqcolcount);

  store = gtk_list_store_newv (musicqcolcount, musicqstoretypes);
  assert (store != NULL);
  free (musicqstoretypes);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song = dbGetByIdx (uimusicq->musicdb, musicqupdate->dbidx);

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pauseImg;
    }

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MUSICQ_COL_ELLIPSIZE, PANGO_ELLIPSIZE_END,
        MUSICQ_COL_FONT, listingFont,
        MUSICQ_COL_IDX, musicqupdate->idx,
        MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
        MUSICQ_COL_DBIDX, musicqupdate->dbidx,
        MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
        MUSICQ_COL_PAUSEIND, pixbuf,
        -1);

    uimusicqSetMusicqDisplay (uimusicq, store, &iter, song);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), GTK_TREE_MODEL (store));

  g_object_unref (G_OBJECT (store));
  nlistFree (uimusicq->dispList);
  nlistFree (uimusicq->uniqueList);
  uimusicq->dispList = NULL;
  uimusicq->uniqueList = NULL;
  logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataNew", "");
}


static void
uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  unsigned long     tlong;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeRowReference *rowref;
  musicqupdate_t    *musicqupdate;
  nlistidx_t        iteridx;
  gboolean          valid;
  char              *listingFont;


  logProcBegin (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate");
  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  uimusicq->workList = nlistAlloc ("temp-musicq-work", LIST_UNORDERED, NULL);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));

  if (model == NULL) {
    logProcEnd (LOG_PROC, "uimusicqProcessMusicQueueDataUpdate", "null-model");
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

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song_t        *song;
    GdkPixbuf     *pixbuf;

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pauseImg;
    }

    if (valid) {
      gtk_tree_model_get (model, &iter, MUSICQ_COL_UNIQUE_IDX, &tlong, -1);
      musicqupdate->uniqueidx = tlong;
    }

    song = dbGetByIdx (uimusicq->musicdb, musicqupdate->dbidx);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (! valid) {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_ELLIPSIZE, 1,
          MUSICQ_COL_FONT, listingFont,
          MUSICQ_COL_IDX, musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
      uimusicqSetMusicqDisplay (uimusicq, GTK_LIST_STORE (model), &iter, song);
    } else {
      /* all data must be updated */
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_IDX, musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          -1);
      uimusicqSetMusicqDisplay (uimusicq, GTK_LIST_STORE (model), &iter, song);
    }

    if (valid) {
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }

  nlistFree (uimusicq->dispList);
  nlistFree (uimusicq->uniqueList);
  uimusicq->dispList = NULL;
  uimusicq->uniqueList = NULL;
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
  uiutilsSetDisplayColumns (store, iter, sellist, song, MUSICQ_COL_MAX);
}
