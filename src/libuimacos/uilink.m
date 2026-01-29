/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uilink.h"

uiwcont_t *
uiCreateLink (const char *label, const char *uri)
{
  uiwcont_t   *uiwidget;
  ILabel      *widget;

fprintf (stderr, "c-link\n");
  widget = [[ILabel alloc] init];
  [widget setBezeled : NO];
  [widget setDrawsBackground : NO];
  [widget setEditable : NO];
  [widget setSelectable : NO];
  [widget setStringValue : [NSString stringWithUTF8String : label]];
  [widget setTranslatesAutoresizingMaskIntoConstraints : NO];

  uiwidget = uiwcontAlloc (WCONT_T_LINK, WCONT_T_LINK);
  uiwcontSetWidget (uiwidget, widget, NULL);
  return NULL;
}

void
uiLinkSet (uiwcont_t *uilink, const char *label, const char *uri)
{
  return;
}

void
uiLinkSetActivateCallback (uiwcont_t *uilink, callback_t *uicb)
{
  return;
}
