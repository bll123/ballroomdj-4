#ifndef INC_UISONG_H
#define INC_UISONG_H

#include "slist.h"
#include "song.h"

typedef void (*uisongcb_t)(int col, long num, const char *str, void *udata);
typedef void (*uisongdtcb_t)(int type, void *udata);

void uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col, uisongcb_t cb, void *udata);
char *uisongGetDisplay (song_t *song, int tagidx, long *num, double *dval);
void uisongAddDisplayTypes (slist_t *sellist, uisongdtcb_t cb, void *udata);

#endif /* INC_UISONG_H */
