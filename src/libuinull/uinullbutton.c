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

uibutton_t *
uiCreateButton (callback_t *uicb, char *title, char *imagenm)
{
  return NULL;
}

void
uiButtonFree (uibutton_t *uibutton)
{
  return;
}

uiwcont_t *
uiButtonGetWidgetContainer (uibutton_t *uibutton)
{
  return NULL;
}

void
uiButtonSetImagePosRight (uibutton_t *uibutton)
{
  return;
}

void
uiButtonSetImage (uibutton_t *uibutton, const char *imagenm,
    const char *tooltip)
{
  return;
}

void
uiButtonSetImageIcon (uibutton_t *uibutton, const char *nm)
{
  return;
}

void
uiButtonAlignLeft (uibutton_t *uibutton)
{
  return;
}

void
uiButtonSetReliefNone (uibutton_t *uibutton)
{
  return;
}

void
uiButtonSetFlat (uibutton_t *uibutton)
{
  return;
}

void
uiButtonSetText (uibutton_t *uibutton, const char *txt)
{
  return;
}

void
uiButtonSetRepeat (uibutton_t *uibutton, int repeatms)
{
  return;
}

bool
uiButtonCheckRepeat (uibutton_t *uibutton)
{
  return false;
}

void
uiButtonSetState (uibutton_t *uibutton, int state)
{
  return;
}

