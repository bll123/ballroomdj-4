/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UINULL_WCONT_H
#define INC_UINULL_WCONT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* the structure must contain a member named widget */
/* and a member named packwidget */
typedef struct uispecific {
  void      *packwidget;
  union {
    void    *widget;
  };
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UINULL_WCONT_H */
