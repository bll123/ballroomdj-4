/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uimiscbutton.h"

uiwcont_t *
uiCreateFontButton (const char *fontname)
{
fprintf (stderr, "c-f-bt\n");
  return NULL;
}

const char *
uiFontButtonGetFont (uiwcont_t *uiwidget)
{
  return NULL;
}

uiwcont_t *
uiCreateColorButton (const char *color)
{
fprintf (stderr, "c-c-bt\n");
  return NULL;
}

void
uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz)
{
  return;
}
