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

#include "ui.h"

void
uiCreateHorizSeparator (UIWidget *uiwidget)
{
  GtkWidget   *sep;
  char        tbuff [100];

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiwidget->widget = sep;
  uiWidgetExpandHoriz (uiwidget);
  uiWidgetSetMarginTop (uiwidget, uiBaseMarginSz);
  snprintf (tbuff, sizeof (tbuff),
      "separator { min-height: 4px; }");
  uiSetCss (uiwidget->widget, tbuff);
}

void
uiSeparatorAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "separator.%s", classnm);
  uiAddBGColorClass (tbuff, color);
}

