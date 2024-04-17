#ifndef INC_UITREEDDISP_H
#define INC_UITREEDDISP_H

#include "callback.h"
#include "slist.h"
#include "uiwcont.h"

enum {
  FAVORITE_INCREMENT = 1,
};

void uitreedispAddDisplayColumns (uiwcont_t *uiwidget, slist_t *sellist, int col, int fontcol, int ellipsizeColumn, int colorcol, int colorsetcol);
void uitreedispSetDisplayColumn (uiwcont_t *uiwidget, int col, long num, const char *str);

#endif /* INC_UITREEDDISP_H */
