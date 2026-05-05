/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

typedef struct uientryint {
  GtkEntryBuffer  *buffer;
  bool            changed;
} uientryint_t;

static void uigtkEntryValidateHandler (GtkEditable *e, gpointer udata);
static gboolean uiEntryFocusInHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata);
static gboolean uiEntryFocusOutHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata);

uiwcont_t *
uiEntryInit_r (int entrySize, int maxSize)
{
  uiwcont_t     *uiwidget;
  uientryint_t     *uientry;
  GtkWidget     *widget;

  uientry = mdmalloc (sizeof (uientryint_t));
  uientry->buffer = gtk_entry_buffer_new (NULL, -1);

  widget = gtk_entry_new_with_buffer (uientry->buffer);

  uiwidget = uiwcontAlloc (WCONT_T_ENTRY, WCONT_T_ENTRY);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uientry = uientry;
  uientry->changed = false;

  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->widget), entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->widget), maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (uiwidget->widget), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (uiwidget->widget, FALSE);

  return uiwidget;
}


void
uiEntryFree_r (uiwcont_t *uiwidget)
{
  uientryint_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-free")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;
  mdfree (uientry);
  /* the container is freed by uiwcontFree() */
}

void
uiEntrySetWidth (uiwcont_t *uiwidget, int maxWidth)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-width")) {
    return;
  }

  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->widget), maxWidth);
}

void
uiEntrySetIcon (uiwcont_t *uiwidget, const char *name)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-icon")) {
    return;
  }

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uiwidget->widget),
      GTK_ENTRY_ICON_SECONDARY, name);
}

void
uiEntryClearIcon (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-clear-icon")) {
    return;
  }

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uiwidget->widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
}

const char *
uiEntryGetValue (uiwcont_t *uiwidget)
{
  uientryint_t   *uientry;
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
  uientryint_t   *uientry;

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

  g_signal_connect (uiwidget->widget, "changed",
      G_CALLBACK (uigtkEntryValidateHandler), uiwidget);
}

void
uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-in-cb")) {
    return;
  }

  g_signal_connect (uiwidget->widget, "focus-in-event",
      G_CALLBACK (uiEntryFocusInHandler), uicb);
}

void
uiEntrySetFocusOutCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-out-cb")) {
    return;
  }

  g_signal_connect (uiwidget->widget, "focus-out-event",
      G_CALLBACK (uiEntryFocusOutHandler), uicb);
}

void
uiEntrySetState (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-state")) {
    return;
  }

  uiWidgetSetState (uiwidget, state);
}

bool
uiEntryChanged (uiwcont_t *uiwidget)
{
  uientryint_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-changed")) {
    return false;
  }

  uientry = uiwidget->uiint.uientry;
  return uientry->changed;
}

void
uiEntryClearChanged (uiwcont_t *uiwidget)
{
  uientryint_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-changed")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;
  uientry->changed = false;
}

void
uiEntryAlignEnd (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-align-end")) {
    return;
  }

  gtk_entry_set_alignment (GTK_ENTRY (uiwidget->widget), 1.0);
}

/* internal routines */

static void
uigtkEntryValidateHandler (GtkEditable *e, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  uientryint_t   *uientry;

  uientry = uiwidget->uiint.uientry;
  uientry->changed = true;
  uiEntryClearIcon (uiwidget);
  uiEntryValidateHandler (uiwidget);
}

static gboolean
uiEntryFocusInHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata)
{
  callback_t    *uicb = udata;

  if (uicb != NULL) {
    callbackHandler (uicb);
  }

  return false;
}

static gboolean
uiEntryFocusOutHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata)
{
  callback_t    *uicb = udata;

  if (uicb != NULL) {
    callbackHandler (uicb);
  }

  return false;
}
