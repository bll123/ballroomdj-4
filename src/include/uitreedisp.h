/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITREEDISP_H
#define INC_UITREEDISP_H

#include "callback.h"
#include "slist.h"
#include "uiwcont.h"

enum {
  FAVORITE_INCREMENT = 1,
};

void uitreedispAddDisplayColumns (uiwcont_t *uiwidget, slist_t *sellist, int col, int fontcol, int ellipsizeColumn, int colorcol, int colorsetcol);
void uitreedispSetDisplayColumn (uiwcont_t *uiwidget, int col, long num, const char *str);

#endif /* INC_UITREEDISP_H */
