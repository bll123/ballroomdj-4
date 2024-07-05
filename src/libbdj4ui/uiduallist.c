/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "queue.h"
#include "slist.h"
#include "ui.h"
#include "callback.h"
#include "uiduallist.h"
#include "uivirtlist.h"

enum {
  DL_COL_DISP,
  DL_COL_MAX,
};

enum {
  DL_MOVE_PREV,
  DL_MOVE_NEXT,
};

enum {
  DL_BUTTON_SELECT,
  DL_BUTTON_REMOVE,
  DL_BUTTON_MOVE_UP,
  DL_BUTTON_MOVE_DOWN,
  DL_BUTTON_MAX,
};

enum {
  DL_CB_MOVEPREV,
  DL_CB_MOVENEXT,
  DL_CB_SELECT,
  DL_CB_REMOVE,
  DL_CB_MAX,
};

enum {
  DL_MIN_WIDTH = 200,
};

typedef struct uiduallist {
  uivirtlist_t      *uivl [DL_LIST_MAX];
  callback_t        *callbacks [DL_CB_MAX];
  uiwcont_t         *buttons [DL_BUTTON_MAX];
  /* dispq is the display list of strings */
  /* it is used as the source to display in the virtlist */
  queue_t           *dispq [DL_LIST_MAX];
  /* displist is used to look up the keys */
  /* and to retain the values from the caller */
  slist_t           *displist [DL_LIST_MAX];
  int               flags;
  bool              changed : 1;
} uiduallist_t;

