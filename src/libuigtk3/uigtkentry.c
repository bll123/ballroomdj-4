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
#include "pathutil.h"
#include "tmutil.h"
#include "ui.h"
#include "uiutils.h"

typedef struct uientry {
  GtkEntryBuffer  *buffer;
  UIWidget        uientry;
  int             entrySize;
  int             maxSize;
  uiutilsentryval_t validateFunc;
  mstime_t        validateTimer;
  void            *udata;
} uientry_t;

static void uiEntryValidateStart (GtkEditable *e, gpointer udata);
static void uiEntryValidateHandler (GtkEditable *e, gpointer udata);

uientry_t *
uiEntryInit (int entrySize, int maxSize)
{
  uientry_t *entry;

  entry = malloc (sizeof (uientry_t));
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->buffer = NULL;
  uiutilsUIWidgetInit (&entry->uientry);
  entry->validateFunc = NULL;
  entry->udata = NULL;
  mstimeset (&entry->validateTimer, 3600000);
  return entry;
}


void
uiEntryFree (uientry_t *entry)
{
  if (entry != NULL) {
    free (entry);
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

  gtk_entry_buffer_set_text (entry->buffer, value, -1);
}

void
uiEntrySetColor (uientry_t *entry, const char *color)
{
  char  tbuff [100];
  if (entry == NULL) {
    return;
  }
  if (entry->uientry.widget == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "entry { color: %s; }", color);
  uiSetCss (entry->uientry.widget, tbuff);
}

void
uiEntrySetValidate (uientry_t *entry, uiutilsentryval_t valfunc, void *udata,
    int valdelay)
{
  entry->validateFunc = valfunc;
  entry->udata = udata;
  if (valfunc != NULL) {
    if (valdelay) {
      g_signal_connect (entry->uientry.widget, "changed",
          G_CALLBACK (uiEntryValidateStart), entry);
      mstimeset (&entry->validateTimer, 500);
    }
    if (! valdelay) {
      g_signal_connect (entry->uientry.widget, "changed",
          G_CALLBACK (uiEntryValidateHandler), entry);
    }
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

  rc = UIENTRY_ERROR;
  if (entry->buffer != NULL) {
    dir = gtk_entry_buffer_get_text (entry->buffer);
    if (dir != NULL) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      if (fileopIsDirectory (tbuff)) {
        rc = UIENTRY_OK;
      } /* exists */
    } /* not null */
  } /* have a buffer */

  return rc;
}

int
uiEntryValidateFile (uientry_t *entry, void *udata)
{
  int              rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];

  rc = UIENTRY_ERROR;
  if (entry->buffer != NULL) {
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
  } /* have a buffer */

  return rc;
}

void
uiEntryDisable (uientry_t *entry)
{
  if (entry == NULL) {
    return;
  }
  uiWidgetDisable (&entry->uientry);
}

void
uiEntryEnable (uientry_t *entry)
{
  if (entry == NULL) {
    return;
  }
  uiWidgetEnable (&entry->uientry);
}

/* internal routines */

static void
uiEntryValidateStart (GtkEditable *e, gpointer udata)
{
  uientry_t  *entry = udata;

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
  return;
}

