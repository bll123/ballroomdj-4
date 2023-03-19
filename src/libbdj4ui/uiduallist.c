/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "istring.h"
#include "mdebug.h"
#include "slist.h"
#include "ui.h"
#include "callback.h"
#include "uiduallist.h"

enum {
  DUALLIST_COL_DISP,
  DUALLIST_COL_SB_PAD,
  DUALLIST_COL_DISP_IDX,
  DUALLIST_COL_MAX,
};

enum {
  DUALLIST_MOVE_PREV,
  DUALLIST_MOVE_NEXT,
  DUALLIST_SEARCH_INSERT,
  DUALLIST_SEARCH_REMOVE,
};

enum {
  DUALLIST_BUTTON_SELECT,
  DUALLIST_BUTTON_REMOVE,
  DUALLIST_BUTTON_MOVE_UP,
  DUALLIST_BUTTON_MOVE_DOWN,
  DUALLIST_BUTTON_MAX,
};

enum {
  DUALLIST_CB_MOVEPREV,
  DUALLIST_CB_MOVENEXT,
  DUALLIST_CB_SELECT,
  DUALLIST_CB_REMOVE,
  DUALLIST_CB_GET_DATA,
  DUALLIST_CB_SRC_SEARCH,
  DUALLIST_CB_MAX,
};

typedef struct uiduallist {
  uitree_t          *uitrees [DUALLIST_TREE_MAX];
  callback_t        *callbacks [DUALLIST_CB_MAX];
  uibutton_t        *buttons [DUALLIST_BUTTON_MAX];
  int               flags;
  const char        *searchstr;
  int               pos;
  int               searchtype;
  slist_t           *savelist;
  bool              changed : 1;
  bool              searchfound : 1;
} uiduallist_t;

static bool uiduallistMovePrev (void *tduallist);
static bool uiduallistMoveNext (void *tduallist);
static void uiduallistMove (uiduallist_t *duallist, int which, int dir);
static bool uiduallistDispSelect (void *udata);
static bool uiduallistDispRemove (void *udata);
static bool uiduallistSourceSearch (void *udata);
static bool uiduallistGetData (void *udata);
static void uiduallistSetDefaultSelection (uiduallist_t *duallist, int which);

uiduallist_t *
uiCreateDualList (UIWidget *mainvbox, int flags,
    const char *sourcetitle, const char *targettitle)
{
  uiduallist_t  *duallist;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      uiwidget;
  UIWidget      scwindow;
  uibutton_t    *uibutton;
  UIWidget      *uiwidgetp = NULL;
  uitree_t      *uitree;
  UIWidget      *uitreewidgetp = NULL;

  duallist = mdmalloc (sizeof (uiduallist_t));
  for (int i = 0; i < DUALLIST_TREE_MAX; ++i) {
    duallist->uitrees [i] = NULL;
  }
  duallist->flags = flags;
  duallist->pos = 0;
  duallist->searchtype = DUALLIST_SEARCH_INSERT;
  duallist->searchfound = false;
  duallist->searchstr = NULL;
  duallist->savelist = NULL;
  duallist->changed = false;
  for (int i = 0; i < DUALLIST_BUTTON_MAX; ++i) {
    duallist->buttons [i] = NULL;
  }
  for (int i = 0; i < DUALLIST_CB_MAX; ++i) {
    duallist->callbacks [i] = NULL;
  }

  duallist->callbacks [DUALLIST_CB_MOVEPREV] = callbackInit (
      uiduallistMovePrev, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_MOVENEXT] = callbackInit (
      uiduallistMoveNext, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_SELECT] = callbackInit (
      uiduallistDispSelect, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_REMOVE] = callbackInit (
      uiduallistDispRemove, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_GET_DATA] = callbackInit (
      uiduallistGetData, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_SRC_SEARCH] = callbackInit (
      uiduallistSourceSearch, duallist, NULL);

  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&dvbox);
  uiutilsUIWidgetInit (&uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizStart (&hbox);
  uiBoxPackStartExpand (mainvbox, &hbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetMarginStart (&vbox, 8);
  uiWidgetSetMarginTop (&vbox, 8);
  uiBoxPackStartExpand (&hbox, &vbox);

  if (sourcetitle != NULL) {
    uiCreateLabel (&uiwidget, sourcetitle);
    uiBoxPackStart (&vbox, &uiwidget);
  }

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  uitree = uiCreateTreeView ();
  duallist->uitrees [DUALLIST_TREE_SOURCE] = uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);
  uiTreeViewDarkBackground (uitree);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);
  uiTreeViewDisableHeaders (uitree);

  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_GROW, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_DISP, TREE_COL_MODE_END);
  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_NORM, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_SB_PAD, TREE_COL_MODE_END);

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, 4);
  uiWidgetSetMarginTop (&dvbox, 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_SELECT],
      /* CONTEXT: side-by-side list: button: add the selected field */
      _("Select"), "button_right");
  duallist->buttons [DUALLIST_BUTTON_SELECT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_REMOVE],
      /* CONTEXT: side-by-side list: button: remove the selected field */
      _("Remove"), "button_left");
  duallist->buttons [DUALLIST_BUTTON_REMOVE] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uiCreateVertBox (&vbox);
  uiWidgetSetMarginStart (&vbox, 8);
  uiWidgetSetMarginTop (&vbox, 8);
  uiBoxPackStartExpand (&hbox, &vbox);

  if (targettitle != NULL) {
    uiCreateLabel (&uiwidget, targettitle);
    uiBoxPackStart (&vbox, &uiwidget);
  }

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  uitree = uiCreateTreeView ();
  duallist->uitrees [DUALLIST_TREE_TARGET] = uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);
  uiTreeViewDarkBackground (uitree);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);
  uiTreeViewDisableHeaders (uitree);

  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_GROW, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_DISP, TREE_COL_MODE_END);
  uiTreeViewAppendColumn (uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_NORM, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_SB_PAD, TREE_COL_MODE_END);

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, 4);
  uiWidgetSetMarginTop (&dvbox, 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_MOVEPREV],
      /* CONTEXT: side-by-side list: button: move the selected field up */
      _("Move Up"), "button_up");
  duallist->buttons [DUALLIST_BUTTON_MOVE_UP] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_MOVENEXT],
      /* CONTEXT: side-by-side list: button: move the selected field down */
      _("Move Down"), "button_down");
  duallist->buttons [DUALLIST_BUTTON_MOVE_DOWN] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  return duallist;
}

