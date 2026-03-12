/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */

#pragma once

#include "callback.h"

/* decrement values must be the negative of the increment value */
enum {
  SB_INCREMENT = 1,
  SB_DECREMENT = - (SB_INCREMENT),
  SB_PAGE_INCR = 2,
  SB_PAGE_DECR = - (SB_PAGE_INCR),
  SB_VALIDATE = 3,
  SB_VAL_FORCE = 4,
};

enum {
  SB_IS_NUM,
  SB_IS_TEXT,
};

typedef struct uisb uisb_t;

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uisb_t * uisbCreate (uiwcont_t *box, uiwcont_t *disp, bool istext, int margin);
void uisbFree (uisb_t *sb);
void uisbExpandHoriz (uisb_t *sb);
void uisbSetCallback (uisb_t *sb, callback_t *uicb);
void uisbSetState (uisb_t *sb, int state);
void uisbSizeGroupAdd (uisb_t *sb, uiwcont_t *sg);
void uisbSetRepeat (uisb_t *sb, int repeatms);
void uisbCheck (uisb_t *sb);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

