/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  uiwcont_t  *switchw;
  uiwcont_t  *switchoffimg;
  uiwcont_t  *switchonimg;
} uiswitch_t;

static void uiSwitchImageHandler (GtkButton *b, gpointer udata);
static void uiSwitchToggleHandler (GtkButton *b, gpointer udata);
static void uiSwitchSetImage (GtkWidget *w, void *udata);

uiswitch_t *
uiCreateSwitch (int value)
{
  uiswitch_t  *uiswitch;
  GtkWidget   *widget;
  char        tbuff [MAXPATHLEN];

  /* the gtk switch is different in every theme, some of which are not */
  /* great.  use a toggle button instead and set our own image */

  uiswitch = mdmalloc (sizeof (uiswitch_t));
  uiswitch->switchw = NULL;
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
  uiswitch->switchw = uiwcontAlloc ();
  uiswitch->switchw->widget = widget;

  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiSwitchSetImage (widget, uiswitch);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  uiWidgetSetClass (uiswitch->switchw, SWITCH_CLASS);
  g_signal_connect (widget, "toggled",
      G_CALLBACK (uiSwitchImageHandler), uiswitch);
  return uiswitch;
}

void
uiSwitchFree (uiswitch_t *uiswitch)
{
  if (uiswitch != NULL) {
    uiWidgetClearPersistent (uiswitch->switchoffimg);
    uiWidgetClearPersistent (uiswitch->switchonimg);
    uiwcontFree (uiswitch->switchoffimg);
    uiwcontFree (uiswitch->switchonimg);
    uiwcontFree (uiswitch->switchw);
    mdfree (uiswitch);
  }
}

void
uiSwitchSetValue (uiswitch_t *uiswitch, int value)
{
  if (uiswitch == NULL) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiswitch->switchw->widget), value);
  uiSwitchSetImage (uiswitch->switchw->widget, uiswitch);
}

int
uiSwitchGetValue (uiswitch_t *uiswitch)
{
  if (uiswitch == NULL) {
    return 0;
  }
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiswitch->switchw->widget));
}

uiwcont_t *
uiSwitchGetWidgetContainer (uiswitch_t *uiswitch)
{
  if (uiswitch == NULL) {
    return NULL;
  }
  return uiswitch->switchw;
}

void
uiSwitchSetCallback (uiswitch_t *uiswitch, callback_t *uicb)
{
  g_signal_connect (uiswitch->switchw->widget, "toggled",
      G_CALLBACK (uiSwitchToggleHandler), uicb);
}

void
uiSwitchSetState (uiswitch_t *uiswitch, int state)
{
  if (uiswitch == NULL) {
    return;
  }
  uiWidgetSetState (uiswitch->switchw, state);
}

/* internal routines */

static void
uiSwitchToggleHandler (GtkButton *b, gpointer udata)
{
  callback_t  *uicb = udata;
  int         value;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b));
  callbackHandlerLong (uicb, value);
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
  uiswitch_t  *uiswitch = udata;
  int         value;

  if (w == NULL) {
    return;
  }

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiswitch->switchw->widget));
  if (value) {
    gtk_button_set_image (GTK_BUTTON (w), uiswitch->switchonimg->widget);
  } else {
    gtk_button_set_image (GTK_BUTTON (w), uiswitch->switchoffimg->widget);
  }
}
