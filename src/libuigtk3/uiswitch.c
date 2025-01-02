/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "ui/uiswitch.h"

typedef struct uiswitch {
  uiwcont_t  *switchoffimg;
  uiwcont_t  *switchonimg;
} uiswitch_t;

static void uiSwitchImageHandler (GtkButton *b, gpointer udata);
static void uiSwitchToggleHandler (GtkButton *b, gpointer udata);
static void uiSwitchSetImage (GtkWidget *w, void *udata);

uiwcont_t *
uiCreateSwitch (int value)
{
  uiwcont_t   *uiwidget;
  uiswitch_t  *uiswitch;
  GtkWidget   *widget;
  char        tbuff [MAXPATHLEN];

  /* the gtk switch is different in every theme, some of which are not */
  /* great.  use a toggle button instead and set our own image */

  uiwidget = uiwcontAlloc (WCONT_T_SWITCH, WCONT_T_SWITCH);

  uiswitch = mdmalloc (sizeof (uiswitch_t));
  uiswitch->switchoffimg = NULL;
  uiswitch->switchonimg = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff), "switch-off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiswitch->switchoffimg = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiswitch->switchoffimg);

  pathbldMakePath (tbuff, sizeof (tbuff), "switch-on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiswitch->switchonimg = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiswitch->switchonimg);

  widget = gtk_toggle_button_new ();
  uiwcontSetWidget (uiwidget, widget, NULL);
//  uiwidget->uidata.widget = widget;
//  uiwidget->uidata.packwidget = widget;
  uiwidget->uiint.uiswitch = uiswitch;

  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);

  uiWidgetAddClass (uiwidget, SWITCH_CLASS);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  uiSwitchSetImage (widget, uiwidget);

  g_signal_connect (widget, "toggled",
      G_CALLBACK (uiSwitchImageHandler), uiwidget);

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
  uiWidgetClearPersistent (sw->switchoffimg);
  uiWidgetClearPersistent (sw->switchonimg);
  uiwcontBaseFree (sw->switchoffimg);
  uiwcontBaseFree (sw->switchonimg);
  mdfree (sw);
  /* the container is freed by uiwcontFree() */
}

void
uiSwitchSetValue (uiwcont_t *uiwidget, int value)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SWITCH, "switch-set")) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget), value);
  uiSwitchSetImage (uiwidget->uidata.widget, uiwidget);
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
  uiSwitchSetImage (GTK_WIDGET (b), udata);
}

static void
uiSwitchSetImage (GtkWidget *w, void *udata)
{
  uiwcont_t     *uiwidget = udata;
  int           value;
  uiswitch_t    *sw;

  if (uiwidget == NULL || uiwidget->wtype != WCONT_T_SWITCH) {
    return;
  }
  if (w == NULL) {
    return;
  }

  sw = uiwidget->uiint.uiswitch;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiwidget->uidata.widget));
  if (value) {
    gtk_button_set_image (GTK_BUTTON (w), sw->switchonimg->uidata.widget);
  } else {
    gtk_button_set_image (GTK_BUTTON (w), sw->switchoffimg->uidata.widget);
  }
}
