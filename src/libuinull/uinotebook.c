/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "ui/uiui.h"
#include "ui/uinotebook.h"

uiwcont_t *
uiCreateNotebook (const char *ident)
{
  return NULL;
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget,
    const char *label, uiwcont_t *image)
{
  return;
}

void
uiNotebookSetActionWidget (uiwcont_t *uinotebook, uiwcont_t *uiwidget)
{
  return;
}

void
uiNotebookSetPage (uiwcont_t *uinotebook, int pagenum)
{
  return;
}

void
uiNotebookHideShowPage (uiwcont_t *uinotebook, int pagenum, bool show)
{
  return;
}

void
uiNotebookSetCallback (uiwcont_t *uinotebook, callback_t *uicb)
{
  return;
}

void
uiNotebookHideTabs (uiwcont_t *uinotebook)
{
  if (! uiwcontValid (uinotebook, WCONT_T_NOTEBOOK, "nb-tabs")) {
    return;
  }
}

