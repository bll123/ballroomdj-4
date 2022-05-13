#ifndef INC_UIDUALLIST_H
#define INC_UIDUALLIST_H

#include "uiutils.h"
#include "slist.h"

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

typedef struct {
  GtkWidget         *tree;
  GtkTreeSelection  *sel;
} uiduallisttree_t;

typedef struct {
  uiduallisttree_t  trees [DUALLIST_TREE_MAX];
  UICallback        moveprevcb;
  UICallback        movenextcb;
  UICallback        selectcb;
  UICallback        removecb;
  int               flags;
  char              *searchstr;
  int               pos;
  int               searchtype;
  bool              changed : 1;
} uiduallist_t;

uiduallist_t * uiCreateDualList (UIWidget *vbox, int flags,
    const char *sourcetitle, const char *targettitle);
void uiduallistFree (uiduallist_t *uiduallist);
void uiduallistSet (uiduallist_t *uiduallist, slist_t *slist, int which);
bool uiduallistIsChanged (uiduallist_t *duallist);
void uiduallistClearChanged (uiduallist_t *duallist);

#endif /* INC_UIDUALLIST_H */
