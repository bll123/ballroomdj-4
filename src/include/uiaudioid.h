/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIAUDIOID_H
#define INC_UIAUDIOID_H

#include <stdbool.h>

#include "audioid.h"
#include "dispsel.h"
#include "musicdb.h"
#include "nlist.h"
#include "song.h"
#include "tmutil.h"
#include "uisongsel.h"
#include "ui.h"

typedef struct aid_internal aid_internal_t;

typedef struct {
  nlist_t           *options;
  dispsel_t         *dispsel;
  uisongsel_t       *uisongsel;
  aid_internal_t    *audioidInternalData;
  callback_t        *savecb;
  uiwcont_t         *statusMsg;
} uiaudioid_t;

/* uiaudioid.c */
uiaudioid_t * uiaudioidInit (nlist_t *opts, dispsel_t *dispsel);
void  uiaudioidFree (uiaudioid_t *uiaudioid);
void  uiaudioidMainLoop (uiaudioid_t *uiaudioid);
void  uiaudioidSetSaveCallback (uiaudioid_t *uiaudioid, callback_t *uicb);
bool  uiaudioidIsRepeating (uiaudioid_t *uiaudioid);
bool  uiaudioidInSave (uiaudioid_t *uiaudioid);

/* uiaudioidui.c */
void  uiaudioidUIInit (uiaudioid_t *uiaudioid);
void  uiaudioidUIFree (uiaudioid_t *uiaudioid);
void  uiaudioidSavePanePosition (uiaudioid_t *uiaudioid);
uiwcont_t * uiaudioidBuildUI (uiaudioid_t *uiaudioid, uisongsel_t *uisongsel, uiwcont_t *parentwin, uiwcont_t *statusMsg);
void  uiaudioidLoadData (uiaudioid_t *uiaudioid, song_t *song, dbidx_t dbidx);
void  uiaudioidResetDisplayList (uiaudioid_t *uiaudioid);
void  uiaudioidSetDisplayList (uiaudioid_t *uiaudioid, nlist_t *dlist);
void  uiaudioidFinishDisplayList (uiaudioid_t *uiaudioid);
void  uiaudioidUIMainLoop (uiaudioid_t *uiaudioid);

#endif /* INC_UIAUDIOID_H */

