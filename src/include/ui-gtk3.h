/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK_H
#define INC_UIGTK_H

#include <gtk/gtk.h>        // will go away

typedef glong treenum_t;      // will go away

/* tree view storage types */
enum {
  TREE_ELLIPSIZE_END = PANGO_ELLIPSIZE_END,     // will be moved
};

/* these are defined based on the gtk values */
/* would change for a different gui package */
enum {
  UICB_STOP = true,
  UICB_CONT = false,
  UICB_DISPLAYED = true,
  UICB_NO_DISP = false,
  UICB_NO_CONV = false,
  UICB_CONVERTED = true,
  UICB_SUPPORTED = true,
  UICB_NOT_SUPPORTED = false,
  UIWIDGET_DISABLE = false,
  UIWIDGET_ENABLE = true,
  UI_TOGGLE_BUTTON_ON = true,
  UI_TOGGLE_BUTTON_OFF = false,
};

typedef union uiwidget uiwidget_t;
typedef union uiwidget UIWidget; // temporary, will be removed later

#endif /* INC_UIGTK_H */
