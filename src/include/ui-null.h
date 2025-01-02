/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UI_NULL_H
#define INC_UI_NULL_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  UICB_STOP = true,
  UICB_CONT = false,
  UICB_DISPLAY_ON = true,
  UICB_DISPLAY_OFF = false,
  UICB_CONVERTED = true,
  UICB_NOT_CONVERTED = false,
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

#endif /* INC_UI_NULL_H */
