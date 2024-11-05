/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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


uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
fprintf (stderr, "c-chgind\n");
  return NULL;
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  return;
}

void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  return;
}
