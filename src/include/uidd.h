/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDD_H
#define INC_UIDD_H

#include "callback.h"
#include "nlist.h"

typedef struct uidd uidd_t;

enum {
  DD_KEEP_TITLE,
  DD_REPLACE_TITLE,     // a combo-box (no entry)
  DD_LIST_TYPE_STR,
  DD_LIST_TYPE_NUM,
};

enum {
  DD_NO_SELECTION = -1,
};

uidd_t *uiddCreate (const char *tag, uiwcont_t *parentwin, uiwcont_t *boxp, nlist_t *keylist, nlist_t *displist, int listtype, const char *title, int titleflag, callback_t *ddcb);
void uiddFree (uidd_t *dd);
void uiddSetSelection (uidd_t *dd, nlistidx_t idx);
void uiddSetState (uidd_t *dd, int state);
const char *uiddGetSelectionStr (uidd_t *dd);
int uiddGetSelectionNum (uidd_t *dd);

#endif /* INC_UIDD_H */
