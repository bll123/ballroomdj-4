/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIREQEXT_H
#define INC_UIREQEXT_H

#include "musicdb.h"
#include "nlist.h"
#include "song.h"
#include "ui.h"

typedef struct uireqext uireqext_t;

uireqext_t  *uireqextInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts);
void    uireqextFree (uireqext_t *uireqext);
void    uireqextSetResponseCallback (uireqext_t *uireqext, callback_t *uicb);
bool    uireqextDialog (uireqext_t *uireqext, const char *fn);
song_t  *uireqextGetSong (uireqext_t *uireqext);
void    uireqextProcess (uireqext_t *uireqext);

#endif /* INC_UIREQEXT_H */
