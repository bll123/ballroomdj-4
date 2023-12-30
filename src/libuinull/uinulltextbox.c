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

uiwcont_t *
uiTextBoxCreate (int height, const char *hlcolor)
{
  return NULL;
}

void
uiTextBoxFree (uiwcont_t *uiwidget)
{
  return;
}

uiwcont_t *
uiTextBoxGetScrolledWindow (uiwcont_t *uiwidget)
{
  return NULL;
}

void
uiTextBoxSetReadonly (uiwcont_t *uiwidget)
{
  return;
}

char *
uiTextBoxGetValue (uiwcont_t *uiwidget)
{
  return NULL;
}

void
uiTextBoxScrollToEnd (uiwcont_t *uiwidget)
{
  return;
}

void
uiTextBoxAppendStr (uiwcont_t *uiwidget, const char *str)
{
  return;
}

void
uiTextBoxAppendBoldStr (uiwcont_t *uiwidget, const char *str)
{
  return;
}

void
uiTextBoxAppendHighlightStr (uiwcont_t *uiwidget, const char *str)
{
  return;
}

void
uiTextBoxSetValue (uiwcont_t *uiwidget, const char *str)
{
  return;
}

/* this does not handle any selected text */

void
uiTextBoxDarken (uiwcont_t *uiwidget)
{
  return;
}

void
uiTextBoxHorizExpand (uiwcont_t *uiwidget)
{
  return;
}

void
uiTextBoxVertExpand (uiwcont_t *uiwidget)
{
  return;
}

void
uiTextBoxSetHeight (uiwcont_t *uiwidget, int h)
{
  return;
}

void
uiTextBoxSetParagraph (uiwcont_t *uiwidget, int indent, int interpara)
{
  return;
}
