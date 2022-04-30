#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "tagdef.h"
#include "uiutils.h"

static valuetype_t uiutilsDetermineValueType (int tagidx);
static char  * uiutilsMakeDisplayStr (song_t *song, int tagidx, int *allocflag);

GtkWidget *
uiutilsCreateTreeView (void)
{
  GtkWidget         *tree;
  GtkTreeSelection  *sel;

  tree = gtk_tree_view_new ();
  gtk_widget_set_margin_start (tree, 4);
  gtk_widget_set_margin_end (tree, 4);
  gtk_widget_set_margin_top (tree, 4);
  gtk_widget_set_margin_bottom (tree, 4);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_halign (tree, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tree, FALSE);
  gtk_widget_set_vexpand (tree, FALSE);
  return tree;
}

GType *
uiutilsAppendType (GType *types, int *ncol, int type)
{
  ++(*ncol);
  types = realloc (types, *ncol * sizeof (GType));
  types [*ncol-1] = type;

  return types;
}

GtkTreeViewColumn *
uiutilsAddDisplayColumns (GtkWidget *tree, slist_t *sellist, int col,
    int fontcol, int ellipsizeCol)
{
  slistidx_t  seliteridx;
  int         tagidx;
  GtkCellRenderer       *renderer = NULL;
  GtkTreeViewColumn     *column = NULL;
  GtkTreeViewColumn     *favColumn = NULL;


  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    renderer = gtk_cell_renderer_text_new ();
    if (tagidx == TAG_FAVORITE) {
      /* use the normal UI font here */
      column = gtk_tree_view_column_new_with_attributes ("", renderer,
          "markup", col,
          NULL);
      gtk_cell_renderer_set_alignment (renderer, 0.5, 0.5);
      favColumn = column;
    } else {
      column = gtk_tree_view_column_new_with_attributes ("", renderer,
          "text", col,
          "font", fontcol,
          NULL);
    }
    if (tagdefs [tagidx].alignRight) {
      gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
    }
    if (tagdefs [tagidx].ellipsize) {
      if (tagidx == TAG_TITLE) {
        gtk_tree_view_column_set_min_width (column, 200);
      }
      gtk_tree_view_column_add_attribute (column, renderer,
          "ellipsize", ellipsizeCol);
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      gtk_tree_view_column_set_expand (column, TRUE);
    } else {
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    }
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    if (tagidx == TAG_FAVORITE) {
      gtk_tree_view_column_set_title (column, "\xE2\x98\x86");
    } else {
      gtk_tree_view_column_set_title (column, tagdefs [tagidx].displayname);
    }
    col++;
  }

  return favColumn;
}

void
uiutilsSetDisplayColumns (GtkListStore *store, GtkTreeIter *iter,
    slist_t *sellist, song_t *song, int col)
{
  slistidx_t    seliteridx;
  int           tagidx;


  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    valuetype_t vt;
    char        *str;
    gulong      num;
    ssize_t     val;
    int         allocated = 0;

    vt = uiutilsDetermineValueType (tagidx);
    if (vt == VALUE_STR) {
      songfavoriteinfo_t  * favorite;

      if (tagidx == TAG_FAVORITE) {
        favorite = songGetFavoriteData (song);
        str = favorite->spanStr;
      } else {
        str = uiutilsMakeDisplayStr (song, tagidx, &allocated);
        if (allocated) {
          free (str);
        }
      }
      gtk_list_store_set (store, iter, col++, str, -1);
    } else {
      num = songGetNum (song, tagidx);
      val = (ssize_t) num;
      if (val != LIST_VALUE_INVALID) {
        gtk_list_store_set (store, iter, col++, num, -1);
      } else {
        gtk_list_store_set (store, iter, col++, 0, -1);
      }
    }
  }
}


GType *
uiutilsAddDisplayTypes (GType *types, slist_t *sellist, int *col)
{
  slistidx_t    seliteridx;
  int           tagidx;

  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    valuetype_t vt;
    int         type;

    vt = uiutilsDetermineValueType (tagidx);
    if (vt == VALUE_NUM) {
      type = G_TYPE_ULONG;
    }
    if (vt == VALUE_STR) {
      type = G_TYPE_STRING;
    }
    types = uiutilsAppendType (types, col, type);
  }

  return types;
}

/* internal routines */

static valuetype_t
uiutilsDetermineValueType (int tagidx)
{
  valuetype_t   vt;

  vt = tagdefs [tagidx].valueType;
  if (tagdefs [tagidx].convfunc != NULL) {
    vt = VALUE_STR;
  }

  return vt;
}


static char *
uiutilsMakeDisplayStr (song_t *song, int tagidx, int *allocated)
{
  valuetype_t     vt;
  dfConvFunc_t    convfunc;
  datafileconv_t  conv;
  char            *str;

  *allocated = false;
  vt = tagdefs [tagidx].valueType;
  convfunc = tagdefs [tagidx].convfunc;
  if (convfunc != NULL) {
    conv.allocated = false;
    if (vt == VALUE_NUM) {
      conv.u.num = songGetNum (song, tagidx);
    } else if (vt == VALUE_LIST) {
      conv.u.list = songGetList (song, tagidx);
    }
    conv.valuetype = vt;
    convfunc (&conv);
    str = conv.u.str;
    *allocated = conv.allocated;
  } else {
    str = songGetStr (song, tagidx);
  }

  return str;
}

