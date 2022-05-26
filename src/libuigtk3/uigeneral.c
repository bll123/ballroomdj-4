#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"
#include "uiutils.h"

inline void
uiutilsUIWidgetInit (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  uiwidget->widget = NULL;
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
