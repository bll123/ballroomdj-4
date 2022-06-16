#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "ui.h"

typedef struct uiswitch {
  UIWidget    uiswitch;
  UIWidget    switchoffimg;
  UIWidget    switchonimg;
} uiswitch_t;

static void uiSwitchImageHandler (GtkButton *b, gpointer udata);
static void uiSwitchToggleHandler (GtkButton *b, gpointer udata);
static void uiSwitchSetImage (GtkWidget *w, void *udata);

uiswitch_t *
uiCreateSwitch (int value)
{
  uiswitch_t  *uiswitch;
  GtkWidget   *widget;

  /* the gtk switch is different in every theme, some of which are not */
  /* great.  use a toggle button instead and set our own image */

  uiswitch = malloc (sizeof (uiswitch_t));
  uiutilsUIWidgetInit (&uiswitch->uiswitch);
  uiutilsUIWidgetInit (&uiswitch->switchoffimg);
  uiutilsUIWidgetInit (&uiswitch->switchonimg);

  uiImageFromFile (&uiswitch->switchoffimg, "img/switch-off.svg");
  uiWidgetMakePersistent (&uiswitch->switchoffimg);
  uiImageFromFile (&uiswitch->switchonimg, "img/switch-on.svg");
  uiWidgetMakePersistent (&uiswitch->switchonimg);

  widget = gtk_toggle_button_new ();
  uiswitch->uiswitch.widget = widget;

  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  uiSwitchSetImage (widget, uiswitch);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  uiSetCss (widget,
      "button { "
      "  border-bottom-left-radius: 15px; "
      "  border-bottom-right-radius: 15px; "
      "  border-top-left-radius: 15px; "
      "  border-top-right-radius: 15px; "
      "  background-color: shade(@theme_base_color,0.8); "
      "} "
      "button:checked { "
      "  background-color: shade(@theme_base_color,0.8); "
      "} "
      "button:hover { "
      "  background-color: shade(@theme_base_color,0.8); "
      "} "
      );
  g_signal_connect (widget, "toggled",
      G_CALLBACK (uiSwitchImageHandler), uiswitch);
  return uiswitch;
}

void
uiSwitchFree (uiswitch_t *uiswitch)
{
  if (uiswitch != NULL) {
    uiWidgetClearPersistent (&uiswitch->switchoffimg);
    uiWidgetClearPersistent (&uiswitch->switchonimg);
    free (uiswitch);
  }
}

void
uiSwitchSetValue (uiswitch_t *uiswitch, int value)
{
  if (uiswitch == NULL) {
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiswitch->uiswitch.widget), value);
  uiSwitchSetImage (uiswitch->uiswitch.widget, uiswitch);
}

int
uiSwitchGetValue (uiswitch_t *uiswitch)
{
  if (uiswitch == NULL) {
    return 0;
  }
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiswitch->uiswitch.widget));
}

UIWidget *
uiSwitchGetUIWidget (uiswitch_t *uiswitch)
{
  if (uiswitch == NULL) {
    return NULL;
  }
  return &uiswitch->uiswitch;
}

void
uiSwitchSetCallback (uiswitch_t *uiswitch, UICallback *uicb)
{
  g_signal_connect (uiswitch->uiswitch.widget, "toggled",
      G_CALLBACK (uiSwitchToggleHandler), uicb);
}

void
uiSwitchDisable (uiswitch_t *uiswitch)
{
  uiWidgetDisable (&uiswitch->uiswitch);
}

void
uiSwitchEnable (uiswitch_t *uiswitch)
{
  uiWidgetEnable (&uiswitch->uiswitch);
}


/* internal routines */

static void
uiSwitchToggleHandler (GtkButton *b, gpointer udata)
{
  UICallback  *uicb = udata;
  long        value;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b));
  uiutilsCallbackLongHandler (uicb, value);
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

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (uiswitch->uiswitch.widget));
  if (value) {
    gtk_button_set_image (GTK_BUTTON (w), uiswitch->switchonimg.widget);
  } else {
    gtk_button_set_image (GTK_BUTTON (w), uiswitch->switchoffimg.widget);
  }
}
