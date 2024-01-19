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

uientry_t *
uiEntryInit (int entrySize, int maxSize)
{
  return NULL;
}


void
uiEntryFree (uientry_t *uientry)
{
  return;
}

void
uiEntryCreate (uientry_t *uientry)
{
  return;
}

void
uiEntrySetIcon (uientry_t *uientry, const char *name)
{
  return;
}

void
uiEntryClearIcon (uientry_t *uientry)
{
  return;
}

uiwcont_t *
uiEntryGetWidgetContainer (uientry_t *uientry)
{
  return NULL;
}

void
uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry)
{
  return;
}

const char *
uiEntryGetValue (uientry_t *uientry)
{
  return NULL;
}

void
uiEntrySetValue (uientry_t *uientry, const char *value)
{
  return;
}

void
uiEntrySetValidate (uientry_t *uientry, uientryval_t valfunc, void *udata,
    int valdelay)
{
  return;
}

int
uiEntryValidate (uientry_t *uientry, bool forceflag)
{
  return 0;
}

int
uiEntryValidateDir (uientry_t *uientry, void *udata)
{
  return 0;
}

int
uiEntryValidateFile (uientry_t *uientry, void *udata)
{
  return 0;
}

void
uiEntrySetState (uientry_t *uientry, int state)
{
  return;
}
