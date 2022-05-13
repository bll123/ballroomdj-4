#ifndef INC_UIDUALLIST_H
#define INC_UIDUALLIST_H

#include "uiutils.h"

enum {
  DUALLIST_TREE_SOURCE,
  DUALLIST_TREE_TARGET,
  DUALLIST_TREE_MAX,
};

enum {
  DUALLIST_FLAGS_NONE,
  DUALLIST_FLAGS_MULTIPLE,
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
  bool              changed : 1;
} uiduallist_t;

uiduallist_t * uiCreateDualList (UIWidget *vbox, int flags);
void uiduallistSet (uiduallist_t *, slist_t *slist, int which);
bool uiduallistIsChanged (uiduallist_t *duallist);
void uiduallistClearChanged (uiduallist_t *duallist);

#endif /* INC_UIDUALLIST_H */
