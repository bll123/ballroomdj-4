/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */

/* A switch is a toggle button with different images set up. */
/* I felt the gtk switch widget was messy and not worth the trouble. */

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "callback.h"
#include "mdebug.h"
#include "pathbld.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uispecific.h"
#include "ui/uiswitch.h"

typedef struct uiswitch {
  GtkWidget   *currimage;
  GtkWidget   *image;
  GdkPixbuf   *imageraw;
  GtkWidget   *altimage;
  GdkPixbuf   *altimageraw;
} uiswitch_t;

static void uiSwitchImageHandler (GtkButton *b, gpointer udata);
static void uiSwitchToggleHandler (GtkButton *b, gpointer udata);
static void uiSwitchSetImage (void *udata);

uiwcont_t *
uiCreateSwitch (int value)
{
  uiwcont_t   *uiwidget;
  uiswitch_t  *uiswitch;
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [BDJ4_PATH_MAX];

  /* the gtk switch is different in every theme, some of which are not */
  /* great.  use a toggle button instead and set our own image */

  uiwidget = uiwcontAlloc (WCONT_T_SWITCH, WCONT_T_SWITCH);

  uiswitch = mdmalloc (sizeof (uiswitch_t));
  uiswitch->currimage = NULL;
  uiswitch->image = NULL;
  uiswitch->imageraw = NULL;
  uiswitch->altimage = NULL;
  uiswitch->altimageraw = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff), "switch-off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  image = uiImageWidget (tbuff);
  uiswitch->image = image;
  if (image != NULL) {
    uiswitch->imageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "switch-on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  image = uiImageWidget (tbuff);
  uiswitch->altimage = image;
  if (image != NULL) {
    uiswitch->altimageraw = gtk_image_get_pixbuf (GTK_IMAGE (image));
  }

  widget = gtk_toggle_button_new ();
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uiswitch = uiswitch;

  image = gtk_image_new ();
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  uiswitch->currimage = image;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);

  uiWidgetAddClass (uiwidget, SWITCH_CLASS);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);

  g_signal_connect (widget, "toggled",
      G_CALLBACK (uiSwitchImageHandler), uiwidget);

  uiSwitchSetImage (uiwidget);

  return uiwidget;
}

/* only frees the internals */
void
uiSwitchFree (uiwcont_t *uiwidget)
{
  uiswitch_t    *sw;

  if (! uiwcontValid (uiwidget, WCONT_T_SWITCH, "switch-free")) {
    return;
  }

  sw = uiwidget->uiint.uiswitch;
  mdfree (sw);
}

void
uiSwitchSetValue (uiwcont_t *uiwidget, int value)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SWITCH, "switch-set")) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget), value);
  uiSwitchSetImage (uiwidget);
}

int
uiSwitchGetValue (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SWITCH, "switch-get")) {
    return false;
  }

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget));
}

void
uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SWITCH, "switch-set-cb")) {
    return;
  }

  g_signal_connect (uiwidget->uidata.widget, "toggled",
      G_CALLBACK (uiSwitchToggleHandler), uicb);
}

/* internal routines */

static void
uiSwitchToggleHandler (GtkButton *b, gpointer udata)
{
  callback_t  *uicb = udata;
  int         value;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b));
  callbackHandlerI (uicb, value);
  return;
}

static void
uiSwitchImageHandler (GtkButton *b, gpointer udata)
{
  uiSwitchSetImage (udata);
}

static void
uiSwitchSetImage (void *udata)
{
  GtkWidget     *w;
  uiwcont_t     *uiwidget = udata;
  int           value;
  uiswitch_t    *sw;

  if (uiwidget == NULL || uiwidget->wtype != WCONT_T_SWITCH) {
    return;
  }

  w = uiwidget->uidata.widget;
  sw = uiwidget->uiint.uiswitch;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget));
  gtk_image_clear (GTK_IMAGE (sw->currimage));
  if (value) {
    gtk_image_set_from_pixbuf (GTK_IMAGE (sw->currimage), sw->altimageraw);
  } else {
    gtk_image_set_from_pixbuf (GTK_IMAGE (sw->currimage), sw->imageraw);
  }
}
