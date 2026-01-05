/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "slist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  DL_LIST_SOURCE,
  DL_LIST_TARGET,
  DL_LIST_MAX,
};

enum {
  DL_FLAGS_NONE       = 0,
  DL_FLAGS_MULTIPLE   = (1 << 0),
  /* if persistent, the selections are not removed from the source tree */
  DL_FLAGS_PERSISTENT = (1 << 1),
};

typedef struct uiduallist uiduallist_t;

uiduallist_t * uiCreateDualList (uiwcont_t *vbox, int flags,
    const char *sourcetitle, const char *targettitle);
void uiduallistFree (uiduallist_t *uiduallist);
void uiduallistSet (uiduallist_t *uiduallist, slist_t *slist, int which);
bool uiduallistIsChanged (uiduallist_t *duallist);
void uiduallistClearChanged (uiduallist_t *duallist);
slist_t * uiduallistGetList (uiduallist_t *duallist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

