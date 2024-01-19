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
#include <stdarg.h>

#include "ui/uiwcont-int.h"

#include "ui/uidialog.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uiselect {
    int         junk;
} uiselect_t;

char *
uiSelectDirDialog (uiselect_t *selectdata)
{
  return NULL;
}

char *
uiSelectFileDialog (uiselect_t *selectdata)
{
  return NULL;
}

char *
uiSaveFileDialog (uiselect_t *selectdata)
{
  return NULL;
}

uiwcont_t *
uiCreateDialog (uiwcont_t *window,
    callback_t *uicb, const char *title, ...)
{
  return NULL;
}

void
uiDialogShow (uiwcont_t *uiwidgetp)
{
  return;
}

void
uiDialogAddButtons (uiwcont_t *uidialog, ...)
{
  return;
}

void
uiDialogPackInDialog (uiwcont_t *uidialog, uiwcont_t *boxp)
{
  return;
}

void
uiDialogDestroy (uiwcont_t *uidialog)
{
  return;
}

uiselect_t *
uiDialogCreateSelect (uiwcont_t *window, const char *label,
    const char *startpath, const char *dfltname,
    const char *mimefiltername, const char *mimetype)
{
  return NULL;
}

