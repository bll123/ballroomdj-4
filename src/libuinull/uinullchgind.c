/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"

typedef struct uichgind {
  int         junk;
} uichgind_t;

uichgind_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  return NULL;
}

void
uichgindFree (uichgind_t *uichgind)
{
  return;
}

void
uichgindMarkNormal (uichgind_t *uichgind)
{
  return;
}

void
uichgindMarkError (uichgind_t *uichgind)
{
  return;
}

void
uichgindMarkChanged (uichgind_t *uichgind)
{
  return;
}
