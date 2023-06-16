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

#include <opus/opusfile.h>

#include "ati.h"
#include "atibdj4.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

void
atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc;

  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return;
  }

  linknum = op_current_link (of);
  tags = op_tags (of, linknum);
  if (tags == NULL) {
    return;
  }

  for (int i = 0; i < tags->comments; ++i) {
    const char  *kw;

    kw = tags->user_comments [i];
    if (opus_tagncompare ("METADATA_BLOCK_PICTURE", 22, kw) == 0) {
      continue;
    }

    atibdj4ProcessVorbisComment (atidata, tagdata, tagtype, kw);
  }
  op_free (of);
  return;
}

int
atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

void
atibdj4LogOpusVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "opus version %s", opus_get_version_string());
}

