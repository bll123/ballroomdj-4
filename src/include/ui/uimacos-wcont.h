/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMACOS_WCONT_H
#define INC_UIMACOS_WCONT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* to hold the margin information for the widget */
typedef struct macosmargin macosmargin_t;

typedef struct uispecific {
  void              *packwidget;
  union {
    void            *widget;
  };
  /* to hold the margin information and margin constraints for the widget */
  macosmargin_t     *margins;
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIMACOS_WCONT_H */
