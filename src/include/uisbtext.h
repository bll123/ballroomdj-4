/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */

#pragma once

#include <stdint.h>

#include "callback.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef const char * (*uisbtextdisp_t)(void *, int);
typedef struct uisbtext uisbtext_t;

uisbtext_t * uisbtextCreate (uiwcont_t *box);
void uisbtextFree (uisbtext_t *sbtext);
void uisbtextSetDisplayCallback (uisbtext_t *sbtext, uisbtextdisp_t cb, void *udata);
void uisbtextSetChangeCallback (uisbtext_t *sbtext, callback_t *cb);
void uisbtextPrependList (uisbtext_t *sbtext, const char *txt);
void uisbtextSetList (uisbtext_t *sbtext, nlist_t *txtlist);
void uisbtextSetCount (uisbtext_t *sbtext, int count);
void uisbtextSetWidth (uisbtext_t *sbtext, int width);
void uisbtextAddClass (uisbtext_t *sbtext, const char *name);
void uisbtextRemoveClass (uisbtext_t *sbtext, const char *name);
void uisbtextSetState (uisbtext_t *sbtext, int state);
void uisbtextSetValue (uisbtext_t *sbtext, int32_t value);
int32_t uisbtextGetValue (uisbtext_t *sbtext);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif
