#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "conn.h"
#include "dance.h"
#include "ilist.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathbld.h"
#include "playlist.h"
#include "portability.h"
#include "progstart.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "uimusicq.h"

enum {
  DANCE_COL_IDX,
  DANCE_COL_NAME,
  DANCE_COL_MAX,
};

enum {
  PLAYLIST_COL_FNAME,
  PLAYLIST_COL_NAME,
  PLAYLIST_COL_MAX,
};

enum {
  MUSICQ_COL_UNIQUE_IDX,
  MUSICQ_COL_IDX,
  MUSICQ_COL_DISP_IDX,
  MUSICQ_COL_DBIDX,
  MUSICQ_COL_PAUSEIND,
  MUSICQ_COL_DANCE,
  MUSICQ_COL_ARTIST,
  MUSICQ_COL_TITLE,
  MUSICQ_COL_MAX,
};

typedef struct {
  slistidx_t      dbidx;
  int             idx;
  int             dispidx;
  int             uniqueidx;
  int             pflag;
} musicqupdate_t;

static void   uimusicqCreateDanceList (uimusicq_t *uimusicq);
static void   uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
static void   uimusicqClearQueueProcess (GtkButton *b, gpointer udata);
static void   uimusicqQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void   uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args);
static void   uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args);
static int    uimusicqMusicQueueDataFindRemovals (GtkTreeModel *model,
                GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void   uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char * args);
static void   uimusicqShowDanceWindow (GtkButton *b, gpointer udata);
static void   uimusicqShowPlaylistWindow (GtkButton *b, gpointer udata);
static gboolean uimusicqCloseMenus (GtkWidget *w, GdkEventFocus *event, gpointer udata);
static void   uimusicqMoveUpProcess (GtkButton *b, gpointer udata);
static void   uimusicqMoveDownProcess (GtkButton *b, gpointer udata);
static void   uimusicqTogglePauseProcess (GtkButton *b, gpointer udata);
static void   uimusicqRemoveProcess (GtkButton *b, gpointer udata);
static ssize_t uimusicqGetSelection (uimusicq_t *uimusicq);

uimusicq_t *
uimusicqInit (progstart_t *progstart, conn_t *conn)
{
  uimusicq_t    *uimusicq;

  uimusicq = malloc (sizeof (uimusicq_t));
  assert (uimusicq != NULL);
  uimusicq->progstart = progstart;
  uimusicq->conn = conn;
  uimusicq->playlistSelectOpen = false;
  uimusicq->danceSelectOpen = false;
  uimusicq->uniqueList = NULL;
  uimusicq->dispList = NULL;
  uimusicq->workList = NULL;

  return uimusicq;
}

void
uimusicqFree (uimusicq_t *uimusicq)
{
  if (uimusicq != NULL) {
    free (uimusicq);
  }
}

