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
  SIGID_OUTPUT,
  SIGID_INPUT,
  /* also abused for image-toggle (switch) */
  SIGID_FOCUS,
  /* click, toggle, response, scroll, activate-link, */
  /* switch-page, drag-data-rcv, menu-toggle, menu-activate */
  SIGID_RESPONSE,
  SIGID_VAL_CHG,
  SIGID_DEL_WIN,
  SIGID_SIZE_ALLOC,
  SIGID_ENTER_NOTIFY,
  SIGID_LEAVE_NOTIFY,
  SIGID_PRESS,
  SIGID_RELEASE,
  SIGID_BPRESS,
  SIGID_BRELEASE,
  SIGID_MAX,
};

typedef struct uispecific {
  /* storing the signal id doesn't work out for many widgets, as the */
  /* widget container is not preserved */
  unsigned long   sigid [SIGID_MAX];
  GtkWidget       *packwidget;
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
