#ifndef INC_UITREEDDISP_H
#define INC_UITREEDDISP_H

#include "callback.h"

#include "ui/uitreeview.h"

void uitreedispAddDisplayColumns (uitree_t *uitree, slist_t *sellist, int col, int fontcol, int ellipsizeColumn, int colorcol, int colorsetcol);
void uitreedispSetDisplayColumn (uitree_t *uitree, int col, long num, const char *str);

#endif /* INC_UITREEDDISP_H */
