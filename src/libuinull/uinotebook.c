/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
uiCreateNotebook (void)
{
  return NULL;
}

void
uiNotebookTabPositionLeft (uiwcont_t *uinotebook)
{
  return;
}

void
uiNotebookAppendPage (uiwcont_t *uinotebook, uiwcont_t *uiwidget,
    uiwcont_t *uilabel)
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
uiNotebookSetScrollable (uiwcont_t *uinotebook)
{
  return;
}
