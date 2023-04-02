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

uiswitch_t *
uiCreateSwitch (int value)
{
  return NULL;
}

void
uiSwitchFree (uiswitch_t *uiswitch)
{
  return;
}

void
uiSwitchSetValue (uiswitch_t *uiswitch, int value)
{
  return;
}

int
uiSwitchGetValue (uiswitch_t *uiswitch)
{
  return 0;
}

uiwcont_t *
uiSwitchGetWidgetContainer (uiswitch_t *uiswitch)
{
  return NULL;
}

void
uiSwitchSetCallback (uiswitch_t *uiswitch, callback_t *uicb)
{
  return;
}

void
uiSwitchSetState (uiswitch_t *uiswitch, int state)
{
  return;
}

