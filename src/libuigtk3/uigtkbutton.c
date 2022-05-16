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
#include "pathbld.h"
#include "ui.h"
#include "uiutils.h"

static void uiButtonClickHandler (GtkButton *b, gpointer udata);

GtkWidget *
uiCreateButton (UIWidget *uiwidget, UICallback *uicb,
    char *title, char *imagenm, void *cb, void *udata)
{
  GtkWidget   *widget;

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
  if (uicb != NULL && cb != NULL) {
    fprintf (stderr, "ERR: create button: cannot have both callback set\n");
  }
  if (uicb != NULL) {
    g_signal_connect (widget, "clicked", G_CALLBACK (uiButtonClickHandler), uicb);
  }
  if (cb != NULL) {
    g_signal_connect (widget, "clicked", G_CALLBACK (cb), udata);
  }

  if (uiwidget != NULL) {
    uiwidget->widget = widget;
  }
  return widget;
}

void
uiButtonSetImage (UIWidget *uiwidget, const char *imagenm, const char *tooltip)
{
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  gtk_button_set_label (GTK_BUTTON (uiwidget->widget), "");
  pathbldMakePath (tbuff, sizeof (tbuff), imagenm, ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (uiwidget->widget), image);
  /* macos needs this */
  gtk_button_set_always_show_image (GTK_BUTTON (uiwidget->widget), TRUE);
  if (tooltip != NULL) {
    gtk_widget_set_tooltip_text (uiwidget->widget, tooltip);
  }
}

void
uiButtonSetImageIcon (UIWidget *uiwidget, const char *nm)
{
  GtkWidget *image;

  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (uiwidget->widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (uiwidget->widget), TRUE);
}

void
uiButtonAlignLeft (UIWidget *uiwidget)
{
  GtkWidget *widget;

  widget = gtk_bin_get_child (GTK_BIN (uiwidget->widget));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
}

inline static void
uiButtonClickHandler (GtkButton *b, gpointer udata)
{
  UICallback *uicb = udata;

  uiutilsCallbackHandler (uicb);
}