void
uiduallistFree (uiduallist_t *duallist)
{
  if (duallist != NULL) {
    for (int i = 0; i < DUALLIST_TREE_MAX; ++i) {
      uiTreeViewFree (duallist->uitrees [i]);
    }
    for (int i = 0; i < DUALLIST_CB_MAX; ++i) {
      callbackFree (duallist->callbacks [i]);
    }
    for (int i = 0; i < DUALLIST_BUTTON_MAX; ++i) {
      uiButtonFree (duallist->buttons [i]);
    }
    mdfree (duallist);
  }
}


void
uiduallistSet (uiduallist_t *duallist, slist_t *slist, int which)
{
  UIWidget      *uiwidgetp = NULL;
  char          *keystr;
  slistidx_t    siteridx;
  uitree_t      *uitree = NULL;
  uitree_t      *uistree = NULL;

  if (duallist == NULL) {
    return;
  }

  if (which < 0 || which >= DUALLIST_TREE_MAX) {
    return;
  }

  uitree = duallist->uitrees [which];
  uiwidgetp = uiTreeViewGetUIWidget (uitree);
  if (uiwidgetp->widget == NULL) {
    return;
  }

  /* the assumption made is that the source tree has been populated */
  /* just before the target tree */
  uistree = duallist->uitrees [DUALLIST_TREE_SOURCE];

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);

  slistStartIterator (slist, &siteridx);
  while ((keystr = slistIterateKey (slist, &siteridx)) != NULL) {
    long    val;

    val = slistGetNum (slist, keystr);
    uiTreeViewValueAppend (uitree);
    uiTreeViewSetValues (uitree,
        DUALLIST_COL_DISP, keystr,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, (treenum_t) val,
        TREE_VALUE_END);

    /* if inserting into the target tree, and the persistent flag */
    /* is not set, remove the matching entries from the source tree */
    if (which == DUALLIST_TREE_TARGET &&
        (duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
      duallist->pos = 0;
      duallist->searchstr = keystr;
      duallist->searchtype = DUALLIST_SEARCH_REMOVE;
      duallist->searchfound = false;
      /* this is not efficient, but the lists are relatively short */
      uiTreeViewForeach (uistree, duallist->callbacks [DUALLIST_CB_SRC_SEARCH]);

      if (duallist->searchfound) {
        uiTreeViewSelectSave (uistree);
        uiTreeViewSelectSet (uistree, duallist->pos);
        uiTreeViewValueRemove (uistree);
        uiTreeViewSelectRestore (uistree);
      }
    }
  }

  uiduallistSetDefaultSelection (duallist, which);
  uiTreeViewSelectSet (uistree, 0);
}

bool
uiduallistIsChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return false;
  }

  return duallist->changed;
}

void
uiduallistClearChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return;
  }

  duallist->changed = false;
}

slist_t *
uiduallistGetList (uiduallist_t *duallist)
{
  uitree_t      *uittree;
  slist_t       *slist;


  uittree = duallist->uitrees [DUALLIST_TREE_TARGET];
  slist = slistAlloc ("duallist-return", LIST_UNORDERED, NULL);
  duallist->savelist = slist;
  uiTreeViewForeach (uittree, duallist->callbacks [DUALLIST_CB_GET_DATA]);
  return slist;
}

/* internal routines */

static bool
uiduallistMovePrev (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_PREV);
  return UICB_CONT;
}

static bool
uiduallistMoveNext (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_NEXT);
  return UICB_CONT;
}

