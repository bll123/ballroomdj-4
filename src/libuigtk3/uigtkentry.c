/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pathutil.h"
#include "tmutil.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

enum {
  UIENTRY_VAL_TIMER = 400,
  UIENTRY_VAL_TIMER_OFF = 3600000,
};

typedef struct uientry {
  GtkEntryBuffer  *buffer;
  int             entrySize;
  int             maxSize;
  uientryval_t    validateFunc;
  const char      *label;
  mstime_t        validateTimer;
  void            *udata;
  bool            valdelay : 1;
} uientry_t;

static void uiEntryValidateStart (uiwcont_t *uiwidget);
static void uiEntryValidateHandler (GtkEditable *e, gpointer udata);
static gboolean uiEntryFocusHandler (GtkWidget* w, GdkEventFocus *event, gpointer udata);

uiwcont_t *
uiEntryInit (int entrySize, int maxSize)
{
  uiwcont_t *uiwidget;
  uientry_t *uientry;

  uientry = mdmalloc (sizeof (uientry_t));
  uientry->entrySize = entrySize;
  uientry->maxSize = maxSize;
  uientry->buffer = NULL;
  uientry->validateFunc = NULL;
  uientry->label = "";
  uientry->udata = NULL;
  mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER_OFF);
  uientry->valdelay = false;
  uientry->buffer = gtk_entry_buffer_new (NULL, -1);

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_ENTRY;
  uiwidget->wtype = WCONT_T_ENTRY;
  uiwidget->uiint.uientry = uientry;
  uiwidget->uidata.widget = gtk_entry_new_with_buffer (uientry->buffer);
  uiwidget->uidata.packwidget = uiwidget->uidata.widget;

  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->uidata.widget), uientry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->uidata.widget), uientry->maxSize);
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

void
uiEntryPeerBuffer (uiwcont_t *targetentry, uiwcont_t *sourceentry)
{
  uientry_t   *target;
  uientry_t   *source;

  if (! uiwcontValid (targetentry, WCONT_T_ENTRY, "entry-peer-buffer-target")) {
    return;
  }
  if (! uiwcontValid (sourceentry, WCONT_T_ENTRY, "entry-peer-buffer-src")) {
    return;
  }

  target = targetentry->uiint.uientry;
  source = sourceentry->uiint.uientry;

  gtk_entry_set_buffer (GTK_ENTRY (targetentry->uidata.widget), source->buffer);
  target->buffer = source->buffer;
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

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-get-value")) {
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
uiEntrySetValidate (uiwcont_t *uiwidget, const char *label,
    uientryval_t valfunc, void *udata, int valdelay)
{
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-validate")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;

  uientry->validateFunc = valfunc;
  uientry->label = label;
  uientry->udata = udata;

  if (valfunc != NULL) {
    if (valdelay == UIENTRY_DELAYED) {
      mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER);
      uientry->valdelay = true;
    }
    g_signal_connect (uiwidget->uidata.widget, "changed",
        G_CALLBACK (uiEntryValidateHandler), uiwidget);
  }
}

int
uiEntryValidate (uiwcont_t *uiwidget, bool forceflag)
{
  int   rc;
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate")) {
    return UIENTRY_OK;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->validateFunc == NULL) {
    return UIENTRY_OK;
  }
  if (forceflag == false &&
      ! mstimeCheck (&uientry->validateTimer)) {
    return UIENTRY_OK;
  }

  mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER_OFF);
  rc = uientry->validateFunc (uiwidget, uientry->label, uientry->udata);
  if (rc == UIENTRY_RESET) {
    mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER);
  }
  if (rc == UIENTRY_ERROR) {
    uiEntrySetIcon (uiwidget, "dialog-error");
  }
  if (rc == UIENTRY_OK) {
    uiEntryClearIcon (uiwidget);
  }
  return rc;
}

void
uiEntryValidateClear (uiwcont_t *uiwidget)
{
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-clear")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->validateFunc != NULL) {
    mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER_OFF);
  }
}

int
uiEntryValidateDir (uiwcont_t *uiwidget, const char *label, void *udata)
{
  int         rc;
  const char  *dir;
  char        tbuff [MAXPATHLEN];
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-dir")) {
    return UIENTRY_OK;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->buffer == NULL) {
    return UIENTRY_OK;
  }

  rc = UIENTRY_ERROR;
  dir = gtk_entry_buffer_get_text (uientry->buffer);
  if (dir != NULL) {
    strlcpy (tbuff, dir, sizeof (tbuff));
    pathNormalizePath (tbuff, sizeof (tbuff));
    if (fileopIsDirectory (tbuff)) {
      rc = UIENTRY_OK;
    } /* exists */
  } /* not null */

  return rc;
}

int
uiEntryValidateFile (uiwcont_t *uiwidget, const char *label, void *udata)
{
  int         rc;
  const char  *fn;
  char        tbuff [MAXPATHLEN];
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-validate-file")) {
    return UIENTRY_OK;
  }

  uientry = uiwidget->uiint.uientry;

  if (uientry->buffer == NULL) {
    return UIENTRY_OK;
  }

  rc = UIENTRY_ERROR;
  fn = gtk_entry_buffer_get_text (uientry->buffer);
  if (fn != NULL) {
    if (*fn == '\0') {
      rc = UIENTRY_OK;
    } else {
      strlcpy (tbuff, fn, sizeof (tbuff));
      pathNormalizePath (tbuff, sizeof (tbuff));
      if (fileopFileExists (tbuff)) {
        rc = UIENTRY_OK;
      } /* exists */
    } /* not empty */
  } /* not null */

  return rc;
}

void
uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  uientry_t   *uientry;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-cb")) {
    return;
  }

  uientry = uiwidget->uiint.uientry;

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
uiEntryValidateStart (uiwcont_t *uiwidget)
{
  uientry_t   *uientry;

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uiwidget->uidata.widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  uientry = uiwidget->uiint.uientry;
  if (uientry->validateFunc != NULL) {
    mstimeset (&uientry->validateTimer, UIENTRY_VAL_TIMER);
  }
}

static void
uiEntryValidateHandler (GtkEditable *e, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  uientry_t   *uientry;

  uientry = uiwidget->uiint.uientry;

  if (uientry->valdelay) {
    uiEntryValidateStart (uiwidget);
  } else {
    if (uientry->validateFunc != NULL) {
      uientry->validateFunc (uiwidget, uientry->label, uientry->udata);
    }
  }
  return;
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
