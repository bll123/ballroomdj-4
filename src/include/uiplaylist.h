/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPLAYLIST_H
#define INC_UIPLAYLIST_H

#include "callback.h"
#include "uiwcont.h"

typedef struct uiplaylist uiplaylist_t;

enum {
  PL_PACK_START,
  PL_PACK_END,
};

uiplaylist_t *uiplaylistCreate (uiwcont_t *parentwin, uiwcont_t *hbox, int type, const char *label, int where);
void uiplaylistFree (uiplaylist_t *uiplaylist);
void uiplaylistSetList (uiplaylist_t *uiplaylist, int type, const char *dir);
const char *uiplaylistGetKey (uiplaylist_t *uiplaylist);
void uiplaylistSetKey (uiplaylist_t *uiplaylist, const char *fn);
void uiplaylistSizeGroupAdd (uiplaylist_t *uiplaylist, uiwcont_t *sg);
void uiplaylistSetSelectCallback (uiplaylist_t *uiplaylist, callback_t *cb);

#endif /* INC_UIPLAYLIST_H */
