/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK3_WCONT_H
#define INC_UIGTK3_WCONT_H

#include <gtk/gtk.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  HID_OUTPUT,
  HID_INPUT,
  /* also abused for image-toggle (switch) */
  HID_FOCUS,
  /* click, toggle, response, scroll, activate-link, */
  /* switch-page, drag-data-rcv, menu-toggle, menu-activate */
  HID_RESPONSE,
  HID_VAL_CHG,
  HID_DEL_WIN,
  HID_SIZE_ALLOC,
  HID_ENTER_NOTIFY,
  HID_LEAVE_NOTIFY,
  HID_PRESS,
  HID_RELEASE,
  HID_BPRESS,
  HID_BRELEASE,
  HID_MAX,
};

typedef struct uispecific {
  unsigned long hid [HID_MAX];
  GtkWidget     *packwidget;
  union {
    GtkWidget     *widget;
    GtkSizeGroup  *sg;
    GdkPixbuf     *pixbuf;
    GtkTextBuffer *buffer;
    GtkAdjustment *adjustment;
  };
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIGTK3_WCONT_H */
