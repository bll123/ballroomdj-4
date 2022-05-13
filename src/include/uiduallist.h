#ifndef INC_UIDUALLIST_H
#define INC_UIDUALLIST_H

#include "uiutils.h"

typedef struct {
  GtkWidget         *ltree;
  GtkTreeSelection  *lsel;
  GtkWidget         *rtree;
  GtkTreeSelection  *rsel;
} uiduallist_t;

uiduallist_t * uiCreateDualList (UIWidget *vbox,
    UICallback *uiselectcb, UICallback *uiremovecb,
    UICallback *uimoveupcb, UICallback *uimovedowncb);

#endif /* INC_UIDUALLIST_H */
