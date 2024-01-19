/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIQUICKEDIT_H
#define INC_UIQUICKEDIT_H

#include "musicdb.h"
#include "nlist.h"
#include "ui.h"

typedef struct uiqe uiqe_t;

typedef struct {
  dbidx_t           dbidx;
  double            voladj;
  double            speed;
  int               level;
  int               rating;
} uiqesave_t;

uiqe_t  *uiqeInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts);
void    uiqeFree (uiqe_t *uiqe);
void    uiqeSetResponseCallback (uiqe_t *uiqe, callback_t *uicb);
bool    uiqeDialog (uiqe_t *uiqe, dbidx_t dbidx, double speed, double vol, int basevol);
const uiqesave_t  *uiqeGetResponseData (uiqe_t *uiqe);

#endif /* INC_UIQUICKEDIT_H */
