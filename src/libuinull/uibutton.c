/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
uiCreateButton (callback_t *uicb, const char *title,
    const char *imagenm, const char *tooltiptxt)
{
  return NULL;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-free")) {
    return;
  }

  return;
}

void
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-pos-r")) {
    return;
  }

  return;
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-margin-top")) {
    return;
  }

  return;
}

void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-icon")) {
    return;
  }

  return;
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-align-left")) {
    return;
  }

  return;
}

void
uiButtonSetReliefNone (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-relief-none")) {
    return;
  }

  return;
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-flat")) {
    return;
  }

  return;
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-text")) {
    return;
  }

  return;
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  return;
}
