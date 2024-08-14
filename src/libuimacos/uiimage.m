/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"

uiwcont_t *
uiImageNew (void)
{
  return NULL;
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t *uiwidget = NULL;

  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  return NULL;
}

void
uiImageClear (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-clr")) {
    return;
  }

  return;
}

void
uiImageConvertToPixbuf (uiwcont_t *uiwidget)
{
  return;
}

void
uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf)
{
  return;
}
