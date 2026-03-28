/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 *
 * spinbox-text
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "ui.h"
#include "uisb.h"
#include "uisbtext.h"
#include "uiwcont.h"

typedef struct uisbtext {
  uisb_t          *sb;
  uiwcont_t       *display;
  nlist_t         *txtlist;
  nlist_t         *idxlist;
  callback_t      *sbtextcb;
  callback_t      *chgcb;
  uisbtextdisp_t  dispcb;
  void            *udata;
  const char      *prepend_text;
  int32_t         old_value;
  int32_t         index;
  int32_t         count;
  bool            changed;
  bool            force;
} uisbtext_t;

static bool uisbtextCBHandler (void *udata, int32_t dir);
static void uisbtextSetDisplay (uisbtext_t *sbtext);

uisbtext_t *
uisbtextCreate (uiwcont_t *box, int margin)
{
  uisbtext_t  *sbtext;

  sbtext = mdmalloc (sizeof (uisbtext_t));
  sbtext->display = uiCreateLabel ("");
  uiWidgetSetAllMargins (sbtext->display, 0);
  uiWidgetSetMarginStart (sbtext->display, 4);
  uiLabelSetMinWidth (sbtext->display, 5);
  sbtext->sb = uisbCreate (box, sbtext->display, SB_IS_TEXT, margin);
  sbtext->txtlist = NULL;
  sbtext->idxlist = NULL;
  sbtext->old_value = LIST_VALUE_INVALID;
  sbtext->index = 0;
  sbtext->prepend_text = NULL;
  sbtext->sbtextcb = NULL;
  sbtext->chgcb = NULL;
  sbtext->dispcb = NULL;
  sbtext->udata = NULL;
  sbtext->count = 0;
  sbtext->changed = false;
  sbtext->force = false;

  sbtext->sbtextcb = callbackInitI (uisbtextCBHandler, sbtext);
  uisbSetCallback (sbtext->sb, sbtext->sbtextcb);

  return sbtext;
}

void
uisbtextFree (uisbtext_t *sbtext)
{
  if (sbtext == NULL) {
    return;
  }

  uisbFree (sbtext->sb);
  nlistFree (sbtext->idxlist);
  callbackFree (sbtext->sbtextcb);
  uiwcontFree (sbtext->display);
  mdfree (sbtext);
}

void
uisbtextPrependList (uisbtext_t *sbtext, const char *txt)
{
  if (sbtext == NULL) {
    return;
  }

  sbtext->prepend_text = txt;
  sbtext->index = -1;
}

void
uisbtextSetList (uisbtext_t *sbtext, nlist_t *txtlist)
{
  nlistidx_t    iteridx;
  nlistidx_t    key;
  int32_t       idx;
  size_t        width;

  if (sbtext == NULL || txtlist == NULL) {
fprintf (stderr, "sbtxt: null\n");
    return;
  }

  if (sbtext->idxlist != NULL) {
    nlistFree (sbtext->idxlist);
    sbtext->idxlist = NULL;
  }

  sbtext->txtlist = txtlist;
  sbtext->count = nlistGetCount (sbtext->txtlist);

  sbtext->idxlist = nlistAlloc ("sbtextidx", LIST_UNORDERED, NULL);
  nlistSetSize (sbtext->idxlist, sbtext->count);

  nlistCalcMaxValueWidth (sbtext->txtlist);
  width = nlistGetMaxValueWidth (sbtext->txtlist);
  if (sbtext->prepend_text != NULL) {
    size_t      len;

    len = istrlen (sbtext->prepend_text);
    width = len > width ? len : width;
  }
  uiLabelSetMinWidth (sbtext->display, width + 1);

  idx = 0;
  nlistStartIterator (sbtext->txtlist, &iteridx);
  while ((key = nlistIterateKey (sbtext->txtlist, &iteridx)) >= 0) {
fprintf (stderr, "sbtxt: %d %d %s\n", idx, key, nlistGetStr (sbtext->txtlist, key));
    nlistSetNum (sbtext->idxlist, idx, key);
    ++idx;
  }
  nlistSort (sbtext->idxlist);
  uisbtextSetDisplay (sbtext);
}

void
uisbtextSetCount (uisbtext_t *sbtext, int count)
{
  if (sbtext == NULL) {
    return;
  }

  sbtext->count = count;
}

void
uisbtextSetWidth (uisbtext_t *sbtext, int width)
{
  if (sbtext == NULL) {
    return;
  }

  uiLabelSetMinWidth (sbtext->display, width);
  uiLabelSetMaxWidth (sbtext->display, width);
}

bool
uisbtextIsChanged (uisbtext_t *sbtext)
{
  bool      chg;

  if (sbtext == NULL) {
    return false;
  }

  chg = sbtext->changed;
  return chg;
}

