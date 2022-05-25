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
#include "uiutils.h"

static gboolean uiSwitchStateHandler (GtkSwitch *sw, gboolean value, gpointer udata);

void
uiCreateSwitch (UIWidget *uiwidget, int value)
{
  GtkWidget   *widget;

  widget = gtk_switch_new ();
  assert (widget != NULL);
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
#if 0
  /* the entire switch css would need replacing */
  /* switch, slider, etc. */
  /* if this route is taken, the slider can be made separate, */
  /* and the green/red text can be put in the background-image */
  uiSetCss (widget,
    "switch { "
    "  min-height: 26px; "
    "  min-width: 40px; "
    "  background-image: -gtk-scaled(url(\"img/switch-off.svg\")); "
    "}"
    "switch:checked { "
    "  min-height: 26px; "
    "  min-width: 40px; "
    "  background-image: -gtk-scaled(url(\"img/switch-on.svg\")); "
    "}"
    );
#endif
  uiwidget->widget = widget;
}

void
uiSwitchSetValue (UIWidget *uiwidget, int value)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }
  gtk_switch_set_active (GTK_SWITCH (uiwidget->widget), value);
}

int
uiSwitchGetValue (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return 0;
  }
  if (uiwidget->widget == NULL) {
    return 0;
  }
  return gtk_switch_get_active (GTK_SWITCH (uiwidget->widget));
}

void
uiSwitchSetCallback (UIWidget *uiwidget, UICallback *uicb)
{
  g_signal_connect (uiwidget->widget, "state-set",
      G_CALLBACK (uiSwitchStateHandler), uicb);
}

/* internal routines */

static gboolean
uiSwitchStateHandler (GtkSwitch *sw, gboolean value, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc;

  rc = uiutilsCallbackIntHandler (uicb, value);
  return rc;
}
