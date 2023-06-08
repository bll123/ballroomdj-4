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

#include <bento4/Ap4.h>

extern "C" {

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

void
atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  AP4_ByteStream* input     = NULL;
  AP4_File*       file      = NULL;
  AP4_Result      result    = AP4_SUCCESS;

  result = AP4_FileByteStream::Create (ffn,
      AP4_FileByteStream::STREAM_MODE_READ, input);
  file = new AP4_File (*input);

  delete file;
  delete input;
}

} /* extern C */
