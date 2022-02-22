#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "conn.h"
#include "dance.h"
#include "ilist.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "pathbld.h"
#include "playlist.h"
#include "progstate.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "uimusicq.h"
#include "uiutils.h"

enum {
  MUSICQ_COL_IDX,
  MUSICQ_COL_DISP_IDX,
  MUSICQ_COL_UNIQUE_IDX,
  MUSICQ_COL_DBIDX,
  MUSICQ_COL_PAUSEIND,
  MUSICQ_COL_DANCE,
  MUSICQ_COL_TITLE,
  MUSICQ_COL_ARTIST,
  MUSICQ_COL_MAX,
};

enum {
  UIMUSICQ_SEL_NONE,
  UIMUSICQ_SEL_PREV,
  UIMUSICQ_SEL_NEXT,
  UIMUSICQ_SEL_TOP,
};

typedef struct {
  slistidx_t      dbidx;
  int             idx;
  int             dispidx;
  int             uniqueidx;
  int             pflag;
} musicqupdate_t;

static void   uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
static void   uimusicqClearQueueProcess (GtkButton *b, gpointer udata);
static void   uimusicqQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args);
static int    uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
                GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static int    uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char * args);
static void   uimusicqMoveTopProcess (GtkButton *b, gpointer udata);
static void   uimusicqStopRepeat (GtkButton *b, gpointer udata);
static gboolean uimusicqMoveUpRepeat (void *udata);
static void   uimusicqMoveUpProcess (GtkButton *b, gpointer udata);
static gboolean uimusicqMoveDownRepeat (void *udata);
static void   uimusicqMoveDownProcess (GtkButton *b, gpointer udata);
static void   uimusicqTogglePauseProcess (GtkButton *b, gpointer udata);
static void   uimusicqRemoveProcess (GtkButton *b, gpointer udata);
static ssize_t uimusicqGetSelection (uimusicq_t *uimusicq);
static void   uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which);

uimusicq_t *
uimusicqInit (progstate_t *progstate, conn_t *conn)
{
  uimusicq_t    *uimusicq;

  uimusicq = malloc (sizeof (uimusicq_t) * MUSICQ_MAX);
  assert (uimusicq != NULL);
  uimusicq->progstate = progstate;
  uimusicq->conn = conn;
  uimusicq->uniqueList = NULL;
  uimusicq->dispList = NULL;
  uimusicq->workList = NULL;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiutilsDropDownInit (&uimusicq->ui [i].playlistsel);
    uiutilsDropDownInit (&uimusicq->ui [i].dancesel);
    uimusicq->ui [i].selPathStr = NULL;
    mstimeset (&uimusicq->ui [i].rowChangeTimer, 3600000);
  }
  uimusicq->musicqManageIdx = MUSICQ_A;
  uimusicq->musicqPlayIdx = MUSICQ_A;

  return uimusicq;
}

void
uimusicqFree (uimusicq_t *uimusicq)
{
  if (uimusicq != NULL) {
    free (uimusicq);
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      if (uimusicq->ui [i].selPathStr != NULL) {
        free (uimusicq->ui [i].selPathStr);
      }
      uiutilsDropDownFree (&uimusicq->ui [i].playlistsel);
      uiutilsDropDownFree (&uimusicq->ui [i].dancesel);
    }
  }
}