GtkWidget *
uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin)
{
  char              tbuff [MAXPATHLEN];
  GtkWidget         *image = NULL;
  GtkWidget         *widget = NULL;
  GtkWidget         *hbox = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkTreeSelection  *sel;


  image = gtk_image_new ();
  gtk_image_clear (GTK_IMAGE (image));
  uimusicq->clearImg = gtk_image_get_pixbuf (GTK_IMAGE (image));

  uimusicq->parentwin = parentwin;

  uimusicq->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uimusicq->box != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->box), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uimusicq->box), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uimusicq->box), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  widget = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (widget), "");
  assert (widget != NULL);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_up", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqMoveUpProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqMoveDownProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uimusicq->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqTogglePauseProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 8);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_audioremove", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqRemoveProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "Request External");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);

  widget = gtk_button_new ();
  assert (widget != NULL);
  uimusicq->playlistSelectButton = widget;
  gtk_button_set_label (GTK_BUTTON (widget), "Queue Playlist");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK
      (uimusicqShowPlaylistWindow), uimusicq);

  uimusicq->playlistSelectWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (uimusicq->playlistSelectWin), widget);
  gtk_window_set_transient_for (GTK_WINDOW (uimusicq->playlistSelectWin),
      GTK_WINDOW (parentwin));
  gtk_window_set_decorated (GTK_WINDOW (uimusicq->playlistSelectWin), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (uimusicq->playlistSelectWin), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (uimusicq->playlistSelectWin), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (uimusicq->playlistSelectWin), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (uimusicq->playlistSelectWin), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (uimusicq->playlistSelectWin), 1);
  gtk_widget_hide (GTK_WIDGET (uimusicq->playlistSelectWin));
  gtk_widget_set_events (GTK_WIDGET (uimusicq->playlistSelectWin), GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (uimusicq->playlistSelectWin),
      "focus-out-event", G_CALLBACK (uimusicqCloseMenus), uimusicq);

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (uimusicq->playlistSelectWin), widget);

  uimusicq->playlistSelect = gtk_tree_view_new ();
  g_object_ref (G_OBJECT (uimusicq->playlistSelect));
  assert (uimusicq->playlistSelect != NULL);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uimusicq->playlistSelect), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->playlistSelect), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->playlistSelect));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->playlistSelect), TRUE);
  gtk_container_add (GTK_CONTAINER (widget), uimusicq->playlistSelect);
  uimusicqCreatePlaylistList (uimusicq);
  g_signal_connect (uimusicq->playlistSelect, "row-activated",
      G_CALLBACK (uimusicqQueuePlaylistProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  uimusicq->danceSelectButton = widget;
  gtk_button_set_label (GTK_BUTTON (widget), "Queue Dance");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqShowDanceWindow), uimusicq);

  uimusicq->danceSelectWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (uimusicq->danceSelectWin), widget);
  gtk_window_set_transient_for (GTK_WINDOW (uimusicq->danceSelectWin),
      GTK_WINDOW (parentwin));
  gtk_window_set_decorated (GTK_WINDOW (uimusicq->danceSelectWin), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (uimusicq->danceSelectWin), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (uimusicq->danceSelectWin), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (uimusicq->danceSelectWin), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (uimusicq->danceSelectWin), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (uimusicq->danceSelectWin), 1);
  gtk_widget_hide (GTK_WIDGET (uimusicq->danceSelectWin));
  gtk_widget_set_events (GTK_WIDGET (uimusicq->danceSelectWin), GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (uimusicq->danceSelectWin),
      "focus-out-event", G_CALLBACK (uimusicqCloseMenus), uimusicq);

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (uimusicq->danceSelectWin), widget);

  uimusicq->danceSelect = gtk_tree_view_new ();
  assert (uimusicq->danceSelect != NULL);
  g_object_ref (G_OBJECT (uimusicq->danceSelect));
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uimusicq->danceSelect), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->danceSelect), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->danceSelect));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->danceSelect), TRUE);
  gtk_container_add (GTK_CONTAINER (widget), uimusicq->danceSelect);
  uimusicqCreateDanceList (uimusicq);
  g_signal_connect (uimusicq->danceSelect, "row-activated",
      G_CALLBACK (uimusicqQueueDanceProcess), uimusicq);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "Clear Queue");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uimusicqClearQueueProcess), uimusicq);

  /* musicq tree view */

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 300);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end (GTK_BOX (uimusicq->box), GTK_WIDGET (widget), FALSE, FALSE, 0);

  uimusicq->musicqTree = gtk_tree_view_new ();
  assert (uimusicq->musicqTree != NULL);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (uimusicq->musicqTree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uimusicq->musicqTree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uimusicq->musicqTree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->musicqTree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_margin_start (GTK_WIDGET (uimusicq->musicqTree), 4);
  gtk_widget_set_margin_end (GTK_WIDGET (uimusicq->musicqTree), 4);
  gtk_widget_set_margin_top (GTK_WIDGET (uimusicq->musicqTree), 2);
  gtk_widget_set_margin_bottom (GTK_WIDGET (uimusicq->musicqTree), 4);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->musicqTree), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uimusicq->musicqTree), FALSE);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (uimusicq->musicqTree));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DISP_IDX, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DANCE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_ARTIST, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_TITLE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->musicqTree), NULL);

  return uimusicq->box;
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

/* internal routines */