void
uisbtextSetDisplayCallback (uisbtext_t *sbtext, uisbtextdisp_t cb, void *udata)
{
  if (sbtext == NULL) {
    return;
  }

  sbtext->dispcb = cb;
  sbtext->udata = udata;

  uisbtextSetDisplay (sbtext);
}

void
uisbtextSetChangeCallback (uisbtext_t *sbtext, callback_t *chgcb)
{
  if (sbtext == NULL) {
    return;
  }

  sbtext->chgcb = chgcb;
}

void
uisbtextSetClass (uisbtext_t *sbtext, const char *name)
{
  if (sbtext == NULL || name == NULL) {
    return;
  }

  uiWidgetSetClass (sbtext->display, name);
}

void
uisbtextRemoveClass (uisbtext_t *sbtext, const char *name)
{
  if (sbtext == NULL || name == NULL) {
    return;
  }

  uiWidgetClearClass (sbtext->display, name);
}

void
uisbtextSetState (uisbtext_t *sbtext, int state)
{
  if (sbtext == NULL) {
    return;
  }

  uisbSetState (sbtext->sb, state);
}

void
uisbtextSizeGroupAdd (uisbtext_t *sbtext, uiwcont_t *sg)
{
  if (sbtext == NULL || sg == NULL) {
    return;
  }

  uisbSizeGroupAdd (sbtext->sb, sg);
}

void
uisbtextSetValueForce (uisbtext_t *sbtext, int value)
{
  if (sbtext == NULL) {
    return;
  }

  sbtext->force = true;
  uisbtextSetValue (sbtext, value);
  sbtext->force = false;
}

void
uisbtextSetValue (uisbtext_t *sbtext, int value)
{
  nlistidx_t    iteridx;
  nlistidx_t    idx;

  if (sbtext == NULL) {
    return;
  }

  if (sbtext->old_value == LIST_VALUE_INVALID) {
    sbtext->old_value = value;
  }

  if (sbtext->idxlist == NULL) {
    sbtext->index = value;
  } else {
    /* locate the index for the set value */
    nlistStartIterator (sbtext->idxlist, &iteridx);
    while ((idx = nlistIterateKey (sbtext->idxlist, &iteridx)) >= 0) {
      if (nlistGetNum (sbtext->idxlist, idx) == value) {
        sbtext->index = idx;
        break;
      }
    }
  }
  uisbtextSetDisplay (sbtext);
}

int32_t
uisbtextGetValue (uisbtext_t *sbtext)
{
  int32_t   value;

  if (sbtext == NULL) {
    return LIST_VALUE_INVALID;
  }

  if (sbtext->idxlist == NULL) {
    value = sbtext->index;
  } else {
    value = nlistGetNum (sbtext->idxlist, sbtext->index);
  }
  return value;
}

/* internal routines */

static bool
uisbtextCBHandler (void *udata, int32_t dir)
{
  uisbtext_t  *sbtext = udata;

  if (sbtext == NULL) {
    return UICB_STOP;
  }

  if (dir == SB_INCREMENT || dir == SB_PAGE_INCR) {
    sbtext->index += 1;
  }
  if (dir == SB_DECREMENT || dir == SB_PAGE_DECR) {
    sbtext->index -= 1;
  }

  /* text spinboxes wrap around to the beginning/end */
  if (sbtext->index < 0) {
    if (sbtext->prepend_text == NULL || sbtext->index < -1) {
      sbtext->index = sbtext->count - 1;
    }
  }

  if (sbtext->index >= sbtext->count) {
    sbtext->index = 0;
    if (sbtext->prepend_text != NULL) {
      sbtext->index = -1;
    }
  }

  uisbtextSetDisplay (sbtext);

  return UICB_CONT;
}

static void
uisbtextSetDisplay (uisbtext_t *sbtext)
{
  const char  *disp;
  int32_t     newvalue = LIST_VALUE_INVALID;

  if (sbtext->dispcb != NULL) {
    newvalue = sbtext->index;
    disp = sbtext->dispcb (sbtext->udata, sbtext->index);
    uiLabelSetText (sbtext->display, disp);
  } else {
    if (sbtext->prepend_text != NULL && sbtext->index == -1) {
      newvalue = sbtext->index;
      uiLabelSetText (sbtext->display, sbtext->prepend_text);
    } else {
      newvalue = nlistGetNum (sbtext->idxlist, sbtext->index);
      uiLabelSetText (sbtext->display,
          nlistGetStr (sbtext->txtlist, newvalue));
    }
  }

  if (sbtext->old_value != newvalue) {
    sbtext->changed = true;
    sbtext->old_value = newvalue;
  }

  if (sbtext->chgcb != NULL && sbtext->changed) {
    callbackHandler (sbtext->chgcb);
    sbtext->changed = false;
  }
}
