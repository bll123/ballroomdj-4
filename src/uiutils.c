#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "pathbld.h"
#include "sysvars.h"
#include "uiutils.h"

static char **cssdata = NULL;
static int  csscount = 0;

static GLogWriterOutput uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

/* drop-down/combobox handling */
static void     uiutilsDropDownWindowShow (GtkButton *b, gpointer udata);
static gboolean uiutilsDropDownClose (GtkWidget *w,
    GdkEventFocus *event, gpointer udata);
static GtkWidget * uiutilsDropDownButtonCreate (char *title,
    uiutilsdropdown_t *dropdown);
static void uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata);
static void uiutilsDropDownSelectionSet (uiutilsdropdown_t *dropdown,
    ssize_t internalidx);

static gint uiutilsSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gboolean uiutilsSpinboxDisplay (GtkSpinButton *sb, gpointer udata);

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
uiutilsInitGtkLog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

GtkWidget *
uiutilsCreateLabel (char *label)
{
  GtkWidget *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateLabel");

  widget = gtk_label_new (label);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  logProcEnd (LOG_PROC, "uiutilsCreateLabel", "");
  return widget;
}

GtkWidget *
uiutilsCreateColonLabel (char *label)
{
  GtkWidget *widget;
  char      tbuff [100];

  logProcBegin (LOG_PROC, "uiutilsCreateColonLabel");

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  widget = gtk_label_new (tbuff);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  logProcEnd (LOG_PROC, "uiutilsCreateColonLabel", "");
  return widget;
}

GtkWidget *
uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateButton");

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  if (imagenm != NULL) {
    GtkWidget   *image;
    char        tbuff [MAXPATHLEN];

    gtk_button_set_label (GTK_BUTTON (widget), "");
    pathbldMakePath (tbuff, sizeof (tbuff), "", imagenm, ".svg",
        PATHBLD_MP_IMGDIR);
    image = gtk_image_new_from_file (tbuff);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_tooltip_text (widget, title);
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }
  if (clickCallback != NULL) {
    g_signal_connect (widget, "clicked", G_CALLBACK (clickCallback), udata);
  }

  logProcEnd (LOG_PROC, "uiutilsCreateButton", "");
  return widget;
}

GtkWidget *
uiutilsCreateScrolledWindow (void)
{
  GtkWidget   *widget;
  GtkWidget   *twidget;

  logProcBegin (LOG_PROC, "uiutilsCreateScrolledWindow");

  widget = gtk_scrolled_window_new (NULL, NULL);
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

void
uiutilsDropDownInit (uiutilsdropdown_t *dropdown)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownInit");
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  dropdown->window = NULL;
  dropdown->tree = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->strIndexMap = NULL;
  dropdown->numIndexMap = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
  logProcEnd (LOG_PROC, "uiutilsDropDownInit", "");
}


void
uiutilsDropDownFree (uiutilsdropdown_t *dropdown)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownFree");
  if (dropdown != NULL) {
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    if (dropdown->strIndexMap != NULL) {
      slistFree (dropdown->strIndexMap);
    }
    if (dropdown->numIndexMap != NULL) {
      nlistFree (dropdown->numIndexMap);
    }
  }
  logProcEnd (LOG_PROC, "uiutilsDropDownFree", "");
}

GtkWidget *
uiutilsDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownCreate");
  dropdown->parentwin = parentwin;
  dropdown->button = uiutilsDropDownButtonCreate (title, dropdown);
  uiutilsDropDownWindowCreate (dropdown, processSelectionCallback, udata);
  logProcEnd (LOG_PROC, "uiutilsDropDownCreate", "");
  return dropdown->button;
}

GtkWidget *
uiutilsComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata)
{
  dropdown->iscombobox = true;
  return uiutilsDropDownCreate (parentwin,
      title, processSelectionCallback,
      dropdown, udata);
}

void
uiutilsCreateDanceList (uiutilsdropdown_t *dropdown, char *selectLabel)
{
  dance_t           *dances;
  slist_t           *danceList;

  logProcBegin (LOG_PROC, "uiutilsCreateDanceList");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  uiutilsDropDownSetNumList (dropdown, danceList, selectLabel);
  logProcEnd (LOG_PROC, "uiutilsCreateDanceList", "");
}

