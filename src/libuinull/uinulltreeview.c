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

typedef struct uitree {
  int         junk;
} uitree_t;

uitree_t *
uiCreateTreeView (void)
{
  return NULL;
}

void
uiTreeViewFree (uitree_t *uitree)
{
  return;
}

int
uiTreeViewGetDragDropRow (uiwcont_t *uiwcont, int x, int y)
{
  return 0;
}

void
uiTreeViewEnableHeaders (uitree_t *uitree)
{
  return;
}

void
uiTreeViewDisableHeaders (uitree_t *uitree)
{
  return;
}

void
uiTreeViewDarkBackground (uitree_t *uitree)
{
  return;
}

void
uiTreeViewDisableSingleClick (uitree_t *uitree)
{
  return;
}

void
uiTreeViewSelectSetMode (uitree_t *uitree, int mode)
{
  return;
}

void
uiTreeViewSetSelectChangedCallback (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewSetSizeChangeCallback (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewSetScrollEventCallback (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewSetRowActivatedCallback (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewRadioSetRow (uitree_t *uitree, int row)
{
  return;
}

uiwcont_t *
uiTreeViewGetWidgetContainer (uitree_t *uitree)
{
  return NULL;
}

void
uiTreeViewPreColumnSetMinWidth (uitree_t *uitree, int minwidth)
{
  return;
}

void
uiTreeViewPreColumnSetEllipsizeColumn (uitree_t *uitree, int ellipsizeColumn)
{
  return;
}

void
uiTreeViewPreColumnSetColorColumn (uitree_t *uitree, int colcol, int colsetcol)
{
  return;
}

void
uiTreeViewAppendColumn (uitree_t *uitree, int activecol, int widgettype,
    int alignment, int coldisp, const char *title, ...)
{
  return;
}

void
uiTreeViewColumnSetVisible (uitree_t *uitree, int col, int flag)
{
  return;
}

void
uiTreeViewCreateValueStore (uitree_t *uitree, int colmax, ...)
{
  return;
}

void
uiTreeViewCreateValueStoreFromList (uitree_t *uitree, int colmax, int *typelist)
{
  return;
}

void
uiTreeViewValueAppend (uitree_t *uitree)
{
  return;
}

void
uiTreeViewValueInsertBefore (uitree_t *uitree)
{
  return;
}

void
uiTreeViewValueInsertAfter (uitree_t *uitree)
{
  return;
}

void
uiTreeViewValueRemove (uitree_t *uitree)
{
  return;
}

void
uiTreeViewValueClear (uitree_t *uitree, int startrow)
{
  return;
}

/* must be called before set-values if the value iter is being used */
void
uiTreeViewSetValueEllipsize (uitree_t *uitree, int col)
{
  return;
}

void
uiTreeViewSetValues (uitree_t *uitree, ...)
{
  return;
}

int
uiTreeViewSelectGetCount (uitree_t *uitree)
{
  return 0;
}

int
uiTreeViewSelectGetIndex (uitree_t *uitree)
{
  return 0;
}

/* makes sure that the stored selectiter is pointing to the current selection */
/* coded for both select-mode single and multiple */
/* makes sure the iterator is actually selected and selectiter is set */
void
uiTreeViewSelectCurrent (uitree_t *uitree)
{
  return;
}

bool
uiTreeViewSelectFirst (uitree_t *uitree)
{
  return false;
}

bool
uiTreeViewSelectNext (uitree_t *uitree)
{
  return false;
}

bool
uiTreeViewSelectPrevious (uitree_t *uitree)
{
  return false;
}

void
uiTreeViewSelectDefault (uitree_t *uitree)
{
  return;
}

void
uiTreeViewSelectSave (uitree_t *uitree)
{
  return;
}

void
uiTreeViewSelectRestore (uitree_t *uitree)
{
  return;
}

void
uiTreeViewSelectClear (uitree_t *uitree)
{
  return;
}

/* use when the select mode is select-multiple */
void
uiTreeViewSelectClearAll (uitree_t *uitree)
{
  return;
}

void
uiTreeViewSelectForeach (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewMoveBefore (uitree_t *uitree)
{
  return;
}

void
uiTreeViewMoveAfter (uitree_t *uitree)
{
  return;
}

/* gets the value for the selected row */
long
uiTreeViewGetValue (uitree_t *uitree, int col)
{
  return 0;
}

/* gets the string value for the selected row */
char *
uiTreeViewGetValueStr (uitree_t *uitree, int col)
{
  return NULL;
}

/* gets the value for the row just processed by select-foreach */
long
uiTreeViewSelectForeachGetValue (uitree_t *uitree, int col)
{
  return 0;
}

void
uiTreeViewForeach (uitree_t *uitree, callback_t *cb)
{
  return;
}

void
uiTreeViewSelectSet (uitree_t *uitree, int row)
{
  return;
}

void
uiTreeViewValueIteratorSet (uitree_t *uitree, int row)
{
  return;
}

void
uiTreeViewValueIteratorClear (uitree_t *uitree)
{
  return;
}

void
uiTreeViewScrollToCell (uitree_t *uitree)
{
  return;
}

void
uiTreeViewAttachScrollController (uitree_t *uitree, double upper)
{
  return;
}

