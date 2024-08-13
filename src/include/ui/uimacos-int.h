/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMACOS_INT_H
#define INC_UIMACOS_INT_H

typedef struct uispecific {
  void          *packwidget;
  union {
    void        *widget;
  };
} uispecific_t;

#endif /* INC_UIMACOS_INT_H */