static bool uiduallistMovePrev (void *tduallist);
static bool uiduallistMoveNext (void *tduallist);
static void uiduallistMove (uiduallist_t *duallist, int which, int dir);
static bool uiduallistDispSelect (void *udata);
static bool uiduallistDispRemove (void *udata);
static void uiduallistVLFillSourceCB (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uiduallistVLFillTargetCB (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uiduallistVLFillCB (uiduallist_t *duallist, uivirtlist_t *vl, int32_t rownum, int which);
static void uiduallistFreeQKey (void *data);

uiduallist_t *
uiCreateDualList (uiwcont_t *mainvbox, int flags,
    const char *sourcetitle, const char *targettitle)
{
  uiduallist_t  *duallist;
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *dvbox;
  uiwcont_t     *uiwidgetp = NULL;
  uivirtlist_t  *uivl;

  duallist = mdmalloc (sizeof (uiduallist_t));
  for (int i = 0; i < DL_LIST_MAX; ++i) {
    duallist->uivl [i] = NULL;
    duallist->dispq [i] = queueAlloc ("duallist-q", uiduallistFreeQKey);
    duallist->displist [i] = slistAlloc ("duallist-list", LIST_ORDERED, NULL);
  }
  duallist->flags = flags;
  duallist->changed = false;
  for (int i = 0; i < DL_BUTTON_MAX; ++i) {
    duallist->buttons [i] = NULL;
  }
  for (int i = 0; i < DL_CB_MAX; ++i) {
    duallist->callbacks [i] = NULL;
  }

  duallist->callbacks [DL_CB_MOVEPREV] = callbackInit (
      uiduallistMovePrev, duallist, NULL);
  duallist->callbacks [DL_CB_MOVENEXT] = callbackInit (
      uiduallistMoveNext, duallist, NULL);
  duallist->callbacks [DL_CB_SELECT] = callbackInit (
      uiduallistDispSelect, duallist, NULL);
  duallist->callbacks [DL_CB_REMOVE] = callbackInit (
      uiduallistDispRemove, duallist, NULL);

  hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizStart (hbox);
  uiBoxPackStartExpand (mainvbox, hbox);

  vbox = uiCreateVertBox ();
  uiWidgetSetMarginStart (vbox, 8);
  uiWidgetSetMarginTop (vbox, 8);
  uiBoxPackStartExpand (hbox, vbox);

  if (sourcetitle != NULL) {
    uiwidgetp = uiCreateLabel (sourcetitle);
    uiBoxPackStart (vbox, uiwidgetp);
    uiwcontFree (uiwidgetp);
  }

  uivl = uivlCreate ("dl-source", NULL, vbox, 15, DL_MIN_WIDTH,
      VL_NO_HEADING | VL_ENABLE_KEYS);
  duallist->uivl [DL_LIST_SOURCE] = uivl;
  uivlSetDarkBackground (uivl);
  uivlSetNumColumns (uivl, DL_COL_MAX);
  uivlMakeColumn (uivl, "src", DL_COL_DISP, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, DL_COL_DISP, VL_COL_WIDTH_GROW_ONLY);
  uivlSetRowFillCallback (uivl, uiduallistVLFillSourceCB, duallist);

  dvbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (dvbox, 4);
  uiWidgetSetMarginTop (dvbox, 64);
  uiBoxPackStart (hbox, dvbox);

  uiwidgetp = uiCreateButton (duallist->callbacks [DL_CB_SELECT],
      /* CONTEXT: side-by-side list: button: add the selected field */
      _("Select"), "button_right");
  uiBoxPackStart (dvbox, uiwidgetp);
  duallist->buttons [DL_BUTTON_SELECT] = uiwidgetp;

  if ((duallist->flags & DL_FLAGS_PERSISTENT) != DL_FLAGS_PERSISTENT) {
    uiwidgetp = uiCreateButton (duallist->callbacks [DL_CB_REMOVE],
        /* CONTEXT: side-by-side list: button: remove the selected field */
        _("Remove"), "button_left");
    uiBoxPackStart (dvbox, uiwidgetp);
    duallist->buttons [DL_BUTTON_REMOVE] = uiwidgetp;
  }

  uiwcontFree (vbox);
  vbox = uiCreateVertBox ();
  uiWidgetSetMarginStart (vbox, 8);
  uiWidgetSetMarginTop (vbox, 8);
  uiBoxPackStartExpand (hbox, vbox);

  if (targettitle != NULL) {
    uiwidgetp = uiCreateLabel (targettitle);
    uiBoxPackStart (vbox, uiwidgetp);
    uiwcontFree (uiwidgetp);
  }

  uivl = uivlCreate ("dl-target", NULL, vbox, 10, DL_MIN_WIDTH,
      VL_NO_HEADING | VL_ENABLE_KEYS);
  duallist->uivl [DL_LIST_TARGET] = uivl;
  uivlSetDarkBackground (uivl);
  uivlSetNumColumns (uivl, DL_COL_MAX);
  uivlMakeColumn (uivl, "tgt", DL_COL_DISP, VL_TYPE_LABEL);
  uivlSetColumnGrow (uivl, DL_COL_DISP, VL_COL_WIDTH_GROW_ONLY);
  uivlSetRowFillCallback (uivl, uiduallistVLFillTargetCB, duallist);

  uiwcontFree (dvbox);
  dvbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (dvbox, 4);
  uiWidgetSetMarginTop (dvbox, 64);
  uiWidgetAlignVertStart (dvbox);
  uiBoxPackStart (hbox, dvbox);

  uiwidgetp = uiCreateButton (duallist->callbacks [DL_CB_MOVEPREV],
      /* CONTEXT: side-by-side list: button: move the selected field up */
      _("Move Up"), "button_up");
  uiBoxPackStart (dvbox, uiwidgetp);
  duallist->buttons [DL_BUTTON_MOVE_UP] = uiwidgetp;

  uiwidgetp = uiCreateButton (duallist->callbacks [DL_CB_MOVENEXT],
      /* CONTEXT: side-by-side list: button: move the selected field down */
      _("Move Down"), "button_down");
  uiBoxPackStart (dvbox, uiwidgetp);
  duallist->buttons [DL_BUTTON_MOVE_DOWN] = uiwidgetp;

  if ((duallist->flags & DL_FLAGS_PERSISTENT) == DL_FLAGS_PERSISTENT) {
    uiwidgetp = uiCreateButton (duallist->callbacks [DL_CB_REMOVE],
        /* CONTEXT: side-by-side list: button: remove the selected field */
        _("Remove"), "button_remove");
    uiBoxPackStart (dvbox, uiwidgetp);
    duallist->buttons [DL_BUTTON_REMOVE] = uiwidgetp;
  }

  uiwcontFree (dvbox);
  uiwcontFree (vbox);
  uiwcontFree (hbox);

  for (int i = 0; i < DL_LIST_MAX; ++i) {
    uivlDisplay (duallist->uivl [i]);
  }

  return duallist;
}

void
uiduallistFree (uiduallist_t *duallist)
{
  if (duallist != NULL) {
    for (int i = 0; i < DL_LIST_MAX; ++i) {
      uivlFree (duallist->uivl [i]);
      queueFree (duallist->dispq [i]);
      slistFree (duallist->displist [i]);
    }
    for (int i = 0; i < DL_CB_MAX; ++i) {
      callbackFree (duallist->callbacks [i]);
    }
    for (int i = 0; i < DL_BUTTON_MAX; ++i) {
      uiwcontFree (duallist->buttons [i]);
    }
    mdfree (duallist);
  }
}


void
uiduallistSet (uiduallist_t *duallist, slist_t *slist, int which)
{
  const char    *keystr;
  slistidx_t    siteridx;

  if (duallist == NULL) {
    return;
  }

  if (which < 0 || which >= DL_LIST_MAX) {
    return;
  }

  queueClear (duallist->dispq [which], 0);
  slistFree (duallist->displist [which]);
  duallist->displist [which] = slistAlloc ("duallist", LIST_UNORDERED, NULL);

  /* the caller should set the target list first */
  /* so that the source list can be populated correctly */
  slistStartIterator (slist, &siteridx);
  while ((keystr = slistIterateKey (slist, &siteridx)) != NULL) {
    long  val;
    char  *tkeystr;

    val = slistGetNum (slist, keystr);
    if (which == DL_LIST_SOURCE &&
        duallist->displist [DL_LIST_TARGET] != NULL &&
        (duallist->flags & DL_FLAGS_PERSISTENT) != DL_FLAGS_PERSISTENT) {
      if (slistGetNum (duallist->displist [DL_LIST_TARGET], keystr) >= 0) {
        continue;
      }
    }

    slistSetNum (duallist->displist [which], keystr, val);
    tkeystr = mdstrdup (keystr);
    queuePush (duallist->dispq [which], tkeystr);
  }

  slistSort (duallist->displist [which]);

  /* initial number of rows */
  uivlSetNumRows (duallist->uivl [which],
      queueGetCount (duallist->dispq [which]));
  uivlPopulate (duallist->uivl [which]);
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

/* the caller takes ownership of the list */
slist_t *
uiduallistGetList (uiduallist_t *duallist)
{
  slist_t       *slist;
  qidx_t        qiter;
  const char    *keystr;


  slist = slistAlloc ("duallist-return", LIST_UNORDERED, NULL);
  queueStartIterator (duallist->dispq [DL_LIST_TARGET], &qiter);
  while ((keystr =
      queueIterateData (duallist->dispq [DL_LIST_TARGET], &qiter)) != NULL) {
    listnum_t   val;

    val = slistGetNum (duallist->displist [DL_LIST_TARGET], keystr);
    slistSetNum (slist, keystr, val);
  }

  return slist;
}

/* internal routines */

static bool
uiduallistMovePrev (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DL_LIST_TARGET, DL_MOVE_PREV);
  return UICB_CONT;
}

static bool
uiduallistMoveNext (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DL_LIST_TARGET, DL_MOVE_NEXT);
  return UICB_CONT;
}

static void
uiduallistMove (uiduallist_t *duallist, int which, int dir)
{
  int               count;
  int               idx;
  int               toidx = -1;

  if (duallist == NULL) {
    return;
  }
  if (which < 0 || which >= DL_LIST_MAX) {
    return;
  }

  count = uivlSelectionCount (duallist->uivl [which]);
  if (count != 1) {
    return;
  }

  idx = uivlGetCurrSelection (duallist->uivl [which]);
  count = queueGetCount (duallist->dispq [which]);

  /* a move has no effect on duallist->displist, as it is sorted */
  if (idx > 0 && dir == DL_MOVE_PREV) {
    toidx = idx - 1;
  }
  if (idx < count - 1 && dir == DL_MOVE_NEXT) {
    toidx = idx + 1;
  }
  if (toidx < 0 || toidx >= count) {
    return;
  }

  queueMove (duallist->dispq [which], idx, toidx);
  uivlSetSelection (duallist->uivl [which], toidx);

  duallist->changed = true;
  uivlPopulate (duallist->uivl [which]);
}

/* select in the source tree. */
/* add to the target tree. */
/* remove from the source tree. */
static bool
uiduallistDispSelect (void *udata)
{
  uiduallist_t  *duallist = udata;
  int           count;
  char          *keystr;
  char          *tkeystr;
  int           idx;
  int           toidx;
  listnum_t     val;

  count = uivlSelectionCount (duallist->uivl [DL_LIST_SOURCE]);
  if (count != 1) {
    return UICB_CONT;
  }
  if (queueGetCount (duallist->dispq [DL_LIST_SOURCE]) == 0) {
    return UICB_CONT;
  }

  idx = uivlGetCurrSelection (duallist->uivl [DL_LIST_SOURCE]);
  toidx = uivlGetCurrSelection (duallist->uivl [DL_LIST_TARGET]);
  keystr = queueGetByIdx (duallist->dispq [DL_LIST_SOURCE], idx);
  val = slistGetNum (duallist->displist [DL_LIST_SOURCE], keystr);

  tkeystr = mdstrdup (keystr);
  if (toidx + 1 >= queueGetCount (duallist->dispq [DL_LIST_TARGET])) {
    queuePush (duallist->dispq [DL_LIST_TARGET], tkeystr);
  } else {
    queueInsert (duallist->dispq [DL_LIST_TARGET], toidx + 1, tkeystr);
  }
  slistSetNum (duallist->displist [DL_LIST_TARGET], keystr, val);
  uivlSetNumRows (duallist->uivl [DL_LIST_TARGET],
      queueGetCount (duallist->dispq [DL_LIST_TARGET]));
  uivlMoveSelection (duallist->uivl [DL_LIST_TARGET], VL_DIR_NEXT);

  if ((duallist->flags & DL_FLAGS_PERSISTENT) != DL_FLAGS_PERSISTENT) {
    queueRemoveByIdx (duallist->dispq [DL_LIST_SOURCE], idx);
    slistDelete (duallist->displist [DL_LIST_SOURCE], keystr);
    uivlSetNumRows (duallist->uivl [DL_LIST_SOURCE],
        queueGetCount (duallist->dispq [DL_LIST_SOURCE]));
  }

  duallist->changed = true;
  uivlPopulate (duallist->uivl [DL_LIST_SOURCE]);
  uivlPopulate (duallist->uivl [DL_LIST_TARGET]);
  return UICB_CONT;
}

/* select in the target tree. */
/* add to the source tree (in the proper position). */
/* remove from the target tree. */
static bool
uiduallistDispRemove (void *udata)
{
  uiduallist_t  *duallist = udata;
  int           count;
  int           idx;
  int           toidx;
  const char    *keystr;


  count = uivlSelectionCount (duallist->uivl [DL_LIST_TARGET]);
  if (count != 1) {
    return UICB_CONT;
  }
  if (queueGetCount (duallist->dispq [DL_LIST_TARGET]) == 0) {
    return UICB_CONT;
  }

  idx = uivlGetCurrSelection (duallist->uivl [DL_LIST_TARGET]);
  keystr = queueRemoveByIdx (duallist->dispq [DL_LIST_TARGET], idx);
  uivlSetNumRows (duallist->uivl [DL_LIST_TARGET],
      queueGetCount (duallist->dispq [DL_LIST_TARGET]));

  if ((duallist->flags & DL_FLAGS_PERSISTENT) != DL_FLAGS_PERSISTENT) {
    listnum_t   val;

    val = slistGetNum (duallist->displist [DL_LIST_TARGET], keystr);
    slistSetNum (duallist->displist [DL_LIST_SOURCE], keystr, val);
    toidx = slistGetIdx (duallist->displist [DL_LIST_SOURCE], keystr);

    if (toidx == 0) {
      queuePushHead (duallist->dispq [DL_LIST_SOURCE], (void *) keystr);
    } else {
      toidx -= 1;
      if (toidx + 1 >= queueGetCount (duallist->dispq [DL_LIST_SOURCE])) {
        queuePush (duallist->dispq [DL_LIST_SOURCE], (void *) keystr);
      } else {
        queueInsert (duallist->dispq [DL_LIST_SOURCE], toidx + 1, (void *) keystr);
      }
    }
    uivlSetNumRows (duallist->uivl [DL_LIST_SOURCE],
        queueGetCount (duallist->dispq [DL_LIST_SOURCE]));
    uivlSetSelection (duallist->uivl [DL_LIST_SOURCE], toidx + 1);
  }

  slistDelete (duallist->displist [DL_LIST_TARGET], keystr);

  duallist->changed = true;
  uivlPopulate (duallist->uivl [DL_LIST_SOURCE]);
  uivlPopulate (duallist->uivl [DL_LIST_TARGET]);
  return UICB_CONT;
}

static void
uiduallistVLFillSourceCB (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  uiduallistVLFillCB (udata, vl, rownum, DL_LIST_SOURCE);
}

static void
uiduallistVLFillTargetCB (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  uiduallistVLFillCB (udata, vl, rownum, DL_LIST_TARGET);
}

static void
uiduallistVLFillCB (uiduallist_t *duallist, uivirtlist_t *vl, int32_t rownum, int which)
{
  qidx_t        count;
  const char    *keystr;

  if (duallist->dispq [which] == NULL) {
    return;
  }
  if (duallist->uivl [which] == NULL) {
    return;
  }

  count = queueGetCount (duallist->dispq [which]);
  if (rownum >= count) {
    return;
  }

  keystr = queueGetByIdx (duallist->dispq [which], rownum);
  uivlSetRowColumnStr (duallist->uivl [which], rownum,
      DL_COL_DISP, keystr);
}

static void
uiduallistFreeQKey (void *data)
{
  dataFree (data);
}
