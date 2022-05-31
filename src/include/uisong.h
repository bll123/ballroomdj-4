#ifndef INC_UISONG_H
#define INC_UISONG_H

#include <gtk/gtk.h>

#include "slist.h"

void uisongSetDisplayColumns (slist_t *sellist, song_t *song, int col, GtkTreeModel *model, GtkTreeIter *iter);
GType * uisongAddDisplayTypes (GType *typelist, slist_t *sellist, int *colcount);

#endif /* INC_UISONG_H */
