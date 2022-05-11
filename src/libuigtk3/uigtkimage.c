#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "ui.h"
#include "uiutils.h"

void
uiImageNew (UIWidget *uiwidget)
{
  GtkWidget *image;

  image = gtk_image_new ();
  uiwidget->widget = image;
}

void
uiImageFromFile (UIWidget *uiwidget, const char *fn)
{
  GtkWidget *image;

  image = gtk_image_new_from_file (fn);
  uiwidget->widget = image;
}

void
uiImageClear (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_image_clear (GTK_IMAGE (uiwidget->widget));
}

void
uiImageGetPixbuf (UIWidget *uiwidget)
{
  GdkPixbuf   *pixbuf;

  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (uiwidget->widget));
  uiwidget->pixbuf = pixbuf;
}

void
uiImageSetFromPixbuf (UIWidget *uiwidget, UIWidget *uipixbuf)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }
  if (uipixbuf->pixbuf == NULL) {
    return;
  }

  gtk_image_set_from_pixbuf (GTK_IMAGE (uiwidget->widget), uipixbuf->pixbuf);
}
