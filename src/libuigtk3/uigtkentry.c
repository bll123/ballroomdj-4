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

static void uiEntryValidateStart (GtkEditable *e, gpointer udata);

void
uiEntryInit (uientry_t *entry, int entrySize, int maxSize)
{
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->buffer = NULL;
  uiutilsUIWidgetInit (&entry->uientry);
  entry->validateFunc = NULL;
  entry->udata = NULL;
  mstimeset (&entry->validateTimer, 3600000);
}


void
uiEntryFree (uientry_t *entry)
{
  ;
}

GtkWidget *
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

  return entry->uientry.widget;
}

void
uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry)
{
  gtk_entry_set_buffer (GTK_ENTRY (targetentry->uientry.widget), sourceentry->buffer);
  targetentry->buffer = sourceentry->buffer;
}

GtkWidget *
uiEntryGetWidget (uientry_t *entry)
{
  if (entry == NULL) {
    return NULL;
  }

  return entry->uientry.widget;
}

const char *
uiEntryGetValue (uientry_t *entry)
{
  const char  *value;

  if (entry == NULL) {
    return NULL;
  }

  value = gtk_entry_buffer_get_text (entry->buffer);
  return value;
}

void
uiEntrySetValue (uientry_t *entry, const char *value)
{
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
uiEntrySetValidate (uientry_t *entry, uiutilsentryval_t valfunc, void *udata)
{
  entry->validateFunc = valfunc;
  entry->udata = udata;
  g_signal_connect (entry->uientry.widget, "changed",
      G_CALLBACK (uiEntryValidateStart), entry);
  if (entry->validateFunc != NULL) {
    mstimeset (&entry->validateTimer, 3600000);
  }
}

bool
uiEntryValidate (uientry_t *entry)
{
  bool  rc;

  if (entry->validateFunc == NULL) {
    return true;
  }
  if (! mstimeCheck (&entry->validateTimer)) {
    return true;
  }

  rc = entry->validateFunc (entry, entry->udata);
  if (! rc) {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
        GTK_ENTRY_ICON_SECONDARY, "dialog-error");
  } else {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
        GTK_ENTRY_ICON_SECONDARY, NULL);
  }
  mstimeset (&entry->validateTimer, 3600000);
  return rc;
}

bool
uiEntryValidateDir (void *edata, void *udata)
{
  uientry_t    *entry = edata;
  bool              rc;
  const char        *dir;
  char              tbuff [MAXPATHLEN];

  rc = false;
  if (entry->buffer != NULL) {
    dir = gtk_entry_buffer_get_text (entry->buffer);
    if (dir != NULL) {
      strlcpy (tbuff, dir, sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      if (fileopIsDirectory (tbuff)) {
        rc = true;
      }
    }
  }

  return rc;
}

bool
uiEntryValidateFile (void *edata, void *udata)
{
  uientry_t    *entry = edata;
  bool              rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];

  rc = false;
  if (entry->buffer != NULL) {
    fn = gtk_entry_buffer_get_text (entry->buffer);
    if (fn != NULL) {
      if (*fn == '\0') {
        rc = true;
      } else {
        strlcpy (tbuff, fn, sizeof (tbuff));
        pathNormPath (tbuff, sizeof (tbuff));
        if (fileopFileExists (tbuff)) {
          rc = true;
        }
      }
    }
  }

  return rc;
}

/* internal routines */

static void
uiEntryValidateStart (GtkEditable *e, gpointer udata)
{
  uientry_t  *entry = udata;

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->uientry.widget),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  mstimeset (&entry->validateTimer, 500);
}

