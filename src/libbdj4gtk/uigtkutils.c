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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "fileop.h"
#include "ilist.h"
#include "localeutil.h"
#include "log.h"
#include "pathbld.h"
#include "pathutil.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "uiutils.h"

static char **cssdata = NULL;
static int  csscount = 0;

static GLogWriterOutput uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

static valuetype_t uiutilsDetermineValueType (int tagidx);
static char  * uiutilsMakeDisplayStr (song_t *song, int tagidx, int *allocflag);

void
uiutilsCleanup (void)
{
  if (cssdata != NULL) {
    for (int i = 0; i < csscount; ++i) {
      if (cssdata [i] != NULL) {
        free (cssdata [i]);
      }
    }
    free (cssdata);
    csscount = 0;
    cssdata = NULL;
  }
}

void
uiutilsSetCss (GtkWidget *w, char *style)
{
  GtkCssProvider  *tcss;
  char            *tstyle;

  logProcBegin (LOG_PROC, "uiutilsSetCss");

  tcss = gtk_css_provider_new ();
  tstyle = strdup (style);
  ++csscount;
  cssdata = realloc (cssdata, sizeof (char *) * csscount);
  cssdata [csscount-1] = tstyle;

  gtk_css_provider_load_from_data (tcss, tstyle, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (w),
      GTK_STYLE_PROVIDER (tcss),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  logProcEnd (LOG_PROC, "uiutilsSetCss", "");
}

void
uiutilsSetUIFont (char *uifont)
{
  GtkCssProvider  *tcss;
  GdkScreen       *screen;
  char            tbuff [MAXPATHLEN];
  char            wbuff [MAXPATHLEN];
  char            *p;
  int             sz = 0;

  logProcBegin (LOG_PROC, "uiutilsSetUIFont");

  strlcpy (wbuff, uifont, MAXPATHLEN);
  if (uifont != NULL && *uifont) {
    p = strrchr (wbuff, ' ');
    if (p != NULL) {
      ++p;
      if (isdigit (*p)) {
        --p;
        *p = '\0';
        ++p;
        sz = atoi (p);
      }
    }

    tcss = gtk_css_provider_new ();
    snprintf (tbuff, sizeof (tbuff), "* { font-family: '%s'; }", wbuff);
    if (sz > 0) {
      snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; }", sz);
      sz -= 2;
      snprintf (wbuff, sizeof (wbuff), " menuitem label { font-size: %dpt; }", sz);
      strlcat (tbuff, wbuff, MAXPATHLEN);
    }

    p = strdup (tbuff);
    ++csscount;
    cssdata = realloc (cssdata, sizeof (char *) * csscount);
    cssdata [csscount-1] = p;

    gtk_css_provider_load_from_data (tcss, p, -1, NULL);
    screen = gdk_screen_get_default ();
    if (screen != NULL) {
      gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (tcss),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
  logProcEnd (LOG_PROC, "uiutilsSetUIFont", "");
}

void
uiutilsInitUILog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

int
uiutilsCreateApplication (int argc, char *argv [],
    char *tag, GtkApplication **app, void *activateFunc, void *udata)
{
  int             status;
  char            tbuff [80];

  logProcBegin (LOG_PROC, "uiutilsCreateApplication");

  snprintf (tbuff, sizeof (tbuff), "org.bdj4.BDJ4.%s", tag);
  *app = gtk_application_new (tbuff, G_APPLICATION_NON_UNIQUE);

  g_signal_connect (*app, "activate", G_CALLBACK (activateFunc), udata);

  /* gtk messes up the locale setting somehow; a re-bind is necessary */
  localeInit ();

  status = g_application_run (G_APPLICATION (*app), argc, argv);

  logProcEnd (LOG_PROC, "uiutilsCreateApplication", "");
  return status;
}

GtkWidget *
uiutilsCreateScrolledWindow (void)
{
  GtkWidget   *widget;
  GtkWidget   *twidget;

  logProcBegin (LOG_PROC, "uiutilsCreateScrolledWindow");

  widget = gtk_scrolled_window_new (NULL, NULL);
  uiutilsSetCss (widget,
      "undershoot.top, undershoot.right, "
      "undershoot.left, undershoot.bottom "
      "{ background-image: none; outline-width: 0; }");
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 400);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");

  logProcEnd (LOG_PROC, "uiutilsCreateScrolledWindow", "");
  return widget;
}

GtkWidget *
uiutilsCreateSwitch (int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateSwitch");

  widget = gtk_switch_new ();
  assert (widget != NULL);
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
  logProcEnd (LOG_PROC, "uiutilsCreateSwitch", "");
  return widget;
}

GtkWidget *
uiutilsCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateCheckButton");

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  logProcEnd (LOG_PROC, "uiutilsCreateCheckButton", "");
  return widget;
}

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

void
uiutilsGetForegroundColor (GtkWidget *widget, char *buff, size_t sz)
{
  GdkRGBA         gcolor;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &gcolor);
  snprintf (buff, sz, "#%02x%02x%02x",
      (int) round (gcolor.red * 255.0),
      (int) round (gcolor.green * 255.0),
      (int) round (gcolor.blue * 255.0));
}

char *
uiutilsSelectDirDialog (uiutilsselect_t *selectdata)
{
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      /* CONTEXT: actions associated with the select folder dialog */
      _("Select"), _("Close"));
  if (selectdata->startpath != NULL) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
        selectdata->startpath);
  }

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  }

  g_object_unref (widget);
  return fn;
}

char *
uiutilsSelectFileDialog (uiutilsselect_t *selectdata)
{
  GtkFileChooserNative  *widget = NULL;
  gint                  res;
  char                  *fn = NULL;

  widget = gtk_file_chooser_native_new (
      selectdata->label,
      GTK_WINDOW (selectdata->window),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      /* CONTEXT: actions associated with the select file dialog */
      _("Select"), _("Close"));
  if (selectdata->startpath != NULL) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
        selectdata->startpath);
  }
  if (selectdata->mimetype != NULL) {
    GtkFileFilter   *ff;

    ff = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (ff, selectdata->mimetype);
    if (selectdata->mimefiltername != NULL) {
      gtk_file_filter_set_name (ff, selectdata->mimefiltername);
    }
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (widget), ff);
  }

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (widget));
  if (res == GTK_RESPONSE_ACCEPT) {
    fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  }

  g_object_unref (widget);
  return fn;
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

void
uiutilsWidgetSetAllMargins (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_top (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
  gtk_widget_set_margin_start (widget, margin);
  gtk_widget_set_margin_end (widget, margin);
}

/* internal routines */

static GLogWriterOutput
uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields,
    gsize n_fields,
    gpointer udata)
{
  char      *msg;

  if (logLevel != G_LOG_LEVEL_DEBUG) {
    msg = g_log_writer_format_fields (logLevel, fields, n_fields, FALSE);
    logMsg (LOG_GTK, LOG_IMPORTANT, msg);
    if (strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "alpha") == 0) {
      fprintf (stderr, "%s\n", msg);
    }
  }

  return G_LOG_WRITER_HANDLED;
}

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

