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

#include "mdebug.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"

uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *widget;

  uiwidget = uiwcontAlloc ();

  widget = gtk_label_new ("");
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_margin_end (widget, uiBaseMarginSz);

  uiwidget->wbasetype = WCONT_T_CHGIND;
  uiwidget->wtype = WCONT_T_CHGIND;
  uiwidget->widget = widget;
  /* the change indicator is a label packed in the beginning of */
  /* the supplied box */
  uiBoxPackStart (boxp, uiwidget);
  return uiwidget;
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-norm")) {
    return;
  }

  uiWidgetRemoveClass (uiwidget, CHGIND_CHANGED_CLASS);
  uiWidgetRemoveClass (uiwidget, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uiwidget, CHGIND_NORMAL_CLASS);
}

void
uichgindMarkError (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-err")) {
    return;
  }

  uiWidgetRemoveClass (uiwidget, CHGIND_NORMAL_CLASS);
  uiWidgetRemoveClass (uiwidget, CHGIND_CHANGED_CLASS);
  uiWidgetSetClass (uiwidget, CHGIND_ERROR_CLASS);
}

void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-chg")) {
    return;
  }

  uiWidgetRemoveClass (uiwidget, CHGIND_NORMAL_CLASS);
  uiWidgetRemoveClass (uiwidget, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uiwidget, CHGIND_CHANGED_CLASS);
}
