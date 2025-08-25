/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  SELFILE_PLAYLIST,
  SELFILE_SEQUENCE,
  SELFILE_SONGLIST,
};

typedef struct {
  const char  *title;
  const char  *defdir;
  uiwcont_t   *entry;
  uiwcont_t   *window;
} uisfcb_t;

typedef struct uiselectfile uiselectfile_t;

void selectInitCallback (uisfcb_t *uisfcb);
void selectFileDialog (int type, uiwcont_t *window, nlist_t *options, callback_t *cb);
void selectFileFree (uiselectfile_t *selectfile);
bool selectAudioFileCallback (void *udata);
bool selectAllFileCallback (void *udata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

