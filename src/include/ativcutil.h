/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "ati.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void atiProcessVorbisCommentCombined (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *kw);
void atiProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *tag, const char *val);
const char * atiParseVorbisComment (const char *kw, char *buff, size_t sz);
slist_t *atiSplitVorbisComment (int tagkey, const char *tagname, const char *val);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

