/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
  uiwidget->wbasetype = WCONT_T_IMAGE;
  uiwidget->wtype = WCONT_T_IMAGE;
  uiwidget->uidata.widget = image;
  uiwidget->uidata.packwidget = image;
  return uiwidget;
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t *uiwidget = NULL;
  GtkWidget *image;
  GdkPixbuf *pixbuf;

  /* using gtk_image_new_from_file creates memory leaks. */
  pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
  if (pixbuf != NULL) {
    image = gtk_image_new_from_pixbuf (pixbuf);
    uiwidget = uiwcontAlloc ();
    uiwidget->wbasetype = WCONT_T_IMAGE;
    uiwidget->wtype = WCONT_T_IMAGE;
    uiwidget->uidata.widget = image;
    uiwidget->uidata.packwidget = image;
    g_object_unref (pixbuf);
  }
  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  uiwcont_t *uiwidget = NULL;
  GdkPixbuf *pixbuf;
  GtkWidget *image = NULL;

  pixbuf = gdk_pixbuf_new_from_file_at_scale (fn, scale, -1, TRUE, NULL);
  if (pixbuf != NULL) {
    image = gtk_image_new ();
    gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
    uiwidget = uiwcontAlloc ();
    uiwidget->wbasetype = WCONT_T_IMAGE;
    uiwidget->wtype = WCONT_T_IMAGE;
    uiwidget->uidata.widget = image;
    uiwidget->uidata.packwidget = image;
    g_object_unref (pixbuf);
  }
  return uiwidget;
}

void
uiImageClear (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-clr")) {
    return;
  }

  if (GTK_IS_IMAGE (uiwidget->uidata.widget)) {
    gtk_image_clear (GTK_IMAGE (uiwidget->uidata.widget));
  }
}

void
uiImageConvertToPixbuf (uiwcont_t *uiwidget)
{
  GdkPixbuf   *pixbuf;

  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-cvt-pixbuf")) {
    return;
  }

  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (uiwidget->uidata.widget));
  uiwidget->wbasetype = WCONT_T_PIXBUF;
  uiwidget->wtype = WCONT_T_PIXBUF;
  uiwidget->uidata.pixbuf = pixbuf;
}

void *
uiImageGetPixbuf (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_PIXBUF, "image-get-pixbuf")) {
    return NULL;
  }
  if (uiwidget->uidata.pixbuf == NULL) {
    return NULL;
  }

  return uiwidget->uidata.pixbuf;
}

void
uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf)
{
  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-set-from-pixbuf-img")) {
    return;
  }
  if (! uiwcontValid (uipixbuf, WCONT_T_PIXBUF, "image-set-from-pixbuf-pixbuf")) {
    return;
  }

  gtk_image_set_from_pixbuf (GTK_IMAGE (uiwidget->uidata.widget), uipixbuf->uidata.pixbuf);
}
