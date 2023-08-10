/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "ati.h"
#include "atibdj4.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
} atisaved_t;

void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  return;
}

int
atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

atisaved_t *
atibdj4SaveMP4Tags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return NULL;
}

void
atibdj4FreeSavedMP4Tags (atisaved_t *atisaved, int tagtype, int filetype)
{
  if (atisaved == NULL) {
    return;
  }
  if (! atisaved->hasdata) {
    return;
  }
  if (atisaved->tagtype != tagtype) {
    return;
  }
  if (atisaved->filetype != filetype) {
    return;
  }

  atisaved->hasdata = false;
  mdfree (atisaved);
}

int
atibdj4RestoreMP4Tags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  return -1;
}

void
atibdj4CleanMP4Tags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return;
}

void
atibdj4LogMP4Version (void)
{
  logMsg (LOG_DBG, LOG_INFO, "mp4 version %s", "not-coded");
}

