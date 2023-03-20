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

#include "ui/uiinternal.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uisep.h"

void
uiCreateHorizSeparator (uiwcont_t *uiwidget)
{
  GtkWidget   *sep;

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiwidget->widget = sep;
  uiWidgetExpandHoriz (uiwidget);
  uiWidgetSetMarginTop (uiwidget, uiBaseMarginSz);
}

void
uiSeparatorAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "separator.%s", classnm);
  uiAddBGColorClass (tbuff, color);
}

