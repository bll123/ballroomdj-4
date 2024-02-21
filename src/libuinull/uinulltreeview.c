/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "callback.h"
#include "mdebug.h"
#include "tagdef.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uitreeview.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

typedef struct {
  int         junk;
} uitree_t;

uitree_t *
uiCreateTreeView (void)
{
  return NULL;
}

void
uiTreeViewFree (uiwcont_t *uiwidget)
{
  return;
}

int
uiTreeViewGetDragDropRow (uiwcont_t *uiwcont, int x, int y)
{
  return 0;
}

void
uiTreeViewEnableHeaders (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewDisableHeaders (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewDarkBackground (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewDisableSingleClick (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewSelectSetMode (uiwcont_t *uiwidget, int mode)
{
  return;
}

void
uiTreeViewSetSelectChangedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewSetSizeChangeCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewSetScrollEventCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewSetRowActivatedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewSetEditedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewRadioSetRow (uiwcont_t *uiwidget, int row)
{
  return;
}

void
uiTreeViewPreColumnSetMinWidth (uiwcont_t *uiwidget, int minwidth)
{
  return;
}

void
uiTreeViewPreColumnSetEllipsizeColumn (uiwcont_t *uiwidget, int ellipsizeColumn)
{
  return;
}

void
uiTreeViewPreColumnSetColorColumn (uiwcont_t *uiwidget, int colcol, int colsetcol)
{
  return;
}

void
uiTreeViewAppendColumn (uiwcont_t *uiwidget, int activecol, int widgettype,
    int alignment, int coldisp, const char *title, ...)
{
  return;
}

void
uiTreeViewColumnSetVisible (uiwcont_t *uiwidget, int col, int flag)
{
  return;
}

void
uiTreeViewCreateValueStore (uiwcont_t *uiwidget, int colmax, ...)
{
  return;
}

void
uiTreeViewCreateValueStoreFromList (uiwcont_t *uiwidget, int colmax, int *typelist)
{
  return;
}

void
uiTreeViewValueAppend (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewValueInsertBefore (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewValueInsertAfter (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewValueRemove (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewValueClear (uiwcont_t *uiwidget, int startrow)
{
  return;
}

/* must be called before set-values if the value iter is being used */
void
uiTreeViewSetValueEllipsize (uiwcont_t *uiwidget, int col)
{
  return;
}

void
uiTreeViewSetValues (uiwcont_t *uiwidget, ...)
{
  return;
}

int
uiTreeViewSelectGetCount (uiwcont_t *uiwidget)
{
  return 0;
}

int
uiTreeViewSelectGetIndex (uiwcont_t *uiwidget)
{
  return 0;
}

/* makes sure that the stored selectiter is pointing to the current selection */
/* coded for both select-mode single and multiple */
/* makes sure the iterator is actually selected and selectiter is set */
void
uiTreeViewSelectCurrent (uiwcont_t *uiwidget)
{
  return;
}

bool
uiTreeViewSelectFirst (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiTreeViewSelectNext (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiTreeViewSelectPrevious (uiwcont_t *uiwidget)
{
  return false;
}

void
uiTreeViewSelectDefault (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewSelectSave (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewSelectRestore (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewSelectClear (uiwcont_t *uiwidget)
{
  return;
}

/* use when the select mode is select-multiple */
void
uiTreeViewSelectClearAll (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewSelectForeach (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewMoveBefore (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewMoveAfter (uiwcont_t *uiwidget)
{
  return;
}

/* gets the value for the selected row */
long
uiTreeViewGetValue (uiwcont_t *uiwidget, int col)
{
  return 0;
}

/* gets the string value for the selected row */
char *
uiTreeViewGetValueStr (uiwcont_t *uiwidget, int col)
{
  return NULL;
}

/* gets the value for the row just processed by select-foreach */
long
uiTreeViewSelectForeachGetValue (uiwcont_t *uiwidget, int col)
{
  return 0;
}

void
uiTreeViewForeach (uiwcont_t *uiwidget, callback_t *cb)
{
  return;
}

void
uiTreeViewSelectSet (uiwcont_t *uiwidget, int row)
{
  return;
}

void
uiTreeViewValueIteratorSet (uiwcont_t *uiwidget, int row)
{
  return;
}

void
uiTreeViewValueIteratorClear (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewScrollToCell (uiwcont_t *uiwidget)
{
  return;
}

void
uiTreeViewAttachScrollController (uiwcont_t *uiwidget, double upper)
{
  return;
}

