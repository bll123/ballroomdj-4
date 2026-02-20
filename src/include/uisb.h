/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */

#pragma once

#include "callback.h"

enum {
  SB_INCREMENT = 1,
  SB_DECREMENT = -1,
  SB_PAGE_INCR = 2,
  SB_PAGE_DECR = -2,
};

typedef struct uisb uisb_t;

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uisb_t * uisbCreate (uiwcont_t *box, uiwcont_t *disp);
void uisbFree (uisb_t *sb);
void uisbExpandHoriz (uisb_t *sb);
void uisbSetCallback (uisb_t *sb, callback_t *uicb);
void uisbSetState (uisb_t *sb, int state);
void uisbSizeGroupAdd (uisb_t *sb, uiwcont_t *sg);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

