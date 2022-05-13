#ifndef INC_UIDUALLIST_H
#define INC_UIDUALLIST_H

#include "uiutils.h"

enum {
  DUALLIST_TREE_LEFT,
  DUALLIST_TREE_RIGHT,
  DUALLIST_TREE_MAX,
};

typedef struct {
  GtkWidget         *tree;
  GtkTreeSelection  *sel;
} uiduallisttree_t;

typedef struct {
  uiduallisttree_t  trees [DUALLIST_TREE_MAX];
  UICallback        moveprevcb;
  UICallback        movenextcb;
  bool              changed : 1;
} uiduallist_t;

uiduallist_t * uiCreateDualList (UIWidget *vbox,
    UICallback *uiselectcb, UICallback *uiremovecb);
void uiduallistSet (uiduallist_t *, slist_t *slist, int which);

#endif /* INC_UIDUALLIST_H */
