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

  if (uicb == NULL) {
    return UICB_STOP;
  }
  if (uicb->intcb == NULL) {
    return UICB_STOP;
  }

  rc = uicb->intcb (uicb->udata, value);
  return rc;
}
