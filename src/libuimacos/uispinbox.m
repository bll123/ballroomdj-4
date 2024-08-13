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

#include "bdj4intl.h"
#include "callback.h"
#include "mdebug.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uievents.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uispinbox.h"

typedef struct uispinbox {
  int         junk;
} uispinbox_t;


void
uiSpinboxFree (uiwcont_t *uiwidget)
{
  return;
}


uiwcont_t *
uiSpinboxTextCreate (void *udata)
{
  return NULL;
}

void
uiSpinboxTextSet (uiwcont_t *uiwidget, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  return;
}

int
uiSpinboxTextGetValue (uiwcont_t *uiwidget)
{
  return 0;
}

void
uiSpinboxTextSetValue (uiwcont_t *uiwidget, int value)
{
  return;
}

void
uiSpinboxSetState (uiwcont_t *uiwidget, int state)
{
  return;
}

uiwcont_t *
uiSpinboxTimeCreate (int sbtype, void *udata,
    const char *label, callback_t *convcb)
{
  return NULL;
}

int32_t
uiSpinboxTimeGetValue (uiwcont_t *uiwidget)
{
  return 0;
}

void
uiSpinboxTimeSetValue (uiwcont_t *uiwidget, int32_t value)
{
  return;
}

void
uiSpinboxTextSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiSpinboxTimeSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiSpinboxSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

uiwcont_t *
uiSpinboxIntCreate (void)
{
  return NULL;
}

uiwcont_t *
uiSpinboxDoubleCreate (void)
{
  return NULL;
}

uiwcont_t *
uiSpinboxDoubleDefaultCreate (void)
{
  return NULL;
}

void
uiSpinboxSetRange (uiwcont_t *uiwidget, double min, double max)
{
  return;
}

void
uiSpinboxSetIncrement (uiwcont_t *spinbox, double incr, double pageincr)
{
  return;
}

void
uiSpinboxWrap (uiwcont_t *uiwidget)
{
  return;
}

void
uiSpinboxSet (uiwcont_t *uispinbox, double min, double max)
{
  return;
}

double
uiSpinboxGetValue (uiwcont_t *uispinbox)
{
  return 0.0;
}

void
uiSpinboxSetValue (uiwcont_t *uispinbox, double value)
{
  return;
}

bool
uiSpinboxIsChanged (uiwcont_t *uiwidget)
{
  return false;
}

void
uiSpinboxResetChanged (uiwcont_t *uiwidget)
{
  return;
}

void
uiSpinboxAddClass (const char *classnm, const char *color)
{
  return;
}

void
uiSpinboxSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}
