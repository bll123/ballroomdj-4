/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uientry.h"

uiwcont_t *
uiEntryInit (int entrySize, int maxSize)
{
  uiwcont_t       *uiwidget;
  NSTextField     *widget;
  uientrybase_t   *ebase;

fprintf (stderr, "c-entry\n");
  widget = [[NSTextField alloc] init];
  [widget setBezeled:YES];
  [widget setDrawsBackground:YES];
  [widget setEditable:YES];
  [widget setSelectable:YES];
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_ENTRY, WCONT_T_ENTRY);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uientry = NULL;

  ebase = &uiwidget->uiint.uientrybase;
  ebase->entrySize = entrySize;
  ebase->maxSize = maxSize;
  ebase->validateFunc = NULL;
  ebase->label = "";
  ebase->udata = NULL;
  mstimeset (&ebase->validateTimer, TM_TIMER_OFF);
  ebase->valdelay = false;
  ebase->valid = true;

//  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->uidata.widget), uientry->entrySize);
//  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->uidata.widget), uientry->maxSize);
//  gtk_entry_set_input_purpose (GTK_ENTRY (uiwidget->uidata.widget), GTK_INPUT_PURPOSE_FREE_FORM);
//  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_START);
//  gtk_widget_set_hexpand (uiwidget->uidata.widget, FALSE);

  return uiwidget;
}

void
uiEntryFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-free")) {
    return;
  }

  /* the container is freed by uiwcontFree() */
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

  widget = uiwidget->uidata.widget;
  [widget setStringValue: [NSString stringWithUTF8String: value]];

  return;
}

void
uiEntrySetState (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-state")) {
    return;
  }

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
