/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISONGEDIT_H
#define INC_UIAUDIOID_H

#include <stdbool.h>

#include "conn.h"
#include "dispsel.h"
#include "musicdb.h"
#include "nlist.h"
#include "song.h"
#include "tmutil.h"
#include "uisongsel.h"
#include "ui.h"

typedef struct aid_internal aid_internal_t;

typedef struct {
  conn_t            *conn;
  musicdb_t         *musicdb;
  dispsel_t         *dispsel;
  nlist_t           *options;
  aid_internal_t    *audioidInternalData;
  uiwcont_t         *statusMsg;
  callback_t        *savecb;
  uisongsel_t       *uisongsel;
} uiaudioid_t;

/* uiaudioid.c */
uiaudioid_t * uiaudioidInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel, nlist_t *opts);
void  uiaudioidFree (uiaudioid_t *uiaudioid);
void  uiaudioidMainLoop (uiaudioid_t *uiaudioid);
void  uiaudioidNewSelection (uiaudioid_t *uiaudioid, dbidx_t dbidx);
void  uiaudioidSetSaveCallback (uiaudioid_t *uiaudioid, callback_t *uicb);

/* uiaudioidui.c */
void  uiaudioidUIInit (uiaudioid_t *uiaudioid);
void  uiaudioidUIFree (uiaudioid_t *uiaudioid);
uiwcont_t   * uiaudioidBuildUI (uisongsel_t *uisongsel, uiaudioid_t *uiaudioid, uiwcont_t *parentwin, uiwcont_t *statusMsg);
void  uiaudioidLoadData (uiaudioid_t *uiaudioid, song_t *song, dbidx_t dbidx);
void  uiaudioidSetDisplayList (uiaudioid_t *uiaudioid, nlist_t *data);
void  uiaudioidUIMainLoop (uiaudioid_t *uiaudioid);

#endif /* INC_UIAUDIOID_H */