GtkWidget *
uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci)
{
  int               tci;
  char              tbuff [MAXPATHLEN];
  GtkWidget         *image = NULL;
  GtkWidget         *widget = NULL;
  GtkWidget         *hbox = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkTreeSelection  *sel = NULL;
  GValue            gvalue = G_VALUE_INIT;


  g_value_init (&gvalue, G_TYPE_INT);
  g_value_set_int (&gvalue, PANGO_ELLIPSIZE_END);

  /* temporary */
  tci = uimusicq->musicqManageIdx;
  uimusicq->musicqManageIdx = ci;

  uimusicq->parentwin = parentwin;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uimusicq->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));

  uimusicq->ui [ci].box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uimusicq->ui [ci].box != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->ui [ci].box), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uimusicq->ui [ci].box), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_margin_top (GTK_WIDGET (hbox), 2);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uimusicq->ui [ci].box), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Move to Top"), "button_movetop",
      uimusicqMoveTopProcess, uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Move Up"), "button_up",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveUpProcess), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeat), uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Move Down"), "button_down",
      NULL, uimusicq);
  g_signal_connect (widget, "pressed",
      G_CALLBACK (uimusicqMoveDownProcess), uimusicq);
  g_signal_connect (widget, "released",
      G_CALLBACK (uimusicqStopRepeat), uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Toggle Pause"), "button_pause",
      uimusicqTogglePauseProcess, uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Remove from queue"), "button_audioremove",
      uimusicqRemoveProcess, uimusicq);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  // ### TODO create code to handle this.
  widget = uiutilsCreateButton (_("Request External"), NULL,
      NULL, uimusicq);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = uiutilsDropDownCreate (parentwin,
      _("Queue Playlist"), uimusicqQueuePlaylistProcess,
      &uimusicq->ui [ci].playlistsel, uimusicq);
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  uimusicqCreatePlaylistList (uimusicq);

  widget = uiutilsDropDownCreate (parentwin,
      _("Queue Dance"), uimusicqQueueDanceProcess,
      &uimusicq->ui [ci].dancesel, uimusicq);
  uiutilsCreateDanceList (&uimusicq->ui [ci].dancesel);
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Clear Queue"), NULL,
      uimusicqClearQueueProcess, uimusicq);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqClearQueueProcess), uimusicq);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  /* musicq tree view */

  widget = uiutilsCreateScrolledWindow ();
  gtk_box_pack_start (GTK_BOX (uimusicq->ui [ci].box), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  uimusicq->ui [ci].musicqTree = gtk_tree_view_new ();
  assert (uimusicq->ui [ci].musicqTree != NULL);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_margin_start (GTK_WIDGET (uimusicq->ui [ci].musicqTree), 4);
  gtk_widget_set_margin_end (GTK_WIDGET (uimusicq->ui [ci].musicqTree), 4);
  gtk_widget_set_margin_top (GTK_WIDGET (uimusicq->ui [ci].musicqTree), 2);
  gtk_widget_set_margin_bottom (GTK_WIDGET (uimusicq->ui [ci].musicqTree), 4);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->ui [ci].musicqTree), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uimusicq->ui [ci].musicqTree), TRUE);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (uimusicq->ui [ci].musicqTree));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DISP_IDX, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "pixbuf", MUSICQ_COL_PAUSEIND, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DANCE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_TITLE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_fixed_width (column, 400);
  gtk_tree_view_column_set_expand (column, TRUE);
  g_object_set_property (G_OBJECT (renderer), "ellipsize", &gvalue);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_ARTIST, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_fixed_width (column, 250);
  gtk_tree_view_column_set_expand (column, TRUE);
  g_object_set_property (G_OBJECT (renderer), "ellipsize", &gvalue);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), NULL);

  g_value_unset (&gvalue);

  uimusicq->musicqManageIdx = tci;
  return uimusicq->ui [ci].box;
}

void
uimusicqMainLoop (uimusicq_t *uimusicq)
{
  int           ci;

  ci = uimusicq->musicqManageIdx;

  /* macos loses the selection after it is moved.  reset it. */
  /* fortunately, it seems to only need to be reset once. */
  if (mstimeCheck (&uimusicq->ui [ci].rowChangeTimer)) {
    GtkTreePath       *path;

    if (uimusicq->ui [ci].selPathStr != NULL) {
      path = gtk_tree_path_new_from_string (uimusicq->ui [ci].selPathStr);
      if (path != NULL) {
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
            path, NULL, FALSE);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree),
            path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free (path);
      }
    }
    mstimeset (&uimusicq->ui [ci].rowChangeTimer, 3600000);
  }

  return;
}

int
uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uimusicq_t    *uimusicq = udata;

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_MUSIC_QUEUE_DATA: {
          uimusicqProcessMusicQueueData (uimusicq, args);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  return 0;
}

void
uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx)
{
  uimusicq->musicqPlayIdx = playIdx;
}

void
uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx)
{
  uimusicq->musicqManageIdx = manageIdx;
}

/* internal routines */

static void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  int               ci;
  slist_t           *plList;

  ci = uimusicq->musicqManageIdx;

  plList = playlistGetPlaylistList ();
  uiutilsDropDownSetList (&uimusicq->ui [ci].playlistsel, plList);
}

static void
uimusicqClearQueueProcess (GtkButton *b, gpointer udata)
{
  int           ci;
  int           idx;
  uimusicq_t    *uimusicq = udata;
  char          tbuff [20];

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    /* if nothing is selected, clear everything */
    idx = 0;
    if (uimusicq->musicqManageIdx == uimusicq->musicqPlayIdx) {
      idx = 1;
    }
  }

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tbuff);
}

