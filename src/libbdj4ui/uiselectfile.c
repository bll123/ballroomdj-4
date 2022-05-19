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
#include "nlist.h"
#include "playlist.h"
#include "ui.h"
#include "uiutils.h"
#include "uiselectfile.h"

enum {
  SELFILE_COL_DISP,
  SELFILE_COL_SB_PAD,
  SELFILE_COL_MAX,
};

typedef struct uiselectfile {
  UIWidget          *parentwinp;
  GtkWidget         *selfiledialog;
  GtkWidget         *selfiletree;
  selfilecb_t       selfilecb;
  nlist_t           *options;
  void              *cbudata;
} uiselectfile_t;

static GtkWidget * selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, selfilecb_t cb);
static void selectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata);
static void selectFileResponseHandler (GtkDialog *d, gint responseid,
    gpointer udata);

void
selectFileDialog (int type, UIWidget *window, nlist_t *options,
    void *udata, selfilecb_t cb)
{
  uiselectfile_t *selectfile;
  slist_t     *filelist;
  int         x, y;
  GtkWidget   *dialog;
  int         playlistSel;

  selectfile = malloc (sizeof (uiselectfile_t));
  selectfile->parentwinp = window;
  selectfile->selfiledialog = NULL;
  selectfile->selfiletree = NULL;
  selectfile->selfilecb = NULL;
  selectfile->options = options;
  selectfile->cbudata = udata;

  playlistSel = PL_LIST_NORMAL; /* excludes queuedance */
  switch (type) {
    case SELFILE_SONGLIST: {
      playlistSel = PL_LIST_SONGLIST;
      break;
    }
    case SELFILE_SEQUENCE: {
      playlistSel = PL_LIST_SEQUENCE;
      break;
    }
    case SELFILE_PLAYLIST: {
      playlistSel = PL_LIST_ALL;
      break;
    }
  }

  filelist = playlistGetPlaylistList (playlistSel);

  if (cb != NULL) {
    /* CONTEXT: file type for the file selection dialog (song list) */
    dialog = selectFileCreateDialog (selectfile, filelist, _("Song List"), cb);
    uiWidgetShowAllW (dialog);

    x = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_X);
    y = nlistGetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y);
    uiWindowMoveW (dialog, x, y);
  }
}

void
selectFileFree (uiselectfile_t *selectfile)
{
  if (selectfile != NULL) {
    free (selectfile);
  }
}

static GtkWidget *
selectFileCreateDialog (uiselectfile_t *selectfile,
    slist_t *filelist, const char *filetype, selfilecb_t cb)
{
  GtkWidget     *dialog;
  GtkWidget     *content;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  GtkWidget     *scwin;
  GtkWidget     *widget;
  char          tbuff [200];
  GtkListStore  *store;
  GtkTreeIter   iter;
  slistidx_t    fliteridx;
  char          *disp;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;


  selectfile->selfilecb = cb;

  /* CONTEXT: file select dialog, title of window: select <file-type> */
  snprintf (tbuff, sizeof (tbuff), _("Select %s"), filetype);
  dialog = gtk_dialog_new_with_buttons (
      tbuff,
      GTK_WINDOW (selectfile->parentwinp->widget),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      /* CONTEXT: file select dialog: closes the dialog */
      _("Close"),
      GTK_RESPONSE_CLOSE,
      /* CONTEXT: file select dialog: selects the file */
      _("Select"),
      GTK_RESPONSE_APPLY,
      NULL
      );
  selectfile->selfiledialog = dialog;

  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  uiWidgetSetAllMarginsW (content, uiBaseMarginSz * 2);

  uiCreateVertBox (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindowWU (content, &vbox);

  scwin = uiCreateScrolledWindowW (200);
  uiWidgetExpandHorizW (scwin);
  uiWidgetExpandVertW (scwin);
  uiBoxPackStartExpandUW (&vbox, scwin);

  widget = uiCreateTreeView ();
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (widget), FALSE);
  uiWidgetAlignHorizFillW (widget);
  uiWidgetAlignVertFillW (widget);
  selectfile->selfiletree = widget;

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
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", SELFILE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_title (column, "");
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (store));
  g_object_unref (store);

  g_signal_connect (widget, "row-activated",
      G_CALLBACK (selectFileSelect), selectfile);

  uiBoxPackInWindowWW (scwin, widget);

  /* the dialog doesn't have any space above the buttons */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);

  g_signal_connect (dialog, "response",
      G_CALLBACK (selectFileResponseHandler), selectfile);

  return dialog;
}

static void
selectFileSelect (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uiselectfile_t  *selectfile = udata;

  selectFileResponseHandler (GTK_DIALOG (selectfile->selfiledialog),
      GTK_RESPONSE_APPLY, selectfile);
}

static void
selectFileResponseHandler (GtkDialog *d, gint responseid, gpointer udata)
{
  uiselectfile_t  *selectfile = udata;
  gint          x, y;
  char          *str;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           count;

  uiWindowGetPositionW (GTK_WIDGET (d), &x, &y);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_X, x);
  nlistSetNum (selectfile->options, MANAGE_SELFILE_POSITION_Y, y);

  switch (responseid) {
    case GTK_RESPONSE_DELETE_EVENT: {
      selectfile->selfilecb = NULL;
      break;
    }
    case GTK_RESPONSE_CLOSE: {
      uiCloseWindowW (GTK_WIDGET (d));
      selectfile->selfilecb = NULL;
      break;
    }
    case GTK_RESPONSE_APPLY: {
      count = uiTreeViewGetSelection (selectfile->selfiletree, &model, &iter);
      if (count != 1) {
        break;
      }

      gtk_tree_model_get (model, &iter, SELFILE_COL_DISP, &str, -1);
      uiCloseWindowW (GTK_WIDGET (d));
      if (selectfile->selfilecb != NULL) {
        selectfile->selfilecb (selectfile->cbudata, str);
      }
      free (str);
      selectfile->selfilecb = NULL;
      break;
    }
  }

  selectFileFree (selectfile);
}

