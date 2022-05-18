#ifndef INC_UISELECTFILE_H
#define INC_UISELECTFILE_H

#include "bdj4ui.h"
#include "nlist.h"
#include "uiutils.h"

enum {
  SELFILE_PLAYLIST,
  SELFILE_SEQUENCE,
  SELFILE_SONGLIST,
};

typedef void (*selfilecb_t)(void *cbudata, const char *fname);

typedef struct uiselectfile uiselectfile_t;

void selectFileDialog (int type, UIWidget *window, nlist_t *options, void *udata, selfilecb_t cb);
void selectFileFree (uiselectfile_t *selectfile);

#endif /* INC_UISELECTFILE_H */