ssize_t
uiutilsDropDownSelectionGet (uiutilsdropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  unsigned long idx = 0;
  int32_t       idx32;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionGet");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    /* despite the model using an unsigned long, setting it to -1 */
    /* sets it to a 32-bit value */
    /* want the special -1 index (all items for combobox) */
    idx32 = (int32_t) idx;
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    if (dropdown->iscombobox) {
      char  *p;

      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      gtk_button_set_label (GTK_BUTTON (dropdown->button), p);
      free (p);
    }
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  } else {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionGet", "no-iter");
    return -1;
  }

  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionGet", "");
  return (ssize_t) idx32;
}

void
uiutilsDropDownSetList (uiutilsdropdown_t *dropdown, slist_t *list,
    char *selectLabel)
{
  char              *strval;
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  ssize_t            internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSetList");

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s    ",
        slistGetMaxKeyWidth (list), selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff, -1);
    slistSetNum (dropdown->strIndexMap, "", internalidx++);
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    strval = slistGetStr (list, dispval);
    slistSetNum (dropdown->strIndexMap, strval, internalidx);

    gtk_list_store_append (store, &iter);
    /* gtk does not leave room for the scrollbar */
    snprintf (tbuff, sizeof (tbuff), "%-*s    ",
        slistGetMaxKeyWidth (list), dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, internalidx,
        UIUTILS_DROPDOWN_COL_STR, strval,
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        -1);
    ++internalidx;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "uiutilsDropDownSetList", "");
}

void
uiutilsDropDownSetNumList (uiutilsdropdown_t *dropdown, slist_t *list,
    char *selectLabel)
{
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  ssize_t            internalidx;
  nlistidx_t        idx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSetNumList");

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->numIndexMap = nlistAlloc ("uiutils-num-index", LIST_ORDERED, NULL);
  internalidx = 0;

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s    ",
        slistGetMaxKeyWidth (list), selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff, -1);
    nlistSetNum (dropdown->numIndexMap, -1, internalidx++);
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    idx = slistGetNum (list, dispval);
    nlistSetNum (dropdown->numIndexMap, idx, internalidx);

    gtk_list_store_append (store, &iter);
    /* gtk does not leave room for the scrollbar */
    snprintf (tbuff, sizeof (tbuff), "%-*s    ",
        slistGetMaxKeyWidth (list), dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, idx,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        -1);
    ++internalidx;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "uiutilsDropDownSetNumList", "");
}

void
uiutilsDropDownSelectionSetNum (uiutilsdropdown_t *dropdown, ssize_t idx)
{
  ssize_t        internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSetNum");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetNum", "null-dropdown");
    return;
  }

  if (dropdown->numIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = nlistGetNum (dropdown->numIndexMap, idx);
  }
  uiutilsDropDownSelectionSet (dropdown, internalidx);
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetNum", "");
}

void
uiutilsDropDownSelectionSetStr (uiutilsdropdown_t *dropdown, char *stridx)
{
  ssize_t        internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSetStr");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetStr", "null-dropdown");
    return;
  }

  if (dropdown->strIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = slistGetNum (dropdown->strIndexMap, stridx);
  }
  uiutilsDropDownSelectionSet (dropdown, internalidx);
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetStr", "");
}

void
uiutilsEntryInit (uiutilsentry_t *entry, int entrySize, int maxSize)
{
  logProcBegin (LOG_PROC, "uiutilsEntryInit");
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->buffer = NULL;
  entry->entry = NULL;
  logProcEnd (LOG_PROC, "uiutilsEntryInit", "");
}


void
uiutilsEntryFree (uiutilsentry_t *entry)
{
  ;
}

