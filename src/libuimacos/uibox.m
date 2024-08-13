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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"

uiwcont_t *
uiCreateVertBox (void)
{
  return NULL;
}

uiwcont_t *
uiCreateHorizBox (void)
{
  return NULL;
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  return;
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  return;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  return;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  return;
}

void
uiBoxSetSizeChgCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  return;
}
