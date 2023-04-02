/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

uiscrollbar_t *
uiCreateVerticalScrollbar (double upper)
{
  return NULL;
}

void
uiScrollbarFree (uiscrollbar_t *sb)
{
  return;
}

uiwcont_t *
uiScrollbarGetWidgetContainer (uiscrollbar_t *sb)
{
  return NULL;
}

void
uiScrollbarSetChangeCallback (uiscrollbar_t *sb, callback_t *cb)
{
  return;
}

void
uiScrollbarSetUpper (uiscrollbar_t *sb, double upper)
{
  return;
}

void
uiScrollbarSetPosition (uiscrollbar_t *sb, double pos)
{
  return;
}

void
uiScrollbarSetStepIncrement (uiscrollbar_t *sb, double step)
{
  return;
}

void
uiScrollbarSetPageIncrement (uiscrollbar_t *sb, double page)
{
  return;
}

void
uiScrollbarSetPageSize (uiscrollbar_t *sb, double sz)
{
  return;
}
