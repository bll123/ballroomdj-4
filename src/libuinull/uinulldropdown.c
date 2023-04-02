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

uidropdown_t *
uiDropDownInit (void)
{
  return NULL;
}

void
uiDropDownFree (uidropdown_t *dropdown)
{
  return;
}

uiwcont_t *
uiDropDownCreate (uidropdown_t *dropdown, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  return NULL;
}

uiwcont_t *
uiComboboxCreate (uidropdown_t *dropdown, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  return NULL;
}

void
uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  return;
}

void
uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  return;
}

void
uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx)
{
  return;
}

void
uiDropDownSelectionSetStr (uidropdown_t *dropdown, const char *stridx)
{
  return;
}

void
uiDropDownSetState (uidropdown_t *dropdown, int state)
{
  return;
}

char *
uiDropDownGetString (uidropdown_t *dropdown)
{
  return NULL;
}

