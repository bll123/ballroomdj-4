/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* these are defined based on the gtk values */
/* would change for a different gui package */
enum {
  UICB_STOP = true,
  UICB_CONT = false,
  UICB_DISPLAY_ON = true,
  UICB_DISPLAY_OFF = false,
  UICB_CONVERTED = true,
  UICB_NOT_CONVERTED = false,
  UICB_CONVERT_FAIL = -1,       // GTK_INPUT_ERROR
  UICB_SUPPORTED = true,
  UICB_NOT_SUPPORTED = false,
  UIWIDGET_DISABLE = false,
  UIWIDGET_ENABLE = true,
  UI_TOGGLE_BUTTON_ON = true,
  UI_TOGGLE_BUTTON_OFF = false,
  UI_FOREACH_STOP = true,
  UI_FOREACH_CONT = false,
};

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