static void
uimusicqCreateDanceList (uimusicq_t *uimusicq)
{
  char              *dancenm;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  dance_t           *dances;
  ilist_t           *danceList;
  ilistidx_t        iteridx;
  ilistidx_t        idx;


  store = gtk_list_store_new (DANCE_COL_MAX, G_TYPE_ULONG, G_TYPE_STRING);

  dances = bdjvarsdf [BDJVDF_DANCES];
  danceList = danceGetDanceList (dances);

  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", dancenm);
    gtk_list_store_set (store, &iter,
        DANCE_COL_IDX, idx,
        DANCE_COL_NAME, tbuff, -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
    renderer, "text", DANCE_COL_NAME, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
//  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->danceSelect), column);
  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->danceSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}


static void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  char              *plfnm;
  char              *plnm;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  slist_t           *plList;


  store = gtk_list_store_new (PLAYLIST_COL_MAX, G_TYPE_STRING, G_TYPE_STRING);

  plList = playlistGetPlaylistList ();

  slistStartIterator (plList, &iteridx);

  while ((plnm = slistIterateKey (plList, &iteridx)) != NULL) {
    plfnm = listGetData (plList, plnm);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", plnm);
    gtk_list_store_set (store, &iter,
        PLAYLIST_COL_FNAME, plfnm,
        PLAYLIST_COL_NAME, tbuff, -1);
  }

  slistFree (plList);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", PLAYLIST_COL_NAME, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
//  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->playlistSelect), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->playlistSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
uimusicqClearQueueProcess (GtkButton *b, gpointer udata)
{
  uimusicq_t      *uimusicq = udata;

  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, NULL);
}

static void
uimusicqQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  unsigned long idx;
  char          tbuff [20];

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->danceSelect));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, DANCE_COL_IDX, &idx, -1);
    snprintf (tbuff, sizeof (tbuff), "%lu", idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
    gtk_widget_hide (GTK_WIDGET (uimusicq->danceSelectWin));
    uimusicq->danceSelectOpen = false;
  }
}

static void
uimusicqQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->playlistSelect));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    char    *plfname;

    gtk_tree_model_get (model, &iter, PLAYLIST_COL_FNAME, &plfname, -1);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, plfname);
    free (plfname);
    gtk_widget_hide (GTK_WIDGET (uimusicq->playlistSelectWin));
    uimusicq->playlistSelectOpen = false;
  }
}

static void
uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args)
{
  GtkTreeModel      *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->musicqTree));
  if (model == NULL) {
    uimusicqProcessMusicQueueDataNew (uimusicq, args);
  } else {
    uimusicqProcessMusicQueueDataUpdate (uimusicq, args);
  }
}

static void
uimusicqProcessMusicQueueDataNew (uimusicq_t *uimusicq, char * args)
{
  song_t            *song = NULL;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  ilistidx_t        danceIdx;
  char              *danceStr = NULL;
  dance_t           *dances = NULL;
  GdkPixbuf         *pixbuf = NULL;
  nlistidx_t        iteridx;
  musicqupdate_t    *musicqupdate = NULL;


  dances = bdjvarsdf [BDJVDF_DANCES];

  store = gtk_list_store_new (MUSICQ_COL_MAX,
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG,
      GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  uimusicqMusicQueueDataParse (uimusicq, args);

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song = dbGetByIdx (musicqupdate->dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    danceStr = danceGetData (dances, danceIdx, DANCE_DANCE);

    pixbuf = uimusicq->clearImg;
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

  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->musicqTree), GTK_TREE_MODEL (store));

  g_object_unref (G_OBJECT (store));
  nlistFree (uimusicq->dispList);
  nlistFree (uimusicq->uniqueList);
  uimusicq->dispList = NULL;
  uimusicq->uniqueList = NULL;
}


static void
uimusicqProcessMusicQueueDataUpdate (uimusicq_t *uimusicq, char * args)
{
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

  uimusicqMusicQueueDataParse (uimusicq, args);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->musicqTree));
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

  dances = bdjvarsdf [BDJVDF_DANCES];

  valid = gtk_tree_model_get_iter_first (model, &iter);

  nlistStartIterator (uimusicq->dispList, &iteridx);
  while ((musicqupdate = nlistIterateValueData (uimusicq->dispList, &iteridx)) != NULL) {
    song_t        *song;
    ilistidx_t    danceIdx;
    char          *danceStr;
    GdkPixbuf     *pixbuf;

    pixbuf = uimusicq->clearImg;
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

static void
uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char *args)
{
  char              *p;
  char              *tokstr;
  int               idx;
  musicqupdate_t    *musicqupdate;


  /* first, build ourselves a list to work with */
  uimusicq->uniqueList = nlistAlloc ("temp-musicq-unique", LIST_ORDERED, free);
  uimusicq->dispList = nlistAlloc ("temp-musicq-disp", LIST_ORDERED, NULL);

  /* index 0 is the currently playing song */
  idx = 1;
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
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
}

