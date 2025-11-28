/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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
  return NULL;
}

void
uiEntryFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-free")) {
    return;
  }

  return;
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
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-set-value")) {
    return;
  }

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

bool
uiEntryChanged (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-changed")) {
    return false;
  }

  return false;
}

void
uiEntryClearChanged (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_ENTRY, "entry-changed")) {
    return;
  }
}

