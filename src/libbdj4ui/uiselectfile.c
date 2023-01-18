/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "mdebug.h"
#include "nlist.h"
#include "playlist.h"
#include "ui.h"
#include "callback.h"
#include "uiselectfile.h"

enum {
  SELFILE_COL_DISP,
  SELFILE_COL_SB_PAD,
  SELFILE_COL_MAX,
};

typedef struct uiselectfile {
  UIWidget          *parentwinp;
  UIWidget          uidialog;
  uitree_t          *selfiletree;
  callback_t        *cb;
  selfilecb_t       selfilecb;
  nlist_t           *options;
  void              *cbudata;
} uiselectfile_t;

static void selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, selfilecb_t cb);
static void selectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);
static bool selectFileResponseHandler (void *udata, long responseid);

void
selectFileDialog (int type, UIWidget *window, nlist_t *options,
    void *udata, selfilecb_t cb)
{
  uiselectfile_t *selectfile;
  slist_t     *filelist;
  int         x, y;
  int         playlistSel;
  const char  *title;

  selectfile = mdmalloc (sizeof (uiselectfile_t));
  selectfile->parentwinp = window;
  uiutilsUIWidgetInit (&selectfile->uidialog);
  selectfile->selfiletree = NULL;
  selectfile->cb = NULL;
  selectfile->selfilecb = NULL;
  selectfile->options = options;
  selectfile->cbudata = udata;

  /* CONTEXT: select file: file type for the file selection dialog (song list) */
  title = _("Song List");

  playlistSel = PL_LIST_NORMAL; /* excludes queuedance */
  switch (type) {
    case SELFILE_SONGLIST: {
      playlistSel = PL_LIST_SONGLIST;
      break;
    }
    case SELFILE_SEQUENCE: {
      playlistSel = PL_LIST_SEQUENCE;
      /* CONTEXT: select file: file type for the file selection dialog (sequence) */
      title = _("Sequence");
      break;
    }
    case SELFILE_PLAYLIST: {
      playlistSel = PL_LIST_ALL;
      /* CONTEXT: select file: file type for the file selection dialog (playlist) */
      title = _("Playlist");
      break;
    }
  }

  if (cb != NULL) {
    filelist = playlistGetPlaylistList (playlistSel);

    selectFileCreateDialog (selectfile, filelist, title, cb);
    uiWidgetShowAll (&selectfile->uidialog);

    x = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_X);
    y = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y);
    uiWindowMove (&selectfile->uidialog, x, y, -1);

    slistFree (filelist);
  }
}

void
selectFileFree (uiselectfile_t *selectfile)
{
  if (selectfile != NULL) {
    callbackFree (selectfile->cb);
    uiTreeViewFree (selectfile->selfiletree);
    mdfree (selectfile);
  }
}

static void
selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, selfilecb_t cb)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uitreewidgetp;
  UIWidget      scwindow;
  char          tbuff [200];
  GtkListStore  *store;
  GtkTreeIter   iter;
  slistidx_t    fliteridx;
  char          *disp;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;


  selectfile->selfilecb = cb;

  selectfile->cb = callbackInitLong (
      selectFileResponseHandler, selectfile);

  /* CONTEXT: select file: title of window: select <file-type> */
  snprintf (tbuff, sizeof (tbuff), _("Select %s"), filetype);
  uiCreateDialog (&selectfile->uidialog,
      selectfile->parentwinp, selectfile->cb, tbuff,
      /* CONTEXT: select file: closes the dialog */
      _("Close"), RESPONSE_CLOSE,
      /* CONTEXT: select file: selects the file */
      _("Select"), RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiWidgetExpandVert (&vbox);
  uiDialogPackInDialog (&selectfile->uidialog, &vbox);

  uiCreateScrolledWindow (&scwindow, 200);
  uiWidgetExpandHoriz (&scwindow);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  selectfile->selfiletree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetUIWidget (selectfile->selfiletree);
  uiTreeViewDisableSingleClick (selectfile->selfiletree);
  uiWidgetAlignHorizFill (uitreewidgetp);
  uiWidgetAlignVertFill (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);

  store = gtk_list_store_new (SELFILE_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  slistStartIterator (filelist, &fliteridx);
  while ((disp = slistIterateKey (filelist, &fliteridx)) != NULL) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        SELFILE_COL_DISP, disp,
        SELFILE_COL_SB_PAD, "  ",
        -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", SELFILE_COL_DISP,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", SELFILE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uitreewidgetp->widget), GTK_TREE_MODEL (store));
  g_object_unref (store);

  g_signal_connect (uitreewidgetp->widget, "row-activated",
      G_CALLBACK (selectFileSelect), selectfile);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);
}

static void
selectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uiselectfile_t  *selectfile = udata;

  selectFileResponseHandler (selectfile, RESPONSE_APPLY);
}

static bool
selectFileResponseHandler (void *udata, long responseid)
{
  uiselectfile_t  *selectfile = udata;
  int           x, y, ws;
  char          *str;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           count;

  uiWindowGetPosition (&selectfile->uidialog, &x, &y, &ws);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_X, x);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      selectfile->selfilecb = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      uiCloseWindow (&selectfile->uidialog);
      selectfile->selfilecb = NULL;
      break;
    }
    case RESPONSE_APPLY: {
      count = uiTreeViewGetSelection (selectfile->selfiletree, &model, &iter);
      if (count != 1) {
        break;
      }

      gtk_tree_model_get (model, &iter, SELFILE_COL_DISP, &str, -1);
      uiCloseWindow (&selectfile->uidialog);
      if (selectfile->selfilecb != NULL) {
        selectfile->selfilecb (selectfile->cbudata, str);
      }
      selectfile->selfilecb = NULL;
      break;
    }
  }

  selectFileFree (selectfile);
  return UICB_CONT;
}

