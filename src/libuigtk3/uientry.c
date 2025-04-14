/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "mdebug.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

typedef struct uientry {
  GtkEntryBuffer  *buffer;
} uientry_t;

static void uigtkEntryValidateHandler (GtkEditable *e, gpointer udata);
static gboolean uiEntryFocusHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata);

uiwcont_t *
uiEntryInit (int entrySize, int maxSize)
{
  uiwcont_t     *uiwidget;
  uientry_t     *uientry;
  uientrybase_t *ebase;
  GtkWidget     *widget;

  uientry = mdmalloc (sizeof (uientry_t));
  uientry->buffer = gtk_entry_buffer_new (NULL, -1);

  widget = gtk_entry_new_with_buffer (uientry->buffer);

  uiwidget = uiwcontAlloc (WCONT_T_ENTRY, WCONT_T_ENTRY);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uientry = uientry;

  ebase = &uiwidget->uiint.uientrybase;
  ebase->entrySize = entrySize;
  ebase->maxSize = maxSize;
  ebase->validateFunc = NULL;
  ebase->label = "";
  ebase->udata = NULL;
  mstimeset (&ebase->validateTimer, TM_TIMER_OFF);
  ebase->valdelay = false;
  ebase->valid = true;

  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->uidata.widget), ebase->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->uidata.widget), ebase->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (uiwidget->uidata.widget), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (uiwidget->uidata.widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (uiwidget->uidata.widget, uiBaseMarginSz * 2);
  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (uiwidget->uidata.widget, FALSE);

  return uiwidget;
}


void
uiEntryFree (uiwcont_t *uiwidget)
{
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-free")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;
  mdfree (uientry);
  /* the container is freed by uiwcontFree() */
}

void
uiEntrySetIcon (uiwcont_t *uiwidget, const char *name)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-icon")) {
    return;
  }

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uiwidget->uidata.widget),
      GTK_ENTRY_ICON_SECONDARY, name);
}

void
uiEntryClearIcon (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-clear-icon")) {
    return;
  }

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uiwidget->uidata.widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
}

const char *
uiEntryGetValue (uiwcont_t *uiwidget)
{
  uientry_t   *uientry;
  const char  *value;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-get-value")) {
    return NULL;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->buffer == NULL) {
    return NULL;
  }

  value = gtk_entry_buffer_get_text (uientry->buffer);
  return value;
}

void
uiEntrySetValue (uiwcont_t *uiwidget, const char *value)
{
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-value")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->buffer == NULL) {
    return;
  }

  if (value == NULL) {
    value = "";
  }

  gtk_entry_buffer_set_text (uientry->buffer, value, -1);
}

void
uiEntrySetInternalValidate (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-int-validate")) {
    return;
  }

  uiwidget->sigid [SIGID_VAL_CHG] =
      g_signal_connect (uiwidget->uidata.widget, "changed",
      G_CALLBACK (uigtkEntryValidateHandler), uiwidget);
}

void
uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-cb")) {
    return;
  }

  uiwidget->sigid [SIGID_FOCUS] =
      g_signal_connect (uiwidget->uidata.widget, "focus-in-event",
      G_CALLBACK (uiEntryFocusHandler), uicb);
}

void
uiEntrySetState (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-state")) {
    return;
  }

  uiWidgetSetState (uiwidget, state);
}

/* internal routines */

static void
uigtkEntryValidateHandler (GtkEditable *e, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;

  uiEntryClearIcon (uiwidget);
  uiEntryValidateHandler (uiwidget);
}

static gboolean
uiEntryFocusHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata)
{
  callback_t    *uicb = udata;

  if (uicb != NULL) {
    callbackHandler (uicb);
  }

  return false;
}
