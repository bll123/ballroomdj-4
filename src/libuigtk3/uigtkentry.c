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
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "mdebug.h"
#include "pathutil.h"
#include "tmutil.h"

#include "ui/uigeneral.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

typedef struct uientry {
  GtkEntryBuffer  *buffer;
  UIWidget        uientry;
  int             entrySize;
  int             maxSize;
  uientryval_t    validateFunc;
  mstime_t        validateTimer;
  void            *udata;
  bool            valdelay : 1;
} uientry_t;

static void uiEntryValidateStart (uientry_t *entry);
static void uiEntryValidateHandler (GtkEditable *e, gpointer udata);

uientry_t *
uiEntryInit (int entrySize, int maxSize)
{
  uientry_t *entry;

  entry = mdmalloc (sizeof (uientry_t));
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->buffer = NULL;
  uiutilsUIWidgetInit (&entry->uientry);
  entry->validateFunc = NULL;
  entry->udata = NULL;
  mstimeset (&entry->validateTimer, 3600000);
  entry->valdelay = false;
  return entry;
}


void
uiEntryFree (uientry_t *entry)
{
  if (entry != NULL) {
    mdfree (entry);
  }
}

void
uiEntryCreate (uientry_t *entry)
{
  entry->buffer = gtk_entry_buffer_new (NULL, -1);
  entry->uientry.widget = gtk_entry_new_with_buffer (entry->buffer);
  gtk_entry_set_width_chars (GTK_ENTRY (entry->uientry.widget), entry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (entry->uientry.widget), entry->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry->uientry.widget), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (entry->uientry.widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (entry->uientry.widget, uiBaseMarginSz * 2);
  gtk_widget_set_halign (entry->uientry.widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (entry->uientry.widget, FALSE);
}

void
uiEntrySetIcon (uientry_t *entry, const char *name)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
      GTK_ENTRY_ICON_SECONDARY, name);
}

void
uiEntryClearIcon (uientry_t *entry)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
}

UIWidget *
uiEntryGetUIWidget (uientry_t *entry)
{
  if (entry == NULL) {
    return NULL;
  }

  return &entry->uientry;
}

void
uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry)
{
  gtk_entry_set_buffer (GTK_ENTRY (targetentry->uientry.widget), sourceentry->buffer);
  targetentry->buffer = sourceentry->buffer;
}

const char *
uiEntryGetValue (uientry_t *entry)
{
  const char  *value;

  if (entry == NULL) {
    return NULL;
  }
  if (entry->buffer == NULL) {
    return NULL;
  }

  value = gtk_entry_buffer_get_text (entry->buffer);
  return value;
}

void
uiEntrySetValue (uientry_t *entry, const char *value)
{
  if (entry == NULL) {
    return;
  }
  if (entry->buffer == NULL) {
    return;
  }

  if (value == NULL) {
    value = "";
  }

  gtk_entry_buffer_set_text (entry->buffer, value, -1);
}

void
uiEntrySetValidate (uientry_t *entry, uientryval_t valfunc, void *udata,
    int valdelay)
{
  entry->validateFunc = valfunc;
  entry->udata = udata;
  if (valfunc != NULL) {
    if (valdelay == UIENTRY_DELAYED) {
      mstimeset (&entry->validateTimer, 500);
      entry->valdelay = true;
    }
    g_signal_connect (entry->uientry.widget, "changed",
        G_CALLBACK (uiEntryValidateHandler), entry);
  }
}

int
uiEntryValidate (uientry_t *entry, bool forceflag)
{
  int   rc;

  if (entry == NULL) {
    return UIENTRY_OK;
  }
  if (entry->uientry.widget == NULL) {
    return UIENTRY_OK;
  }
  if (entry->validateFunc == NULL) {
    return UIENTRY_OK;
  }
  if (forceflag == false &&
      ! mstimeCheck (&entry->validateTimer)) {
    return UIENTRY_OK;
  }

  rc = entry->validateFunc (entry, entry->udata);
  if (rc == UIENTRY_RESET) {
    mstimeset (&entry->validateTimer, 500);
  }
  if (rc == UIENTRY_ERROR) {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
        GTK_ENTRY_ICON_SECONDARY, "dialog-error");
  }
  if (rc == UIENTRY_OK) {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
        GTK_ENTRY_ICON_SECONDARY, NULL);
  }
  mstimeset (&entry->validateTimer, 3600000);
  return rc;
}

int
uiEntryValidateDir (uientry_t *entry, void *udata)
{
  int               rc;
  const char        *dir;
  char              tbuff [MAXPATHLEN];

  rc = UIENTRY_OK;
  if (entry->buffer != NULL) {
    rc = UIENTRY_ERROR;
    dir = gtk_entry_buffer_get_text (entry->buffer);
    if (dir != NULL) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      if (fileopIsDirectory (tbuff)) {
        rc = UIENTRY_OK;
      } /* exists */
    } /* not null */
  } /* changed and have a buffer */

  return rc;
}

int
uiEntryValidateFile (uientry_t *entry, void *udata)
{
  int              rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];

  rc = UIENTRY_OK;
  if (entry->buffer != NULL) {
    rc = UIENTRY_ERROR;
    fn = gtk_entry_buffer_get_text (entry->buffer);
    if (fn != NULL) {
      if (*fn == '\0') {
        rc = UIENTRY_OK;
      } else {
        strlcpy (tbuff, fn, sizeof (tbuff));
        pathNormPath (tbuff, sizeof (tbuff));
        if (fileopFileExists (tbuff)) {
          rc = UIENTRY_OK;
        } /* exists */
      } /* not empty */
    } /* not null */
  } /* changed and have a buffer */

  return rc;
}

void
uiEntrySetState (uientry_t *entry, int state)
{
  if (entry == NULL) {
    return;
  }
  uiWidgetSetState (&entry->uientry, state);
}

/* internal routines */

static void
uiEntryValidateStart (uientry_t *entry)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  if (entry->validateFunc != NULL) {
    mstimeset (&entry->validateTimer, 500);
  }
}

static void
uiEntryValidateHandler (GtkEditable *e, gpointer udata)
{
  uientry_t  *entry = udata;

  if (entry->validateFunc != NULL) {
    entry->validateFunc (entry, entry->udata);
  }
  if (entry->valdelay) {
    uiEntryValidateStart (entry);
  }
  return;
}

