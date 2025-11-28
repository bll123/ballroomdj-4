/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if __has_include (<ncursesw/ncurses.h>)
# include <ncursesw/ncurses.h>
#else
# if __has_include (<ncurses.h>)
#  include <ncurses.h>
# endif
#endif

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* the structure must contain a member named widget */
/* and a member named packwidget */
typedef struct uispecific {
  void      *packwidget;
  union {
    void    *widget;
    WINDOW  *wind;
  };
  int       wh;
  int       ww;
  int       wx;
  int       wy;
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

