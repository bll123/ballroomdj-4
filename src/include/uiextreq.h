/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIEXTREQ_H
#define INC_UIEXTREQ_H

#include "callback.h"
#include "musicdb.h"
#include "nlist.h"
#include "song.h"
#include "uiwcont.h"

typedef struct uiextreq uiextreq_t;

uiextreq_t  *uiextreqInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts);
void    uiextreqFree (uiextreq_t *uiextreq);
void    uiextreqSetResponseCallback (uiextreq_t *uiextreq, callback_t *uicb);
bool    uiextreqDialog (uiextreq_t *uiextreq, const char *fn);
song_t  *uiextreqGetSong (uiextreq_t *uiextreq);
void    uiextreqProcess (uiextreq_t *uiextreq);

#endif /* INC_UIEXTREQ_H */
