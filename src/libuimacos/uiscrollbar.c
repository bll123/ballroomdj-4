/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscrollbar.h"

typedef struct uiscrollbar {
  int         junk;
} uiscrollbar_t;

uiwcont_t *
uiCreateVerticalScrollbar (double upper)
{
  return NULL;
}

void
uiScrollbarFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiScrollbarSetChangeCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiScrollbarSetUpper (uiwcont_t *uiwidget, double upper)
{
  return;
}

void
uiScrollbarSetPosition (uiwcont_t *uiwidget, double pos)
{
  return;
}

void
uiScrollbarSetStepIncrement (uiwcont_t *uiwidget, double step)
{
  return;
}

void
uiScrollbarSetPageIncrement (uiwcont_t *uiwidget, double page)
{
  return;
}

void
uiScrollbarSetPageSize (uiwcont_t *uiwidget, double sz)
{
  return;
}
