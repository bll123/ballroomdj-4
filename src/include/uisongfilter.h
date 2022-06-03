#ifndef INC_UISONGFILTER_H
#define INC_UISONGFILTER_H

#include "nlist.h"
#include "songfilter.h"
#include "uiutils.h"

typedef struct uisongfilter uisongfilter_t;

/* uisongfilter.c */
uisongfilter_t * uisfInit (UIWidget *windowp, nlist_t *options, songfilterpb_t pbflag);
void uisfFree (uisongfilter_t *uisf);
void uisfSetApplyCallback (uisongfilter_t *uisf, UICallback *applycb);
void uisfSetDanceSelectCallback (uisongfilter_t *uisf, UICallback *danceselcb);
bool uisfDialog (void *udata);
void uisfSetDanceIdx (uisongfilter_t *uisf, int danceIdx);
songfilter_t *uisfGetSongFilter (uisongfilter_t *uisf);

#endif /* INC_UISONGFILTER_H */
