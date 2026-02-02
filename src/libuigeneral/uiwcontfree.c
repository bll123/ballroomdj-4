/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
#include "ui/uispinbox.h"
#include "ui/uiswitch.h"
#include "ui/uitextbox.h"
#include "ui/uitoggle.h"
#include "ui/uiui.h"
#include "ui/uiwindow.h"

void
uiwcontFree (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  switch (uiwidget->wtype) {
    case WCONT_T_UNKNOWN: {
      fprintf (stderr, "ERR: trying to free a uiwidget twice\n");
      break;
    }
    case WCONT_T_BOX: {
      uiBoxFree (uiwidget);
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
    case WCONT_T_SPINBOX_DOUBLE_DFLT:
    case WCONT_T_SPINBOX_TIME:
    case WCONT_T_SPINBOX_TEXT: {
      uiSpinboxFree (uiwidget);
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