GtkWidget *
uiutilsEntryCreate (uiutilsentry_t *entry)
{
  logProcBegin (LOG_PROC, "uiutilsEntryCreate");
  entry->buffer = gtk_entry_buffer_new (NULL, -1);
  entry->entry = gtk_entry_new_with_buffer (entry->buffer);
  gtk_entry_set_width_chars (GTK_ENTRY (entry->entry), entry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (entry->entry), entry->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry->entry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (entry->entry, 2);
  gtk_widget_set_margin_start (entry->entry, 2);
  gtk_widget_set_halign (entry->entry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (entry->entry, TRUE);

  logProcEnd (LOG_PROC, "uiutilsEntryCreate", "");
  return entry->entry;
}

const char *
uiutilsEntryGetValue (uiutilsentry_t *entry)
{
  const char  *value;

  value = gtk_entry_buffer_get_text (entry->buffer);
  return value;
}

void
uiutilsEntrySetValue (uiutilsentry_t *entry, char *value)
{
  gtk_entry_buffer_set_text (entry->buffer, value, -1);
}

void
uiutilsSpinboxInit (uiutilsspinbox_t *spinbox)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxInit");
  spinbox->spinbox = NULL;
  spinbox->curridx = 0;
  spinbox->textGetProc = NULL;
  spinbox->udata = NULL;
  spinbox->indisp = false;
  spinbox->maxWidth = 0;
  logProcEnd (LOG_PROC, "uiutilsSpinboxInit", "");
}


void
uiutilsSpinboxFree (uiutilsspinbox_t *spinbox)
{
  ;
}


GtkWidget *
uiutilsSpinboxTextCreate (uiutilsspinbox_t *spinbox, void *udata)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxTextCreate");
  spinbox->spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->spinbox), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->spinbox), TRUE);
  gtk_widget_set_margin_top (spinbox->spinbox, 2);
  gtk_widget_set_margin_start (spinbox->spinbox, 2);
  g_signal_connect (spinbox->spinbox, "output",
      G_CALLBACK (uiutilsSpinboxDisplay), spinbox);
  g_signal_connect (spinbox->spinbox, "input",
      G_CALLBACK (uiutilsSpinboxInput), spinbox);
  spinbox->udata = udata;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTextCreate", "");
  return spinbox->spinbox;
}


void
uiutilsSpinboxSet (uiutilsspinbox_t *spinbox, int min, int count,
    int maxWidth, uiutilsspinboxdisp_t textGetProc)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxSet");
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->spinbox),
      (double) min, (double) count - 1);
  /* will width in characters be enough for some glyphs? */
  /* certainly not if languages are mixed */
  spinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (spinbox->spinbox), spinbox->maxWidth);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->spinbox), (double) min);
  spinbox->textGetProc = textGetProc;
  logProcEnd (LOG_PROC, "uiutilsSpinboxSet", "");
}


int
uiutilsSpinboxGetValue (uiutilsspinbox_t *spinbox)
{
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiutilsSpinboxGetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxGetValue", "spinbox-null");
    return -1;
  }
  if (spinbox->spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxGetValue", "spinbox-widget-null");
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  logProcEnd (LOG_PROC, "uiutilsSpinboxGetValue", "");
  return (int) value;
}

void
uiutilsSpinboxSetValue (uiutilsspinbox_t *spinbox, int value)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "uiutilsSpinboxSetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxSetValue", "spinbox-null");
    return;
  }
  if (spinbox->spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxSetValue", "spinbox-widget-null");
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  gtk_adjustment_set_value (adjustment, (double) value);
  logProcEnd (LOG_PROC, "uiutilsSpinboxSetValue", "");
}

/* gtk spinboxes are a bit bizarre */
static gint
uiutilsSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uiutilsspinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiutilsSpinboxInput");

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  logProcEnd (LOG_PROC, "uiutilsSpinboxInput", "");
  return TRUE;
}

