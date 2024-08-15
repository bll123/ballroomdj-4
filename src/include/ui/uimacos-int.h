/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMACOS_INT_H
#define INC_UIMACOS_INT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uispecific {
  void          *packwidget;
  union {
    void        *widget;
  };
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIMACOS_INT_H */
