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

#include "ui/uiwcont-int.h"

#include "ui/uiclipboard.h"

void
uiClipboardSet (const char *txt)
{
  GtkClipboard  *cb;

  // GDK_SELECTION_PRIMARY
  // GDK_SELECTION_SECONDARY
  cb = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (cb, txt, -1);
  cb = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  gtk_clipboard_set_text (cb, txt, -1);
}
