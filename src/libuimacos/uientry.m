/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
