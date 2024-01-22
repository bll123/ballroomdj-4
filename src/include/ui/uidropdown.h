/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDROPDOWN_H
#define INC_UIDROPDOWN_H

#include "callback.h"
#include "nlist.h"
#include "slist.h"
#include "uiwcont.h"

uiwcont_t *uiDropDownInit (void);
void uiDropDownFree (uiwcont_t *dropdown);
uiwcont_t *uiDropDownCreate (uiwcont_t *dropdown,
    uiwcont_t *parentwin, const char *title, callback_t *uicb, void *udata);
uiwcont_t *uiComboboxCreate (uiwcont_t *dropdown,
    uiwcont_t *parentwin, const char *title, callback_t *uicb, void *udata);
/* be sure to call slistCalcMaxWidth() on the lists passed to these routines */
void uiDropDownSetList (uiwcont_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSetNumList (uiwcont_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSelectionSetNum (uiwcont_t *dropdown, nlistidx_t idx);
void uiDropDownSelectionSetStr (uiwcont_t *dropdown, const char *stridx);
void uiDropDownSetState (uiwcont_t *dropdown, int state);
char *uiDropDownGetString (uiwcont_t *dropdown);

#endif /* INC_UIDROPDOWN_H */
