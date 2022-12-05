/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDUALLIST_H
#define INC_UIDUALLIST_H

#include "slist.h"
#include "ui.h"

enum {
  DUALLIST_TREE_SOURCE,
  DUALLIST_TREE_TARGET,
  DUALLIST_TREE_MAX,
};

enum {
  DUALLIST_FLAGS_NONE       = 0x0000,
  DUALLIST_FLAGS_MULTIPLE   = 0x0001,
  /* if persistent, the selections are not removed from the source tree */
  DUALLIST_FLAGS_PERSISTENT = 0x0002,
};

typedef struct uiduallist uiduallist_t;

uiduallist_t * uiCreateDualList (UIWidget *vbox, int flags,
    const char *sourcetitle, const char *targettitle);
void uiduallistFree (uiduallist_t *uiduallist);
void uiduallistSet (uiduallist_t *uiduallist, slist_t *slist, int which);
bool uiduallistIsChanged (uiduallist_t *duallist);
void uiduallistClearChanged (uiduallist_t *duallist);
slist_t * uiduallistGetList (uiduallist_t *duallist);

#endif /* INC_UIDUALLIST_H */
