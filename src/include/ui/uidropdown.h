/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDROPDOWN_H
#define INC_UIDROPDOWN_H

#include "callback.h"
#include "nlist.h"
#include "slist.h"
#include "uiwcont.h"

typedef struct uidropdown uidropdown_t;

uidropdown_t *uiDropDownInit (void);
void uiDropDownFree (uidropdown_t *dropdown);
uiwcont_t *uiDropDownCreate (uidropdown_t *dropdown,
    uiwcont_t *parentwin, const char *title, callback_t *uicb, void *udata);
uiwcont_t *uiComboboxCreate (uidropdown_t *dropdown,
    uiwcont_t *parentwin, const char *title, callback_t *uicb, void *udata);
/* be sure to call slistCalcMaxWidth() on the lists passed to these routines */
void uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx);
void uiDropDownSelectionSetStr (uidropdown_t *dropdown, const char *stridx);
void uiDropDownSetState (uidropdown_t *dropdown, int state);
char *uiDropDownGetString (uidropdown_t *dropdown);

#endif /* INC_UIDROPDOWN_H */
