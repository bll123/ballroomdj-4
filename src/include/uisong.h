/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISONG_H
#define INC_UISONG_H

#include <stdint.h>

#include "slist.h"
#include "song.h"

typedef void (*uisongcb_t)(int col, long num, const char *str, void *udata);
typedef void (*uisongdtcb_t)(int type, void *udata);

nlist_t *uisongGetDisplayList (slist_t *dlsellist, slist_t *itemsellist, song_t *song);
void uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col, uisongcb_t cb, void *udata);
char *uisongGetDisplay (song_t *song, int tagidx, int32_t *num, double *dval);
char *uisongGetValue (song_t *song, int tagidx, int32_t *num, double *dval);
void uisongAddDisplayTypes (slist_t *sellist, uisongdtcb_t cb, void *udata);

#endif /* INC_UISONG_H */
