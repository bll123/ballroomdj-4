/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  UIEIBDJ4_EXPORT,
  UIEIBDJ4_IMPORT,
  UIEIBDJ4_MAX,
};

typedef struct uieibdj4 uieibdj4_t;

uieibdj4_t  *uieibdj4Init (uiwcont_t *windowp, nlist_t *opts);
void    uieibdj4Free (uieibdj4_t *uieibdj4);
void    uieibdj4SetResponseCallback (uieibdj4_t *uieibdj4, callback_t *uicb, int expimptype);
bool    uieibdj4Dialog (uieibdj4_t *uieibdj4, int expimptype);
void    uieibdj4DialogClear (uieibdj4_t *uieibdj4);
void    uieibdj4Process (uieibdj4_t *uieibdj4);
char    *uieibdj4GetDir (uieibdj4_t *uieibdj4);
const char  *uieibdj4GetPlaylist (uieibdj4_t *uieibdj4);
const char  *uieibdj4GetNewName (uieibdj4_t *uieibdj4);
void uieibdj4UpdateStatus (uieibdj4_t *uieibdj4, int count, int tot);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