static void
uiduallistMove (uiduallist_t *duallist, int which, int dir)
{
  uitree_t          *uitree;
  int               count;

  if (duallist == NULL) {
    return;
  }
  if (which < 0 || which >= DUALLIST_TREE_MAX) {
    return;
  }

  uitree = duallist->uitrees [which];

  count = uiTreeViewSelectGetCount (duallist->uitrees [which]);
  if (count != 1) {
    return;
  }

  if (dir == DUALLIST_MOVE_PREV) {
    uiTreeViewMoveBefore (uitree);
  }
  if (dir == DUALLIST_MOVE_NEXT) {
    uiTreeViewMoveAfter (uitree);
  }

  duallist->changed = true;
}

/* select in the source tree. */
/* add to the target tree. */
/* remove from the source tree. */
static bool
uiduallistDispSelect (void *udata)
{
  uiduallist_t      *duallist = udata;
  uitree_t          *uistree;
  int               count;
  uitree_t          *uittree;
  char              *str;
  int               tval;

  uistree = duallist->uitrees [DUALLIST_TREE_SOURCE];

  count = uiTreeViewSelectGetCount (uistree);
  if (count != 1) {
    return UICB_CONT;
  }

  uittree = duallist->uitrees [DUALLIST_TREE_TARGET];
  uiTreeViewSelectCurrent (uittree);

  str = uiTreeViewGetValueStr (uistree, DUALLIST_COL_DISP);
  tval = uiTreeViewGetValue (uistree, DUALLIST_COL_DISP_IDX);

  uiTreeViewValueInsertAfter (uittree);
  uiTreeViewSetValues (uittree,
      DUALLIST_COL_DISP, str,
      DUALLIST_COL_SB_PAD, "    ",
      DUALLIST_COL_DISP_IDX, (treenum_t) tval,
      TREE_VALUE_END);
  dataFree (str);

  if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
    uiTreeViewValueRemove (uistree);
  }
  duallist->changed = true;
  return UICB_CONT;
}

/* select in the target tree. */
/* add to the source tree (in the proper position). */
/* remove from the target tree. */
static bool
uiduallistDispRemove (void *udata)
{
  uiduallist_t  *duallist = udata;
  uitree_t      *uittree;
  uitree_t      *uistree;
  int           count;


  uittree = duallist->uitrees [DUALLIST_TREE_TARGET];
  count = uiTreeViewSelectGetCount (uittree);
  if (count == 0) {
    return UICB_CONT;
  }

  uistree = duallist->uitrees [DUALLIST_TREE_SOURCE];
  uiTreeViewSelectCurrent (uistree);

  if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
    char          *str;
    long          tval;
    callback_t    *cb;

    str = uiTreeViewGetValueStr (uittree, DUALLIST_COL_DISP);
    tval = uiTreeViewGetValue (uittree, DUALLIST_COL_DISP_IDX);

    duallist->pos = 0;
    duallist->searchstr = str;
    duallist->searchtype = DUALLIST_SEARCH_INSERT;
    duallist->searchfound = false;
    cb = callbackInit (uiduallistSourceSearch, duallist, NULL);
    uiTreeViewForeach (uistree, cb);

    if (! duallist->searchfound) {
      duallist->pos = -1;
    }
    uiTreeViewSelectSet (uistree, duallist->pos);
    uiTreeViewValueInsertBefore (uistree);
    uiTreeViewSetValues (uistree,
        DUALLIST_COL_DISP, str,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, (treenum_t) tval,
        TREE_VALUE_END);
    dataFree (str);
  }

  uiTreeViewValueRemove (uittree);
  duallist->changed = true;
  return UICB_CONT;
}

static bool
uiduallistSourceSearch (void *udata)
{
  uiduallist_t  *duallist = udata;
  char          *str;

  str = uiTreeViewGetValueStr (duallist->uitrees [DUALLIST_TREE_SOURCE],
      DUALLIST_COL_DISP);
  if (duallist->searchtype == DUALLIST_SEARCH_INSERT) {
    if (istringCompare (duallist->searchstr, str) < 0) {
      duallist->searchfound = true;
      return TRUE;
    }
  }
  if (duallist->searchtype == DUALLIST_SEARCH_REMOVE) {
    if (istringCompare (duallist->searchstr, str) == 0) {
      duallist->searchfound = true;
      return TRUE;
    }
  }

  dataFree (str);
  duallist->pos += 1;
  return FALSE; // continue iterating
}

static bool
uiduallistGetData (void *udata)
{
  uiduallist_t  *duallist = udata;
  uitree_t      *uittree;
  char          *str;
  long          tval;

  uittree = duallist->uitrees [DUALLIST_TREE_TARGET];
  str = uiTreeViewGetValueStr (uittree, DUALLIST_COL_DISP);
  tval = uiTreeViewGetValue (uittree, DUALLIST_COL_DISP_IDX);
  slistSetNum (duallist->savelist, str, tval);
  dataFree (str);
  return FALSE;     // continue iterating
}


static void
uiduallistSetDefaultSelection (uiduallist_t *duallist, int which)
{
  uitree_t          *uitree;

  if (duallist == NULL) {
    return;
  }
  if (which < 0 || which >= DUALLIST_TREE_MAX) {
    return;
  }

  uitree = duallist->uitrees [which];
  uiTreeViewSelectDefault (uitree);
}
