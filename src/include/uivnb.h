/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  VNB_NO_ID = -1,
};

typedef struct uivnb uivnb_t;

uivnb_t *uivnbCreate (const char *ident, uiwcont_t *box);
void uivnbFree (uivnb_t *vnb);
void uivnbAppendPage (uivnb_t *vnb, uiwcont_t *uiwidget, const char *label, int id);
void uivnbSetPage (uivnb_t *vnb, int pagenum);
void uivnbSetCallback (uivnb_t *vnb, callback_t *uicb);
int  uivnbGetID (uivnb_t *vnb);
int  uivnbGetPage (uivnb_t *vnb, int id);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

