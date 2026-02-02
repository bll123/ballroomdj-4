/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "mdebug.h"
#include "pathbld.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiwidget.h"

typedef struct uiimage {
  GdkPixbuf *pixbuf;
} uimage_t;

uiwcont_t *
uiImageNew (void)
{
  uiwcont_t *uiwidget;
  GtkWidget *image;
  uiimage_t *uiimage;

  uiimage = mdmalloc (sizeof (uiimage_t));
  uiimage->pixbuf = NULL;

  image = gtk_image_new ();
  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, image, NULL);
  uiwidget->uiint.uiimage = uiimage;

  return uiwidget;
}

void
uiImageFree (uiwcont_t *uiwidget)
{
  uiimage_t   *uiimage;

  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-free")) {
    return;
  }

  uiimage = uiwidget->uiint.uiimage;
  if (uiimage != NULL && uiimage->pixbuf != NULL) {
    g_object_unref (G_OBJECT (uiimage->pixbuf));
  }
  mdfree (uiimage);
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t   *uiwidget = NULL;
  GtkWidget   *image;
  GdkPixbuf   *pixbuf;
  uiimage_t   *uiimage;

  uiimage = mdmalloc (sizeof (uiimage_t));
  uiimage->pixbuf = NULL;

  pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
  if (pixbuf != NULL) {
    image = gtk_image_new_from_pixbuf (pixbuf);

    uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
    uiwidget->uidata.widget = image;
    uiwidget->uidata.packwidget = image;
    uiwidget->uiint.uiimage = uiimage;
    uiimage->pixbuf = pixbuf;
  }

  uiWidgetAlignHorizCenter (uiwidget);
  uiWidgetAlignVertCenter (uiwidget);

  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  uiwcont_t *uiwidget = NULL;
  GdkPixbuf *pixbuf;
  GtkWidget *image = NULL;
  uiimage_t *uiimage;

  uiimage = mdmalloc (sizeof (uiimage_t));
  uiimage->pixbuf = NULL;

  pixbuf = gdk_pixbuf_new_from_file_at_scale (fn, scale, -1, TRUE, NULL);
  if (pixbuf != NULL) {
    image = gtk_image_new ();
    gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
    uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
    uiwidget->uidata.widget = image;
    uiwidget->uidata.packwidget = image;
    uiwidget->uiint.uiimage = uiimage;
    uiimage->pixbuf = pixbuf;
  }

  uiWidgetAlignHorizCenter (uiwidget);
  uiWidgetAlignVertCenter (uiwidget);

  return uiwidget;
}

void
uiImageClear (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-clr")) {
    return;
  }

  if (GTK_IS_IMAGE (uiwidget->uidata.widget)) {
    uiimage_t   *uiimage;

    uiimage = uiwidget->uiint.uiimage;
    gtk_image_clear (GTK_IMAGE (uiwidget->uidata.widget));
    if (uiimage->pixbuf != NULL) {
      g_object_unref (uiimage->pixbuf);
    }
    uiimage->pixbuf = NULL;
  }
}

void
uiImageCopy (uiwcont_t *toimg, uiwcont_t *fromimg)
{
  uiimage_t   *uiimage;

  if (! uiwcontValid (toimg, WCONT_T_IMAGE, "image-copy-to")) {
    return;
  }
  if (! uiwcontValid (fromimg, WCONT_T_IMAGE, "image-copy-from")) {
    return;
  }

  uiImageClear (toimg);
  uiimage = toimg->uiint.uiimage;
  uiimage->pixbuf = fromimg->uiint.uiimage->pixbuf;
  g_object_ref_sink (G_OBJECT (uiimage->pixbuf));
  gtk_image_set_from_pixbuf (GTK_IMAGE (toimg->uidata.widget),
      uiimage->pixbuf);
}

