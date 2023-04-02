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

#include "mdebug.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uitextbox.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uitextbox {
  int       junk;
} uitextbox_t;

uitextbox_t *
uiTextBoxCreate (int height, const char *hlcolor)
{
  return NULL;
}

void
uiTextBoxFree (uitextbox_t *tb)
{
  return;
}

uiwcont_t *
uiTextBoxGetScrolledWindow (uitextbox_t *tb)
{
  return NULL;
}

void
uiTextBoxSetReadonly (uitextbox_t *tb)
{
  return;
}

char *
uiTextBoxGetValue (uitextbox_t *tb)
{
  return NULL;
}

void
uiTextBoxScrollToEnd (uitextbox_t *tb)
{
  return;
}

void
uiTextBoxAppendStr (uitextbox_t *tb, const char *str)
{
  return;
}

void
uiTextBoxAppendBoldStr (uitextbox_t *tb, const char *str)
{
  return;
}

void
uiTextBoxAppendHighlightStr (uitextbox_t *tb, const char *str)
{
  return;
}

void
uiTextBoxSetValue (uitextbox_t *tb, const char *str)
{
  return;
}

/* this does not handle any selected text */

void
uiTextBoxDarken (uitextbox_t *tb)
{
  return;
}

void
uiTextBoxHorizExpand (uitextbox_t *tb)
{
  return;
}

void
uiTextBoxVertExpand (uitextbox_t *tb)
{
  return;
}

void
uiTextBoxSetHeight (uitextbox_t *tb, int h)
{
  return;
}

