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
  HNB_NO_ID = -1,
  HNB_SHOW = true,
  HNB_HIDE = false,
};

typedef struct uihnb uihnb_t;

uihnb_t *uihnbCreate (uiwcont_t *box);
void uihnbFree (uihnb_t *hnb);
void uihnbAppendPage (uihnb_t *hnb, uiwcont_t *uiwidget, const char *label, const char *imagenm, int id);
void uihnbHideShowPage (uihnb_t *hnb, int pagenum, bool show);
void uihnbSetPage (uihnb_t *hnb, int pagenum);
void uihnbSetCallback (uihnb_t *hnb, callback_t *uicb);
void uihnbSetActionWidget (uihnb_t *hnb, uiwcont_t *uiwidget);
int  uihnbGetID (uihnb_t *hnb);
int  uihnbGetIDByPage (uihnb_t *hnb, int pagenum);
int  uihnbGetPage (uihnb_t *hnb, int id);
void uihnbStartIDIterator (uihnb_t *hnb);
int  uihnbIterateID (uihnb_t *hnb, int *pagenum);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

