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

#include "bdj4.h"
#include "bdjstring.h"
#include "callback.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uibutton.h"
#include "ui/uidialog.h"
#include "ui/uidropdown.h"
#include "ui/uitreeview.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uidropdown {
  int         junk;
} uidropdown_t;

uiwcont_t *
uiDropDownInit (void)
{
  return NULL;
}

void
uiDropDownFree (uiwcont_t *uiwidget)
{
  return;
}

uiwcont_t *
uiDropDownCreate (uiwcont_t *uiwidget, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  return NULL;
}

uiwcont_t *
uiComboboxCreate (uiwcont_t *uiwidget, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  return NULL;
}

void
uiDropDownSetList (uiwcont_t *uiwidget, slist_t *list,
    const char *selectLabel)
{
  return;
}

void
uiDropDownSetNumList (uiwcont_t *uiwidget, slist_t *list,
    const char *selectLabel)
{
  return;
}

void
uiDropDownSelectionSetNum (uiwcont_t *uiwidget, nlistidx_t idx)
{
  return;
}

void
uiDropDownSelectionSetStr (uiwcont_t *uiwidget, const char *stridx)
{
  return;
}

void
uiDropDownSetState (uiwcont_t *uiwidget, int state)
{
  return;
}

char *
uiDropDownGetString (uiwcont_t *uiwidget)
{
  return NULL;
}

