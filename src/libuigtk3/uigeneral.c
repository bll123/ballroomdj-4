#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"

inline void
uiutilsUIWidgetInit (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  uiwidget->widget = NULL;
}

inline bool
uiutilsUIWidgetSet (UIWidget *uiwidget)
{
  bool rc = true;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    rc = false;
  }
  return rc;
}


inline void
uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source)
{
  if (target == NULL) {
    return;
  }
  if (source == NULL) {
    return;
  }
  memcpy (target, source, sizeof (UIWidget));
}

void
uiutilsUICallbackInit (UICallback *uicb, UICallbackFunc cb, void *udata,
    const char *actiontext)
{
  if (uicb == NULL) {
    return;
  }

  uicb->cb = cb;
  uicb->udata = udata;
  uicb->actiontext = actiontext;
}
void
uiutilsUICallbackDoubleInit (UICallback *uicb, UIDoubleCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->doublecb = cb;
  uicb->udata = udata;
  uicb->actiontext = NULL;
}

void
uiutilsUICallbackIntIntInit (UICallback *uicb, UIIntIntCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->intintcb = cb;
  uicb->udata = udata;
  uicb->actiontext = NULL;
}

void
uiutilsUICallbackLongInit (UICallback *uicb, UILongCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->longcb = cb;
  uicb->udata = udata;
  uicb->actiontext = NULL;
}

void
uiutilsUICallbackLongIntInit (UICallback *uicb, UILongIntCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->longintcb = cb;
  uicb->udata = udata;
  uicb->actiontext = NULL;
}

void
uiutilsUICallbackStrInit (UICallback *uicb, UIStrCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->strcb = cb;
  uicb->udata = udata;
  uicb->actiontext = NULL;
}

bool
uiutilsCallbackHandler (UICallback *uicb)
{
  bool  rc = false;

  if (uicb == NULL) {
    return rc;
  }
  if (uicb->cb == NULL) {
    return rc;
  }

  rc = uicb->cb (uicb->udata);
  return rc;
}

bool
uiutilsCallbackLongHandler (UICallback *uicb, long value)
{
  bool  rc = false;

  if (uicb == NULL) {
    return 0;
  }
  if (uicb->longcb == NULL) {
    return 0;
  }

  rc = uicb->longcb (uicb->udata, value);
  return rc;
}

bool
uiutilsCallbackLongIntHandler (UICallback *uicb, long lval, int ival)
{
  bool  rc = false;

  if (uicb == NULL) {
    return 0;
  }
  if (uicb->longintcb == NULL) {
    return 0;
  }

  rc = uicb->longintcb (uicb->udata, lval, ival);
  return rc;
}

long
uiutilsCallbackStrHandler (UICallback *uicb, const char *str)
{
  long    value;

  if (uicb == NULL) {
    return 0;
  }
  if (uicb->longcb == NULL) {
    return 0;
  }

  value = uicb->strcb (uicb->udata, str);
  return value;
}

