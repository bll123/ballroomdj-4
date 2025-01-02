/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISONG_H
#define INC_UISONG_H

#include <stdint.h>

#include "slist.h"
#include "song.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef void (*uisongcb_t)(int col, long num, const char *str, void *udata);
typedef void (*uisongdtcb_t)(int type, void *udata);

nlist_t *uisongGetDisplayList (slist_t *dlsellist, slist_t *itemsellist, song_t *song);
char *uisongGetDisplay (song_t *song, int tagidx, int32_t *num, double *dval);
char *uisongGetValue (song_t *song, int tagidx, int32_t *num, double *dval);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISONG_H */
