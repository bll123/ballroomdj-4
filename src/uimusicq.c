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
  MUSICQ_COL_IDX,
  MUSICQ_COL_DISP_IDX,
  MUSICQ_COL_PAUSEIND,
  MUSICQ_COL_DANCE,
  MUSICQ_COL_ARTIST,
  MUSICQ_COL_TITLE,
  MUSICQ_COL_MAX,
};


static void  uimusicqCreateDanceList (uimusicq_t *uimusicq);
static void  uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
static void  uimusicqClearQueueProcess (GtkButton *b, gpointer udata);
static void  uimusicqQueueDanceProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void  uimusicqQueuePlaylistProcess (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static void  uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);
static void  uimusicqShowDanceWindow (GtkButton *b, gpointer udata);
static void  uimusicqShowPlaylistWindow (GtkButton *b, gpointer udata);
static gboolean uimusicqCloseMenus (GtkWidget *w, GdkEventFocus *event, gpointer udata);

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
  gtk_container_add (GTK_CONTAINER (widget), uimusicq->playlistSelect);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->playlistSelect), TRUE);
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
  gtk_container_add (GTK_CONTAINER (widget), uimusicq->danceSelect);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->danceSelect), TRUE);
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
  gtk_widget_set_margin_start (GTK_WIDGET (uimusicq->musicqTree), 4);
  gtk_widget_set_margin_end (GTK_WIDGET (uimusicq->musicqTree), 4);
  gtk_widget_set_margin_top (GTK_WIDGET (uimusicq->musicqTree), 2);
  gtk_widget_set_margin_bottom (GTK_WIDGET (uimusicq->musicqTree), 4);
//  gtk_widget_set_size_request (GTK_WIDGET (uimusicq->musicqTree), -1, 400);
  gtk_widget_set_hexpand (GTK_WIDGET (uimusicq->musicqTree), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uimusicq->musicqTree), FALSE);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (uimusicq->musicqTree));
//  gtk_box_pack_end (GTK_BOX (uimusicq->box), GTK_WIDGET (uimusicq->musicqTree),
//      TRUE, TRUE, 0);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DISP_IDX, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_DANCE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_ARTIST, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uimusicq->musicqTree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", MUSICQ_COL_TITLE, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
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
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
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
  char              *p;
  char              *tokstr;
  slistidx_t        dbidx;
  int               pflag;
  song_t            *song;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  int               idx;
  ilistidx_t        danceIdx;
  char              *danceStr;
  GtkTreeModel      *model;
  dance_t           *dances;
  GdkPixbuf         *pixbuf;


  dances = bdjvarsdf [BDJVDF_DANCES];

  store = gtk_list_store_new (MUSICQ_COL_MAX, G_TYPE_ULONG,
      G_TYPE_ULONG, GDK_TYPE_PIXBUF,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  idx = 0;
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  while (p != NULL) {
    dbidx = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    pflag = atoi (p);

    song = dbGetByIdx (dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    danceStr = danceGetData (dances, danceIdx, DANCE_DANCE);

    if (pflag) {
      pixbuf = uimusicq->pauseImg;
    } else {
      pixbuf = NULL;
    }

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MUSICQ_COL_DISP_IDX, idx + 1,
        MUSICQ_COL_PAUSEIND, pixbuf,
        MUSICQ_COL_DANCE, danceStr,
        MUSICQ_COL_ARTIST, songGetData (song, TAG_ARTIST),
        MUSICQ_COL_TITLE, songGetData (song, TAG_TITLE),
        -1);
    if (pflag) {
      g_object_unref (pixbuf);
    }

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    ++idx;
  }

#if 0
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uimusicq->musicqTree));
  if (model != NULL) {
    gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->musicqTree), NULL);
    g_object_unref (G_OBJECT (model));
  }
#endif
  /* try just replacing the model in its entirety */
  gtk_tree_view_set_model (GTK_TREE_VIEW (uimusicq->musicqTree), GTK_TREE_MODEL (store));
  g_object_unref (store);
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

