#ifndef INC_UITREEDDISP_H
#define INC_UITREEDDISP_H

#include "ui/uitreeview.h"

void uitreedispAddDisplayColumns (uitree_t *uitree, slist_t *sellist, int col, int fontcol, int ellipsizeColumn);
void uitreedispSetDisplayColumn (uitree_t *uitree, int col, long num, const char *str);

#endif /* INC_UITREEDDISP_H */