static void
uimusicqQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  int           ci;
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  char          tbuff [20];

  ci = uimusicq->musicqManageIdx;

  idx = uiutilsDropDownSelectionGet (&uimusicq->ui [ci].dancesel, path);
  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
  }
}

static void
uimusicqQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  int           ci;
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  char          tbuff [100];

  ci = uimusicq->musicqManageIdx;

  idx = uiutilsDropDownSelectionGet (&uimusicq->ui [ci].playlistsel, path);
fprintf (stderr, "pl-sel: %zd %s\n", idx,
uimusicq->ui [ci].playlistsel.strSelection);
  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%s", ci, MSG_ARGS_RS,
        uimusicq->ui [ci].playlistsel.strSelection);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
}

static void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  GtkTreeModel      *model;

  ci = uimusicq->musicqManageIdx;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  if (model == NULL) {
    uimusicqProcessMusicQueueDataNew (uimusicq, args);
  } else {
    uimusicqProcessMusicQueueDataUpdate (uimusicq, args);
  }
}

static void
uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  song_t            *song = NULL;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  ilistidx_t        danceIdx;
  char              *danceStr = NULL;
  dance_t           *dances = NULL;
  GdkPixbuf         *pixbuf = NULL;
  nlistidx_t        iteridx;
  musicqupdate_t    *musicqupdate = NULL;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  store = gtk_list_store_new (MUSICQ_COL_MAX,
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG,
      GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song = dbGetByIdx (musicqupdate->dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    danceStr = danceGetData (dances, danceIdx, DANCE_DANCE);

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pauseImg;
    }

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MUSICQ_COL_IDX, musicqupdate->idx,
        MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
        MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
        MUSICQ_COL_DBIDX, musicqupdate->dbidx,
        MUSICQ_COL_PAUSEIND, pixbuf,
        MUSICQ_COL_DANCE, danceStr,
        MUSICQ_COL_ARTIST, songGetData (song, TAG_ARTIST),
        MUSICQ_COL_TITLE, songGetData (song, TAG_TITLE),
        -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree), GTK_TREE_MODEL (store));

  g_object_unref (G_OBJECT (store));
  nlistFree (uimusicq->dispList);
  nlistFree (uimusicq->uniqueList);
  uimusicq->dispList = NULL;
  uimusicq->uniqueList = NULL;
}


static void
uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args)
{
  int               ci;
  unsigned long     tlong;
  int               uniqueidx;
  dance_t           *dances;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeRowReference *rowref;
  musicqupdate_t    *musicqupdate;
  nlistidx_t        iteridx;
  gboolean          valid;

  uimusicq->workList = nlistAlloc ("temp-musicq-work", LIST_UNORDERED, NULL);

  ci = uimusicqMusicQueueDataParse (uimusicq, args);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));

  if (model == NULL) {
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

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  valid = gtk_tree_model_get_iter_first (model, &iter);

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song_t        *song;
    ilistidx_t    danceIdx;
    char          *danceStr;
    GdkPixbuf     *pixbuf;

    pixbuf = NULL;
    if (musicqupdate->pflag) {
      pixbuf = uimusicq->pauseImg;
    }

    if (valid) {
      gtk_tree_model_get (model, &iter, MUSICQ_COL_UNIQUE_IDX, &tlong, -1);
      uniqueidx = tlong;
    }

    song = dbGetByIdx (musicqupdate->dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    danceStr = danceGetData (dances, danceIdx, DANCE_DANCE);

    /* there's no need to determine if the entry is new or not    */
    /* simply overwrite everything until the end of the gtk-store */
    /* is reached, then start appending. */
    if (! valid) {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_IDX, musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          MUSICQ_COL_DANCE, danceStr,
          MUSICQ_COL_ARTIST, songGetData (song, TAG_ARTIST),
          MUSICQ_COL_TITLE, songGetData (song, TAG_TITLE),
          -1);
    } else {
      /* all data must be updated */
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MUSICQ_COL_IDX, musicqupdate->idx,
          MUSICQ_COL_DISP_IDX, musicqupdate->dispidx,
          MUSICQ_COL_UNIQUE_IDX, musicqupdate->uniqueidx,
          MUSICQ_COL_DBIDX, musicqupdate->dbidx,
          MUSICQ_COL_PAUSEIND, pixbuf,
          MUSICQ_COL_DANCE, danceStr,
          MUSICQ_COL_ARTIST, songGetData (song, TAG_ARTIST),
          MUSICQ_COL_TITLE, songGetData (song, TAG_TITLE),
          -1);
    }

    if (valid) {
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }

  nlistFree (uimusicq->dispList);
  nlistFree (uimusicq->uniqueList);
  uimusicq->dispList = NULL;
  uimusicq->uniqueList = NULL;
}

