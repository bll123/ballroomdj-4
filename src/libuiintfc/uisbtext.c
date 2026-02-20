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
  const char      *pre_text;
  int32_t         old_index;
  int32_t         index;
  int32_t         count;
  bool            changed;
} uisbtext_t;

static bool uisbtextCBHandler (void *udata, int32_t dir);
static void uisbtextSetDisplay (uisbtext_t *sbtext);

uisbtext_t *
uisbtextCreate (uiwcont_t *box)
{
  uisbtext_t  *sbtext;

  sbtext = mdmalloc (sizeof (uisbtext_t));
  sbtext->display = uiCreateLabel ("");
  uiLabelSetMinWidth (sbtext->display, 5);
  sbtext->sb = uisbCreate (box, sbtext->display);
  sbtext->txtlist = NULL;
  sbtext->idxlist = NULL;
  sbtext->old_index = LIST_VALUE_INVALID;
  sbtext->index = 0;
  sbtext->pre_text = NULL;
  sbtext->sbtextcb = NULL;
  sbtext->chgcb = NULL;
  sbtext->dispcb = NULL;
  sbtext->udata = NULL;
  sbtext->count = 0;
  sbtext->changed = false;

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

  sbtext->pre_text = txt;
  sbtext->index = -1;
}

void
uisbtextSetList (uisbtext_t *sbtext, nlist_t *txtlist)
{
  nlistidx_t    iteridx;
  nlistidx_t    key;
  int32_t       idx;

  if (sbtext == NULL) {
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

  idx = 0;
  nlistStartIterator (sbtext->txtlist, &iteridx);
  while ((key = nlistIterateKey (sbtext->txtlist, &iteridx)) >= 0) {
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
  sbtext->changed = false;
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
uisbtextAddClass (uisbtext_t *sbtext, const char *name)
{
  if (sbtext == NULL || name == NULL) {
    return;
  }

  uiWidgetAddClass (sbtext->display, name);
}

void
uisbtextRemoveClass (uisbtext_t *sbtext, const char *name)
{
  if (sbtext == NULL || name == NULL) {
    return;
  }

  uiWidgetRemoveClass (sbtext->display, name);
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
uisbtextSetValue (uisbtext_t *sbtext, int value)
{
  nlistidx_t    iteridx;
  nlistidx_t    idx;

  if (sbtext == NULL) {
    return;
  }

  if (sbtext->idxlist == NULL) {
    sbtext->index = value;
  } else {
    nlistStartIterator (sbtext->idxlist, &iteridx);
    while ((idx = nlistIterateKey (sbtext->idxlist, &iteridx)) >= 0) {
      if (nlistGetNum (sbtext->idxlist, idx) == value) {
        sbtext->index = idx;
        break;
      }
    }
  }
  uisbtextSetDisplay (sbtext);
  sbtext->changed = false;
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

  if (dir == SB_INCREMENT) {
    sbtext->index += 1;
  }
  if (dir == SB_DECREMENT) {
    sbtext->index -= 1;
  }

  /* text spinboxes wrap around to the beginning/end */
  if (sbtext->index < 0) {
    if (sbtext->pre_text == NULL || sbtext->index < -1) {
      sbtext->index = sbtext->count - 1;
    }
  }

  if (sbtext->index >= sbtext->count) {
    sbtext->index = 0;
    if (sbtext->pre_text != NULL) {
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

  if (sbtext->old_index == sbtext->index) {
    return;
  }

  if (sbtext->dispcb != NULL) {
    disp = sbtext->dispcb (sbtext->udata, sbtext->index);
    uiLabelSetText (sbtext->display, disp);
  } else {
    if (sbtext->pre_text != NULL && sbtext->index == -1) {
      uiLabelSetText (sbtext->display, sbtext->pre_text);
    } else {
      uiLabelSetText (sbtext->display,
         nlistGetDataByIdx (sbtext->txtlist, sbtext->index));
    }
  }

  if (sbtext->chgcb != NULL) {
    if (sbtext->old_index != LIST_VALUE_INVALID) {
fprintf (stderr, "sb: chg: %s set true\n", uiLabelGetText (sbtext->display));
      sbtext->changed = true;
    }
    callbackHandler (sbtext->chgcb);
  }

  sbtext->old_index = sbtext->index;
}
