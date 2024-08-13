/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "bdj4.h"
#include "callback.h"
#include "mdebug.h"
#include "pathbld.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiswitch.h"

typedef struct uiswitch {
  int       junk;
} uiswitch_t;

uiwcont_t *
uiCreateSwitch (int value)
{
  return NULL;
}

void
uiSwitchFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiSwitchSetValue (uiwcont_t *uiwidget, int value)
{
  return;
}

int
uiSwitchGetValue (uiwcont_t *uiwidget)
{
  return 0;
}

void
uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}
