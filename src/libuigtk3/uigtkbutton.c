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
#include "log.h"
#include "pathbld.h"
#include "ui.h"
#include "uiutils.h"

static void uiButtonClickHandler (GtkButton *b, gpointer udata);

GtkWidget *
uiCreateButton (UIWidget *uiwidget, char *title, char *imagenm,
    void *cb, void *udata)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiCreateButton");

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  if (imagenm != NULL) {
    GtkWidget   *image;
    char        tbuff [MAXPATHLEN];

    gtk_button_set_label (GTK_BUTTON (widget), "");
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, ".svg",
        PATHBLD_MP_IMGDIR);
    image = gtk_image_new_from_file (tbuff);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_tooltip_text (widget, title);
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }
  if (cb != NULL) {
    if (uiwidget != NULL) {
      uiwidget->widget = widget;
      uiwidget->cb.cb = cb;
      uiwidget->cb.udata = udata;
      g_signal_connect (widget, "clicked", G_CALLBACK (uiButtonClickHandler), uiwidget);
    } else {
      g_signal_connect (widget, "clicked", G_CALLBACK (cb), udata);
    }
  }

  logProcEnd (LOG_PROC, "uiCreateButton", "");
  return widget;
}

void
uiButtonAlignLeft (GtkWidget *widget)
{
  widget = gtk_bin_get_child (GTK_BIN (widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
}

static void
uiButtonClickHandler (GtkButton *b, gpointer udata)
{
  UIWidget  *uiwidget = udata;

  uiwidget->cb.cb (uiwidget, uiwidget->cb.udata);
}

