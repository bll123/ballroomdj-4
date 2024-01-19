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

#include "ui/uiwcont-int.h"

#include "ui/uidragdrop.h"

void
uiDragDropSetDestURICallback (uiwcont_t *uiwcont, callback_t *cb)
{
  if (uiwcont == NULL) {
    return;
  }
  if (uiwcont->widget == NULL) {
    return;
  }
}

