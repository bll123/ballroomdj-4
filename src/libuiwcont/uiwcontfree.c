/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "ui/uibutton.h"
#include "ui/uidropdown.h"
#include "ui/uikeys.h"
#include "ui/uimenu.h"
#include "ui/uiscrollbar.h"
#include "ui/uiswitch.h"
#include "ui/uitextbox.h"

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
    case WCONT_T_BUTTON: {
      uiButtonFree (uiwidget);
      break;
    }
    case WCONT_T_DROPDOWN: {
      uiDropDownFree (uiwidget);
      break;
    }
    case WCONT_T_KEY: {
      uiKeyFree (uiwidget);
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
    default: {
      break;
    }
  }

  uiwcontBaseFree (uiwidget);
}

