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
#include "uiutils.h"

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

