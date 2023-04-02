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

#include "bdj4intl.h"
#include "callback.h"
#include "mdebug.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uikeys.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uispinbox.h"

typedef struct uispinbox {
  int         junk;
} uispinbox_t;

uispinbox_t *
uiSpinboxInit (void)
{
  return NULL;
}


void
uiSpinboxFree (uispinbox_t *uispinbox)
{
  return;
}


void
uiSpinboxTextCreate (uispinbox_t *uispinbox, void *udata)
{
  return;
}

void
uiSpinboxTextSet (uispinbox_t *uispinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  return;
}

int
uiSpinboxTextGetIdx (uispinbox_t *uispinbox)
{
  return 0;
}

int
uiSpinboxTextGetValue (uispinbox_t *uispinbox)
{
  return 0;
}

void
uiSpinboxTextSetValue (uispinbox_t *uispinbox, int value)
{
  return;
}

void
uiSpinboxSetState (uispinbox_t *uispinbox, int state)
{
  return;
}

uispinbox_t *
uiSpinboxTimeInit (int sbtype)
{
  return NULL;
}

void
uiSpinboxTimeCreate (uispinbox_t *uispinbox, void *udata, callback_t *convcb)
{
  return;
}

ssize_t
uiSpinboxTimeGetValue (uispinbox_t *uispinbox)
{
  return 0;
}

void
uiSpinboxTimeSetValue (uispinbox_t *uispinbox, ssize_t value)
{
  return;
}

void
uiSpinboxTextSetValueChangedCallback (uispinbox_t *uispinbox, callback_t *uicb)
{
  return;
}

void
uiSpinboxTimeSetValueChangedCallback (uispinbox_t *uispinbox, callback_t *uicb)
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

void
uiSpinboxDoubleDefaultCreate (uispinbox_t *uispinbox)
{
  return;
}

void
uiSpinboxSetRange (uispinbox_t *uispinbox, double min, double max)
{
  return;
}


void
uiSpinboxWrap (uispinbox_t *uispinbox)
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
uiSpinboxIsChanged (uispinbox_t *uispinbox)
{
  return false;
}

void
uiSpinboxResetChanged (uispinbox_t *uispinbox)
{
  return;
}

void
uiSpinboxAlignRight (uispinbox_t *uispinbox)
{
  return;
}

void
uiSpinboxAddClass (const char *classnm, const char *color)
{
  return;
}

uiwcont_t *
uiSpinboxGetWidgetContainer (uispinbox_t *uispinbox)
{
  return NULL;
}