static gboolean
uiutilsSpinboxDisplay (GtkSpinButton *sb, gpointer udata)
{
  uiutilsspinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  char              *disp;
  double            value;
  char              tbuff [100];

  logProcBegin (LOG_PROC, "uiutilsSpinboxDisplay");

  if (spinbox->indisp) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxDisplay", "lock-in-disp");
    return FALSE;
  }
  spinbox->indisp = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->spinbox), value);
  spinbox->curridx = (int) value;
  disp = "";
  if (spinbox->textGetProc != NULL) {
    disp = spinbox->textGetProc (spinbox->udata, spinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", spinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (spinbox->spinbox), tbuff);
  spinbox->indisp = false;
  logProcEnd (LOG_PROC, "uiutilsSpinboxDisplay", "");
  return TRUE;
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

static void
uiutilsDropDownWindowShow (GtkButton *b, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;
  GtkAllocation     alloc;
  gint              x, y;

  logProcBegin (LOG_PROC, "uiutilsDropDownWindowShow");

  gtk_window_get_position (GTK_WINDOW (dropdown->parentwin), &x, &y);
  gtk_widget_get_allocation (dropdown->button, &alloc);
  gtk_widget_show_all (dropdown->window);
  gtk_window_move (GTK_WINDOW (dropdown->window), alloc.x + x + 4, alloc.y + y + 4 + 30);
  dropdown->open = true;
  logProcEnd (LOG_PROC, "uiutilsDropDownWindowShow", "");
}

static gboolean
uiutilsDropDownClose (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;

  logProcBegin (LOG_PROC, "uiutilsDropDownClose");

  if (dropdown->open) {
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  }
  gtk_window_present (GTK_WINDOW (dropdown->parentwin));

  logProcEnd (LOG_PROC, "uiutilsDropDownClose", "");
  return FALSE;
}

static GtkWidget *
uiutilsDropDownButtonCreate (char *title, uiutilsdropdown_t *dropdown)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uiutilsDropDownButtonCreate");

  widget = gtk_button_new ();
  assert (widget != NULL);
  strlcpy (tbuff, title, MAXPATHLEN);
  if (dropdown->iscombobox) {
    snprintf (tbuff, sizeof (tbuff), "- %s  ", title);
  }
  gtk_button_set_label (GTK_BUTTON (widget), tbuff);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);

  g_signal_connect (widget, "clicked",
      G_CALLBACK (uiutilsDropDownWindowShow), dropdown);

  logProcEnd (LOG_PROC, "uiutilsDropDownButtonCreate", "");
  return widget;
}


static void
uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata)
{
  GtkWidget         *scwin;
  GtkWidget         *twidget;
  GtkTreeSelection  *sel;

  logProcBegin (LOG_PROC, "uiutilsDropDownWindowCreate");

  dropdown->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (dropdown->window), dropdown->button);
  gtk_window_set_transient_for (GTK_WINDOW (dropdown->window),
      GTK_WINDOW (dropdown->parentwin));
  gtk_window_set_decorated (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (dropdown->window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dropdown->window), 4);
  gtk_widget_hide (dropdown->window);
  gtk_widget_set_events (dropdown->window, GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (dropdown->window),
      "focus-out-event", G_CALLBACK (uiutilsDropDownClose), dropdown);

  scwin = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scwin), 300);
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scwin), 400);
  gtk_widget_set_hexpand (scwin, TRUE);
//  gtk_widget_set_vexpand (scwin, TRUE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scwin));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  gtk_container_add (GTK_CONTAINER (dropdown->window), scwin);

  dropdown->tree = gtk_tree_view_new ();
  assert (dropdown->tree != NULL);
  g_object_ref_sink (G_OBJECT (dropdown->tree));
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (dropdown->tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dropdown->tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dropdown->tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (dropdown->tree, TRUE);
  gtk_widget_set_vexpand (dropdown->tree, TRUE);
  gtk_container_add (GTK_CONTAINER (scwin), dropdown->tree);
  g_signal_connect (dropdown->tree, "row-activated",
      G_CALLBACK (processSelectionCallback), udata);
  logProcEnd (LOG_PROC, "uiutilsDropDownWindowCreate", "");
}

static void
uiutilsDropDownSelectionSet (uiutilsdropdown_t *dropdown, ssize_t internalidx)
{
  GtkTreePath   *path;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  char          tbuff [40];
  char          *p;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSet");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "null-dropdown");
    return;
  }
  if (dropdown->tree == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "null-tree");
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%zd", internalidx);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL) {
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (dropdown->tree), path, NULL, FALSE);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
    if (model != NULL) {
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      if (p != NULL) {
        gtk_button_set_label (GTK_BUTTON (dropdown->button), p);
      }
    }
  }
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "");
}

