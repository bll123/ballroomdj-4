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
#include "bdjvarsdf.h"
#include "dance.h"
#include "manageui.h"
#include "playlist.h"
#include "ui.h"
#include "uiutils.h"

enum {
  MPLTREE_COL_DANCE_SELECT,
  MPLTREE_COL_DANCE,
  MPLTREE_COL_MAXPLAYTIME,
  MPLTREE_COL_LOWBPM,
  MPLTREE_COL_HIGHBPM,
  MPLTREE_COL_SB_PAD,
  MPLTREE_COL_DANCE_IDX,
  MPLTREE_COL_EDITABLE,
  MPLTREE_COL_ADJUST,
  MPLTREE_COL_DIGITS,
  MPLTREE_COL_MAX,
};

typedef struct managepltree {
  GtkWidget       *tree;
} managepltree_t;

static void managePlaylistTreePopulate (managepltree_t *managepltree);

managepltree_t *
managePlaylistTreeAlloc (void)
{
  managepltree_t *managepltree;

  managepltree = malloc (sizeof (managepltree_t));
  managepltree->tree = NULL;
  return managepltree;
}

void
managePlaylistTreeFree (managepltree_t *managepltree)
{
  if (managepltree != NULL) {
    free (managepltree);
  }
}

void
manageBuildUIPlaylistTree (managepltree_t *managepltree, UIWidget *vboxp)
{
  UIWidget    uiwidget;
  GtkWidget   *tree;
  GtkCellRenderer *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (vboxp, &uiwidget);

  tree = uiCreateTreeView ();
  uiWidgetExpandVertW (tree);
  uiBoxPackInWindowUW (&uiwidget, tree);
  managepltree->tree = tree;

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", MPLTREE_COL_DANCE_SELECT,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MPLTREE_COL_DANCE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: playlist management: dance column header */
  gtk_tree_view_column_set_title (column, _("Dance"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MPLTREE_COL_MAXPLAYTIME,
      "editable", MPLTREE_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: playlist management: max play time column header (keep short) */
  gtk_tree_view_column_set_title (column, _("Max Play Time"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "mpltreecolumn",
      GUINT_TO_POINTER (MPLTREE_COL_LOWBPM));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MPLTREE_COL_LOWBPM,
      "editable", MPLTREE_COL_EDITABLE,
      "adjustment", MPLTREE_COL_ADJUST,
      "digits", MPLTREE_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: playlist management: low bpm/mpm column header */
  gtk_tree_view_column_set_title (column, _("Low BPM"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "mpltreecolumn",
      GUINT_TO_POINTER (MPLTREE_COL_LOWBPM));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MPLTREE_COL_HIGHBPM,
      "editable", MPLTREE_COL_EDITABLE,
      "adjustment", MPLTREE_COL_ADJUST,
      "digits", MPLTREE_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: playlist management: high bpm/mpm column */
  gtk_tree_view_column_set_title (column, _("High BPM"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", MPLTREE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  managePlaylistTreePopulate (managepltree);
}

static void
managePlaylistTreePopulate (managepltree_t *managepltree)
{
  dance_t       *dances;
  GtkTreeIter   iter;
  slist_t       *dancelist;
  slistidx_t    iteridx;
  ilistidx_t    key;
  GtkWidget   *tree;
  GtkListStore  *store;

  tree = managepltree->tree;

  store = gtk_list_store_new (MPLTREE_COL_MAX,
      G_TYPE_BOOLEAN, // dance select
      G_TYPE_STRING,  // dance
      G_TYPE_STRING,  // max play time
      G_TYPE_STRING,  // low bpm
      G_TYPE_STRING,  // high bpm
      G_TYPE_STRING,  // pad
      G_TYPE_LONG,    // dance idx
      G_TYPE_LONG,    // editable
      G_TYPE_OBJECT,  // adjust
      G_TYPE_LONG);   // digits

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);

  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        MPLTREE_COL_DANCE_SELECT, 0,
        MPLTREE_COL_DANCE, dancedisp,
        MPLTREE_COL_MAXPLAYTIME, "",
        MPLTREE_COL_LOWBPM, 0,
        MPLTREE_COL_HIGHBPM, 0,
        MPLTREE_COL_SB_PAD, "  ",
        MPLTREE_COL_DANCE_IDX, key,
        MPLTREE_COL_EDITABLE, 1,
        MPLTREE_COL_ADJUST, NULL,
        MPLTREE_COL_DIGITS, 0,
        -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