static int
uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uimusicq_t      *uimusicq = udata;
  unsigned long   uniqueidx;

  gtk_tree_model_get (model, iter, MUSICQ_COL_UNIQUE_IDX, &uniqueidx, -1);
  if (nlistGetData (uimusicq->uniqueList, uniqueidx) == NULL) {
    GtkTreeRowReference *rowref;

    rowref = gtk_tree_row_reference_new (model, path);
    nlistSetData (uimusicq->workList, uniqueidx, rowref);
  }

  return FALSE;
}

static int
uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char *args)
{
  int               ci;
  char              *p;
  char              *tokstr;
  int               idx;
  musicqupdate_t    *musicqupdate;

  /* first, build ourselves a list to work with */
  uimusicq->uniqueList = nlistAlloc ("temp-musicq-unique", LIST_ORDERED, free);
  uimusicq->dispList = nlistAlloc ("temp-musicq-disp", LIST_ORDERED, NULL);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  ci = atoi (p);

  /* index 0 is the currently playing song */
  idx = 1;
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (p != NULL) {
    musicqupdate = malloc (sizeof (musicqupdate_t));
    assert (musicqupdate != NULL);

    musicqupdate->dispidx = atoi (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->uniqueidx = atoi (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->dbidx = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->pflag = atoi (p);
    musicqupdate->idx = idx;

    nlistSetData (uimusicq->uniqueList, musicqupdate->uniqueidx, musicqupdate);
    nlistSetData (uimusicq->dispList, musicqupdate->idx, musicqupdate);

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    ++idx;
  }

  return ci;
}


static void
uimusicqMoveTopProcess (GtkButton *b, gpointer udata)
{
  int               ci;
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];


  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_TOP);

  snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_TOP, tbuff);
}

static void
uimusicqStopRepeat (GtkButton *b, gpointer udata)
{
  int         ci;
  uimusicq_t  *uimusicq = udata;

  ci = uimusicq->musicqManageIdx;
  if (uimusicq->ui [ci].repeatTimer != 0) {
    g_source_remove (uimusicq->ui [ci].repeatTimer);
  }
  uimusicq->ui [ci].repeatTimer = 0;
}


static gboolean
uimusicqMoveUpRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveUpProcess (NULL, uimusicq);
  return TRUE;
}

static void
uimusicqMoveUpProcess (GtkButton *b, gpointer udata)
{
  int               ci;
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
  snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_UP, tbuff);
  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (UIMUSICQ_REPEAT_TIME, uimusicqMoveUpRepeat, uimusicq);
  }
}

static gboolean
uimusicqMoveDownRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveDownProcess (NULL, uimusicq);
  return TRUE;
}

static void
uimusicqMoveDownProcess (GtkButton *b, gpointer udata)
{
  int               ci;
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_NEXT);
  snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_DOWN, tbuff);

  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (UIMUSICQ_REPEAT_TIME, uimusicqMoveDownRepeat, uimusicq);
  }
}

static void
uimusicqTogglePauseProcess (GtkButton *b, gpointer udata)
{
  int           ci;
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  char          tbuff [20];

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
}

static void
uimusicqRemoveProcess (GtkButton *b, gpointer udata)
{
  int           ci;
  uimusicq_t    *uimusicq = udata;
  ssize_t       idx;
  char          tbuff [20];

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%lu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
}

static ssize_t
uimusicqGetSelection (uimusicq_t *uimusicq)
{
  int               ci;
  GtkTreeSelection  *sel;
  GtkTreeIter       iter;
  GtkTreeModel      *model;
  ssize_t           idx;
  unsigned long     tidx;
  gint              count;

  ci = uimusicq->musicqManageIdx;

  sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return -1;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, MUSICQ_COL_IDX, &tidx, -1);
  idx = tidx;

  return idx;
}

static void
uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which)
{
  GtkTreeModel      *model;
  gboolean          valid;
  GtkTreeSelection  *sel;
  GtkTreeIter       iter;
  gint              count;


  sel = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (uimusicq->ui [ci].musicqTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
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
}


