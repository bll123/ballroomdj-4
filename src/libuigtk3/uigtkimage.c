/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"

uiwcont_t *
uiImageNew (void)
{
  uiwcont_t *uiwidget;
  GtkWidget *image;

  image = gtk_image_new ();
  uiwidget = uiwcontAlloc ();
  uiwidget->widget = image;
  return uiwidget;
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t *uiwidget;
  GtkWidget *image;

  image = gtk_image_new_from_file (fn);
  uiwidget = uiwcontAlloc ();
  uiwidget->widget = image;
  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  uiwcont_t *uiwidget;
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  GError    *gerr = NULL;

  pixbuf = gdk_pixbuf_new_from_file_at_scale (fn, scale, -1, TRUE, &gerr);
  image = gtk_image_new ();
  gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
  uiwidget = uiwcontAlloc ();
  uiwidget->widget = image;
  return uiwidget;
}

void
uiImageClear (uiwcont_t *uiwidget)
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
uiImageConvertToPixbuf (uiwcont_t *uiwidget)
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

void *
uiImageGetPixbuf (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return NULL;
  }
  if (uiwidget->pixbuf == NULL) {
    return NULL;
  }

  return uiwidget->pixbuf;
}

void
uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf)
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
