/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISELECTFILE_H
#define INC_UISELECTFILE_H

#include "callback.h"
#include "nlist.h"
#include "ui.h"

enum {
  SELFILE_PLAYLIST,
  SELFILE_SEQUENCE,
  SELFILE_SONGLIST,
};

typedef struct {
  const char  *title;
  uientry_t   *entry;
  uiwcont_t   *window;
} uisfcb_t;

typedef struct uiselectfile uiselectfile_t;

void selectFileDialog (int type, uiwcont_t *window, nlist_t *options, callback_t *cb);
void selectFileFree (uiselectfile_t *selectfile);
bool selectAudioFileCallback (void *udata);
bool selectAllFileCallback (void *udata);

#endif /* INC_UISELECTFILE_H */
