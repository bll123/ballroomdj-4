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

#include "mdebug.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"

typedef struct uichgind {
  uiwcont_t  *label;
} uichgind_t;

uichgind_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  uichgind_t  *uichgind;
  GtkWidget   *widget;

  uichgind = mdmalloc (sizeof (uichgind_t));

  widget = gtk_label_new ("");
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_margin_end (widget, uiBaseMarginSz);

  uichgind->label = uiwcontAlloc ();
  uichgind->label->widget = widget;
  uiBoxPackStart (boxp, uichgind->label);
  return uichgind;
}

void
uichgindFree (uichgind_t *uichgind)
{
  if (uichgind != NULL) {
    uiwcontFree (uichgind->label);
    mdfree (uichgind);
  }
}

void
uichgindMarkNormal (uichgind_t *uichgind)
{
  uiWidgetRemoveClass (uichgind->label, CHGIND_CHANGED_CLASS);
  uiWidgetRemoveClass (uichgind->label, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uichgind->label, CHGIND_NORMAL_CLASS);
}

void
uichgindMarkError (uichgind_t *uichgind)
{
  uiWidgetRemoveClass (uichgind->label, CHGIND_NORMAL_CLASS);
  uiWidgetRemoveClass (uichgind->label, CHGIND_CHANGED_CLASS);
  uiWidgetSetClass (uichgind->label, CHGIND_ERROR_CLASS);
}

void
uichgindMarkChanged (uichgind_t *uichgind)
{
  uiWidgetRemoveClass (uichgind->label, CHGIND_NORMAL_CLASS);
  uiWidgetRemoveClass (uichgind->label, CHGIND_ERROR_CLASS);
  uiWidgetSetClass (uichgind->label, CHGIND_CHANGED_CLASS);
}
