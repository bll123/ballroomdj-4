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

typedef struct uientry {
  int         junk;
} uientry_t;

uiwcont_t *
uiEntryInit (int entrySize, int maxSize)
{
  uiwcont_t   *uiwidget;
  uientry_t   *uientry;
  NSTextField *widget;

fprintf (stderr, "c-entry\n");
  uientry = mdmalloc (sizeof (uientry_t));
//  uientry->entrySize = entrySize;
//  uientry->maxSize = maxSize;
//  uientry->buffer = NULL;
//  uientry->validateFunc = NULL;
//  uientry->label = "";
//  uientry->udata = NULL;
//  mstimeset (&uientry->validateTimer, TM_TIMER_OFF);
//  uientry->valdelay = false;
//  uientry->valid = true;
//  uientry->buffer = gtk_entry_buffer_new (NULL, -1);

  widget = [[NSTextField alloc] init];

  uiwidget = uiwcontAlloc (WCONT_T_ENTRY, WCONT_T_ENTRY);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uientry = uientry;

//  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->uidata.widget), uientry->entrySize);
//  gtk_entry_set_max_length (GTK_ENTRY (uiwidget->uidata.widget), uientry->maxSize);
//  gtk_entry_set_input_purpose (GTK_ENTRY (uiwidget->uidata.widget), GTK_INPUT_PURPOSE_FREE_FORM);
//  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_START);
//  gtk_widget_set_hexpand (uiwidget->uidata.widget, FALSE);

  return uiwidget;
}

void
uiEntryFree (uiwcont_t *uientry)
{
  return;
}

void
uiEntrySetIcon (uiwcont_t *uientry, const char *name)
{
  return;
}

void
uiEntryClearIcon (uiwcont_t *uientry)
{
  return;
}

const char *
uiEntryGetValue (uiwcont_t *uientry)
{
  return NULL;
}

void
uiEntrySetValue (uiwcont_t *uientry, const char *value)
{
  return;
}

void
uiEntrySetValidate (uiwcont_t *uientry, const char *label,
    uientryval_t valfunc, void *udata, int valdelay)
{
  return;
}

int
uiEntryValidate (uiwcont_t *uientry, bool forceflag)
{
  return 0;
}

int
uiEntryValidateDir (uiwcont_t *uientry, const char *label, void *udata)
{
  return 0;
}

int
uiEntryValidateFile (uiwcont_t *uientry, const char *label, void *udata)
{
  return 0;
}

void
uiEntrySetState (uiwcont_t *uientry, int state)
{
  return;
}

void
uiEntryValidateClear (uiwcont_t *uiwidget)
{
  return;
}

void
uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

bool
uiEntryIsNotValid (uiwcont_t *uiwidget)
{
  return false;
}
