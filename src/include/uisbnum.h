/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */

#pragma once

#include <stdint.h>

#include "callback.h"
#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  SBNUM_NUMERIC,
  SBNUM_TIME_BASIC,
  SBNUM_TIME_PRECISE,
};

typedef const char * (*uisbnumdisp_t)(void *, int);
typedef struct uisbnum uisbnum_t;

uisbnum_t * uisbnumCreate (uiwcont_t *box, int maxWidth);
void uisbnumFree (uisbnum_t *sbnum);
void uisbnumSetIncrements (uisbnum_t *sbnum, double incr, double pageincr);
void uisbnumSetLimits (uisbnum_t *sbnum, double min, double max, int digits);
void uisbnumSetTime (uisbnum_t *sbnum, double min, double max, int timetype);

void uisbnumCheck (uisbnum_t *sbnum);
bool uisbnumIsChanged (uisbnum_t *sbnum);
void uisbnumSetChangeCallback (uisbnum_t *sbnum, callback_t *cb);

//void uisbnumAddClass (uisbnum_t *sbnum, const char *name);
//void uisbnumRemoveClass (uisbnum_t *sbnum, const char *name);

void uisbnumSetState (uisbnum_t *sbnum, int state);
void uisbnumSizeGroupAdd (uisbnum_t *sbnum, uiwcont_t *sg);
void uisbnumSetValue (uisbnum_t *sbnum, double value);
double uisbnumGetValue (uisbnum_t *sbnum);


#if defined (__cplusplus) || defined (c_plusplus)
}
#endif
