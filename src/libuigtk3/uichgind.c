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

#include "mdebug.h"
#include "uiclass.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uichgind.h"
#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;

  widget = gtk_label_new ("");
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);

  uiwidget = uiwcontAlloc (WCONT_T_CHGIND, WCONT_T_CHGIND);
  uiwcontSetWidget (uiwidget, widget, NULL);
  if (boxp != NULL) {
    /* the change indicator is a label packed in the beginning of */
    /* the supplied box */
    uiBoxPackStart (boxp, uiwidget, WCONT_KEEP);
  }
  return uiwidget;
}

void
uichgindPackGrid (uiwcont_t *uiwidget, uiwcont_t *grid)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-pack-grid")) {
    return;
  }
  if (! uiwcontValid (grid, WCONT_T_GRID, "ci-pack-grid")) {
    return;
  }

  uiGridPack (grid, uiwidget, WCONT_KEEP);
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-norm")) {
    return;
  }

  uiWidgetClearClass (uiwidget, CHGIND_CHANGED_CLASS);
  uiWidgetClearClass (uiwidget, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uiwidget, CHGIND_NORMAL_CLASS);
}


void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-chg")) {
    return;
  }

  uiWidgetClearClass (uiwidget, CHGIND_NORMAL_CLASS);
  uiWidgetClearClass (uiwidget, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uiwidget, CHGIND_CHANGED_CLASS);
}
