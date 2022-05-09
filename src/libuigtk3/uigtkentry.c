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
#include "log.h"
#include "pathutil.h"
#include "tmutil.h"
#include "uiutils.h"

static void uiEntryValidateStart (GtkEditable *e, gpointer udata);

void
uiEntryInit (uientry_t *entry, int entrySize, int maxSize)
{
  logProcBegin (LOG_PROC, "uiEntryInit");
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->buffer = NULL;
  entry->entry = NULL;
  entry->validateFunc = NULL;
  entry->udata = NULL;
  mstimeset (&entry->validateTimer, 3600000);
  logProcEnd (LOG_PROC, "uiEntryInit", "");
}


void
uiEntryFree (uientry_t *entry)
{
  ;
}

GtkWidget *
uiEntryCreate (uientry_t *entry)
{
  logProcBegin (LOG_PROC, "uiEntryCreate");
  entry->buffer = gtk_entry_buffer_new (NULL, -1);
  entry->entry = gtk_entry_new_with_buffer (entry->buffer);
  gtk_entry_set_width_chars (GTK_ENTRY (entry->entry), entry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (entry->entry), entry->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry->entry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (entry->entry, uiBaseMarginSz);
  gtk_widget_set_margin_start (entry->entry, uiBaseMarginSz);
  gtk_widget_set_halign (entry->entry, GTK_ALIGN_START);
  gtk_widget_set_hexpand (entry->entry, FALSE);

  logProcEnd (LOG_PROC, "uiEntryCreate", "");
  return entry->entry;
}

void
uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry)
{
  gtk_entry_set_buffer (GTK_ENTRY (targetentry->entry), sourceentry->buffer);
  targetentry->buffer = sourceentry->buffer;
}

GtkWidget *
uiEntryGetWidget (uientry_t *entry)
{
  return entry->entry;
}

const char *
uiEntryGetValue (uientry_t *entry)
{
  const char  *value;

  value = gtk_entry_buffer_get_text (entry->buffer);
  return value;
}

void
uiEntrySetValue (uientry_t *entry, const char *value)
{
  gtk_entry_buffer_set_text (entry->buffer, value, -1);
}

void
uiEntrySetValidate (uientry_t *entry, uiutilsentryval_t valfunc, void *udata)
{
  entry->validateFunc = valfunc;
  entry->udata = udata;
  g_signal_connect (entry->entry, "changed",
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
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->entry),
        GTK_ENTRY_ICON_SECONDARY, "dialog-error");
  } else {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->entry),
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

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry->entry),
      GTK_ENTRY_ICON_SECONDARY, NULL);
  mstimeset (&entry->validateTimer, 500);
}