static void
uimusicqShowDanceWindow (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  GtkAllocation alloc;
  gint          x, y;

  gtk_window_get_position (GTK_WINDOW (uimusicq->parentwin), &x, &y);
  gtk_widget_get_allocation (GTK_WIDGET (uimusicq->danceSelectButton), &alloc);
  gtk_widget_show_all (GTK_WIDGET (uimusicq->danceSelectWin));
  gtk_window_move (GTK_WINDOW (uimusicq->danceSelectWin), alloc.x + x + 4, alloc.y + y + 30);
  uimusicq->danceSelectOpen = true;
}

static void
uimusicqShowPlaylistWindow (GtkButton *b, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;
  GtkAllocation alloc;
  gint          x, y;

  gtk_window_get_position (GTK_WINDOW (uimusicq->parentwin), &x, &y);
  gtk_widget_get_allocation (GTK_WIDGET (uimusicq->playlistSelectButton), &alloc);
  gtk_widget_show_all (GTK_WIDGET (uimusicq->playlistSelectWin));
  gtk_window_move (GTK_WINDOW (uimusicq->playlistSelectWin), alloc.x + x + 4, alloc.y + y + 30);
  uimusicq->playlistSelectOpen = true;
}

static gboolean
uimusicqCloseMenus (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uimusicq_t    *uimusicq = udata;

  if (uimusicq->danceSelectOpen) {
    gtk_widget_hide (GTK_WIDGET (uimusicq->danceSelectWin));
    uimusicq->danceSelectOpen = false;
  }
  if (uimusicq->playlistSelectOpen) {
    gtk_widget_hide (GTK_WIDGET (uimusicq->playlistSelectWin));
    uimusicq->playlistSelectOpen = false;
  }

  return FALSE;
}

static void
uimusicqMoveUpProcess (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];
  GtkTreeSelection  *sel;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  gboolean          valid;


  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%lu", idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_UP, tbuff);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->musicqTree));
  gtk_tree_selection_get_selected (sel, &model, &iter);
  valid = gtk_tree_model_iter_previous (model, &iter);
  if (valid) {
    GtkTreePath *path;

    gtk_tree_selection_select_iter (sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->musicqTree),
        path, NULL, TRUE, 0.2, 0.0);
    gtk_tree_path_free (path);
  }
}

static void
uimusicqMoveDownProcess (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];
  GtkTreeSelection  *sel;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  gboolean          valid;


  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%lu", idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_DOWN, tbuff);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->musicqTree));
  gtk_tree_selection_get_selected (sel, &model, &iter);
  valid = gtk_tree_model_iter_next (model, &iter);
  if (valid) {
    GtkTreePath *path;

    gtk_tree_selection_select_iter (sel, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uimusicq->musicqTree),
        path, NULL, TRUE, 0.8, 0.0);
    gtk_tree_path_free (path);
  }
}

static void
uimusicqTogglePauseProcess (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];


  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%lu", idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
}

static void
uimusicqRemoveProcess (GtkButton *b, gpointer udata)
{
  uimusicq_t        *uimusicq = udata;
  ssize_t           idx;
  char              tbuff [20];

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%lu", idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
}

static ssize_t
uimusicqGetSelection (uimusicq_t *uimusicq)
{
  GtkTreeSelection  *sel;
  GtkTreeIter       iter;
  GtkTreeModel      *model;
  ssize_t           idx;
  unsigned long     tidx;
  gint              count;


  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uimusicq->musicqTree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return -1;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);
  gtk_tree_model_get (model, &iter, MUSICQ_COL_IDX, &tidx, -1);
  idx = tidx;

  return idx;
}
