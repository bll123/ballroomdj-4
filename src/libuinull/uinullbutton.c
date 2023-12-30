/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
#include "ui/uibutton.h"

typedef struct uibutton {
  int         junk;
} uibutton_t;

uiwcont_t *
uiCreateButton (callback_t *uicb, char *title, char *imagenm)
{
  return NULL;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetImage (uiwcont_t *uiwidget, const char *imagenm,
    const char *tooltip)
{
  return;
}

void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
  return;
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetReliefNone (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  return;
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  return;
}

bool
uiButtonCheckRepeat (uiwcont_t *uiwidget)
{
  return false;
}
