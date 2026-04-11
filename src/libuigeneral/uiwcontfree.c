/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "log.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

/* the includes are needed to get the declaration of ...Free() */
#include "ui/uibox.h"
#include "ui/uibutton.h"
#include "ui/uientry.h"
#include "ui/uievents.h"
#include "ui/uiimage.h"
#include "ui/uimenu.h"
#include "ui/uiscrollbar.h"
#include "ui/uiswitch.h"
#include "ui/uitextbox.h"
#include "ui/uitoggle.h"
#include "ui/uiui.h"
#include "ui/uiwindow.h"

static void uiwcontBoxFree (uiwcont_t *uibox);

void
uiwcontFree (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  switch (uiwidget->wtype) {
    case WCONT_T_UNKNOWN: {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: trying to free %" PRIu32 "-%" PRIu32 " twice", uiwidget->ui_id, uiwidget->id);
      fprintf (stderr, "ERR: trying to free %" PRIu32 "-%" PRIu32 " twice\n", uiwidget->ui_id, uiwidget->id);
      break;
    }
    case WCONT_T_HBOX:
    case WCONT_T_VBOX: {
      uiwcontBoxFree (uiwidget);
      break;
    }
    case WCONT_T_BUTTON: {
      uiButtonFree (uiwidget);
      break;
    }
    case WCONT_T_ENTRY: {
      uiEntryFree (uiwidget);
      break;
    }
    case WCONT_T_IMAGE: {
      uiImageFree (uiwidget);
      break;
    }
    case WCONT_T_KEY: {
      uiEventFree (uiwidget);
      break;
    }
    case WCONT_T_MENU: {
      uiMenuFree (uiwidget);
      break;
    }
    case WCONT_T_SCROLLBAR: {
      uiScrollbarFree (uiwidget);
      break;
    }
    case WCONT_T_SWITCH: {
      uiSwitchFree (uiwidget);
      break;
    }
    case WCONT_T_TEXT_BOX: {
      uiTextBoxFree (uiwidget);
      break;
    }
    case WCONT_T_BUTTON_CHKBOX:
    case WCONT_T_BUTTON_RADIO:
    case WCONT_T_BUTTON_TOGGLE: {
      uiToggleButtonFree (uiwidget);
      break;
    }
    case WCONT_T_WINDOW_MAIN: {
      uiMainWindowFree (uiwidget);
      break;
    }
    default: {
      break;
    }
  }

  if (uiwidget->wtype != WCONT_T_UNKNOWN) {
    uiwcontBaseFree (uiwidget);
  }
}

static void
uiwcontBoxFree (uiwcont_t *uibox)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;
  nlist_t     *rlist;
  nlistidx_t  iteridx;
  int         key;
  int32_t     tid;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "wcont-box-free")) {
    return;
  }

  boxbase = &uibox->uiint.uiboxbase;
  tid = uibox->id;

  if (! boxbase->postprocess) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: box %" PRIu32 "-%" PRIu32 " not post-process", uibox->ui_id, tid);
    fprintf (stderr, "ERR: box %" PRIu32 "-%" PRIu32 " not post-process\n", uibox->ui_id, tid);
  }

  // fprintf (stderr, "   free %" PRIu32 "-%" PRIu32 " \n", uibox->ui_id, tid);
  wlist = boxbase->widgetlist;
  rlist = boxbase->releaselist;
  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcontrls_t    release;

    release = nlistGetNum (rlist, key);
    if (release == WCONT_FREE) {
      uiwcont_t     *uiwidgetp;

      uiwidgetp = nlistGetData (wlist, key);
      uiwcontFree (uiwidgetp);
    }
  }

  uiBoxFree (uibox);
}
