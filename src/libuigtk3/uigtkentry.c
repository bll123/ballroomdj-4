/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

typedef struct uientry {
  GtkEntryBuffer  *buffer;
  uiwcont_t       *entry;
  int             entrySize;
  int             maxSize;
  uientryval_t    validateFunc;
  mstime_t        validateTimer;
  void            *udata;
  bool            valdelay : 1;
} uientry_t;

static void uiEntryValidateStart (uientry_t *uientry);
static void uiEntryValidateHandler (GtkEditable *e, gpointer udata);

uientry_t *
uiEntryInit (int entrySize, int maxSize)
{
  uientry_t *uientry;

  uientry = mdmalloc (sizeof (uientry_t));
  uientry->entrySize = entrySize;
  uientry->maxSize = maxSize;
  uientry->buffer = NULL;
  uientry->entry = NULL;
  uientry->validateFunc = NULL;
  uientry->udata = NULL;
  mstimeset (&uientry->validateTimer, 3600000);
  uientry->valdelay = false;
  return uientry;
}


void
uiEntryFree (uientry_t *uientry)
{
  if (uientry != NULL) {
    uiwcontFree (uientry->entry);
    mdfree (uientry);
  }
}

void
uiEntryCreate (uientry_t *uientry)
{
  uientry->buffer = gtk_entry_buffer_new (NULL, -1);
  uientry->entry = uiwcontAlloc ();
  uientry->entry->wbasetype = WCONT_T_ENTRY;
  uientry->entry->wtype = WCONT_T_ENTRY;
  uientry->entry->widget = gtk_entry_new_with_buffer (uientry->buffer);
  gtk_entry_set_width_chars (GTK_ENTRY (uientry->entry->widget), uientry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (uientry->entry->widget), uientry->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (uientry->entry->widget), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (uientry->entry->widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (uientry->entry->widget, uiBaseMarginSz * 2);
  gtk_widget_set_halign (uientry->entry->widget, GTK_ALIGN_START);
  gtk_widget_set_hexpand (uientry->entry->widget, FALSE);
}

void
uiEntrySetIcon (uientry_t *uientry, const char *name)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uientry->entry->widget),
      GTK_ENTRY_ICON_SECONDARY, name);
}

void
uiEntryClearIcon (uientry_t *uientry)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uientry->entry->widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
}

uiwcont_t *
uiEntryGetWidgetContainer (uientry_t *uientry)
{
  if (uientry == NULL) {
    return NULL;
  }

  return uientry->entry;
}

void
uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry)
{
  gtk_entry_set_buffer (GTK_ENTRY (targetentry->entry->widget), sourceentry->buffer);
  targetentry->buffer = sourceentry->buffer;
}

const char *
uiEntryGetValue (uientry_t *uientry)
{
  const char  *value;

  if (uientry == NULL) {
    return NULL;
  }
  if (uientry->buffer == NULL) {
    return NULL;
  }

  value = gtk_entry_buffer_get_text (uientry->buffer);
  return value;
}

void
uiEntrySetValue (uientry_t *uientry, const char *value)
{
  if (uientry == NULL) {
    return;
  }
  if (uientry->buffer == NULL) {
    return;
  }

  if (value == NULL) {
    value = "";
  }

  gtk_entry_buffer_set_text (uientry->buffer, value, -1);
}

void
uiEntrySetValidate (uientry_t *uientry, uientryval_t valfunc, void *udata,
    int valdelay)
{
  uientry->validateFunc = valfunc;
  uientry->udata = udata;
  if (valfunc != NULL) {
    if (valdelay == UIENTRY_DELAYED) {
      mstimeset (&uientry->validateTimer, 500);
      uientry->valdelay = true;
    }
    g_signal_connect (uientry->entry->widget, "changed",
        G_CALLBACK (uiEntryValidateHandler), uientry);
  }
}

int
uiEntryValidate (uientry_t *uientry, bool forceflag)
{
  int   rc;

  if (uientry == NULL) {
    return UIENTRY_OK;
  }
  if (uientry->entry == NULL) {
    return UIENTRY_OK;
  }
  if (uientry->entry->widget == NULL) {
    return UIENTRY_OK;
  }
  if (uientry->validateFunc == NULL) {
    return UIENTRY_OK;
  }
  if (forceflag == false &&
      ! mstimeCheck (&uientry->validateTimer)) {
    return UIENTRY_OK;
  }

  rc = uientry->validateFunc (uientry, uientry->udata);
  if (rc == UIENTRY_RESET) {
    mstimeset (&uientry->validateTimer, 500);
  }
  if (rc == UIENTRY_ERROR) {
    uiEntrySetIcon (uientry, "dialog-error");
  }
  if (rc == UIENTRY_OK) {
    uiEntryClearIcon (uientry);
  }
  mstimeset (&uientry->validateTimer, 3600000);
  return rc;
}

int
uiEntryValidateDir (uientry_t *uientry, void *udata)
{
  int               rc;
  const char        *dir;
  char              tbuff [MAXPATHLEN];

  rc = UIENTRY_OK;
  if (uientry->buffer != NULL) {
    rc = UIENTRY_ERROR;
    dir = gtk_entry_buffer_get_text (uientry->buffer);
    if (dir != NULL) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      pathNormalizePath (tbuff, sizeof (tbuff));
      if (fileopIsDirectory (tbuff)) {
        rc = UIENTRY_OK;
      } /* exists */
    } /* not null */
  } /* changed and have a buffer */

  return rc;
}

int
uiEntryValidateFile (uientry_t *uientry, void *udata)
{
  int              rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];

  rc = UIENTRY_OK;
  if (uientry->buffer != NULL) {
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
  } /* changed and have a buffer */

  return rc;
}

void
uiEntrySetState (uientry_t *uientry, int state)
{
  if (uientry == NULL) {
    return;
  }
  uiWidgetSetState (uientry->entry, state);
}

/* internal routines */

static void
uiEntryValidateStart (uientry_t *uientry)
{
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (uientry->entry->widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  if (uientry->validateFunc != NULL) {
    mstimeset (&uientry->validateTimer, 500);
  }
}

static void
uiEntryValidateHandler (GtkEditable *e, gpointer udata)
{
  uientry_t  *uientry = udata;

  if (uientry->validateFunc != NULL) {
    uientry->validateFunc (uientry, uientry->udata);
  }
  if (uientry->valdelay) {
    uiEntryValidateStart (uientry);
  }
  return;
}

