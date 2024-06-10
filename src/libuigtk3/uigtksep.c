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

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uisep.h"

uiwcont_t *
uiCreateHorizSeparator (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *sep;

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SEPARATOR;
  uiwidget->wtype = WCONT_T_SEPARATOR;
  uiwidget->uidata.widget = sep;
  uiwidget->uidata.packwidget = sep;
  uiWidgetExpandHoriz (uiwidget);
  uiWidgetSetMarginTop (uiwidget, uiBaseMarginSz);
  return uiwidget;
}

void
uiSeparatorAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  if (classnm == NULL) {
    return;
  }
  if (color == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "separator.%s", classnm);
  uiAddBGColorClass (tbuff, color);
}

