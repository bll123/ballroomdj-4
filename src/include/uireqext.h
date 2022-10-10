#ifndef INC_UIREQEXT_H
#define INC_UIREQEXT_H

#include "nlist.h"
#include "song.h"
#include "ui.h"

typedef struct uireqext uireqext_t;

uireqext_t  *uireqextInit (UIWidget *windowp, nlist_t *opts);
void    uireqextFree (uireqext_t *uireqext);
bool    uireqextDialog (uireqext_t *uireqext);
song_t  *uireqextGetSong (uireqext_t *uireqext);

#endif /* INC_UIREQEXT_H */
