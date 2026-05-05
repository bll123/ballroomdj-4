/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "mdebug.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

typedef struct uientryint {
  bool            changed;
} uientryint_t;

uiwcont_t *
uiEntryInit_r (int entrySize, int maxSize)
{
  uientryint_t       *uientry;
  uiwcont_t       *uiwidget;
  NSTextField     *widget;

  uientry = mdmalloc (sizeof (uientryint_t));
  uientry->changed = false;

  widget = [[NSTextField alloc] init];
  [widget setBezeled : YES];
  [widget setDrawsBackground : YES];
  [widget setEditable : YES];
  [widget setSelectable : YES];
  widget.translatesAutoresizingMaskIntoConstraints = NO;
  widget.needsDisplay = true;

  uiwidget = uiwcontAlloc (WCONT_T_ENTRY, WCONT_T_ENTRY);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uientry = uientry;

  uiWidgetSetIdentifier (uiwidget, "entry");

//  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->widget), uientry->entrySize);
//  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->widget), uientry->maxSize);
//  gtk_entry_set_input_purpose (GTK_ENTRY (uiwidget->widget), GTK_INPUT_PURPOSE_FREE_FORM);
//  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_START);
//  gtk_widget_set_hexpand (uiwidget->widget, FALSE);

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
  mdfree (uiwidget->uientrygen);
  mdfree (uientry);
  /* the container is freed by uiwcontFree() */
}

void
uiEntrySetWidth (uiwcont_t *uiwidget, int maxWidth)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-width")) {
    return;
  }

  // set-max-width
}

void
uiEntrySetIcon (uiwcont_t *uiwidget, const char *name)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-icon")) {
    return;
  }

  return;
}

void
uiEntryClearIcon (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-clear-icon")) {
    return;
  }

  return;
}

const char *
uiEntryGetValue (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-get-value")) {
    return NULL;
  }

  return NULL;
}

void
uiEntrySetValue (uiwcont_t *uiwidget, const char *value)
{
  NSTextField   *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-value")) {
    return;
  }

  if (value == NULL) {
    value = "";
  }

  widget = uiwidget->widget;
  [widget setStringValue : [NSString stringWithUTF8String : value]];

  return;
}

void
uiEntrySetInternalValidate (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-int-validate")) {
    return;
  }

  return;
}

void
uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-cb")) {
    return;
  }

  return;
}

void
uiEntrySetFocusOutCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-focus-out-cb")) {
    return;
  }

}

void
uiEntrySetState (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-state")) {
    return;
  }

  return;
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

  // gtk_entry_set_alignment (GTK_ENTRY (uiwidget->widget), 1.0);
}


