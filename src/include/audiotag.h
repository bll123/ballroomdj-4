/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOTAG_H
#define INC_AUDIOTAG_H

#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  AT_FLAGS_NONE             = 0,
  AT_FLAGS_MOD_TIME_KEEP    = (1 << 0),
  AT_FLAGS_MOD_TIME_UPDATE  = (1 << 1),
  AT_FLAGS_FORCE_WRITE      = (1 << 2),
};

enum {
  AUDIOTAG_WRITE_OK = 0,
  AUDIOTAG_WRITE_FAILED = 1,
  AUDIOTAG_WRITE_OFF = 251,
  AUDIOTAG_NOT_SUPPORTED = 252,
};

typedef struct audiotag audiotag_t;

void    audiotagInit (void);
void    audiotagCleanup (void);
slist_t * audiotagParseData (const char *ffn, int *rewrite);
int     audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist, int rewrite, int32_t flags);
void    *audiotagSaveTags (const char *ffn);
void    audiotagFreeSavedTags (const char *ffn, void *sdata);
int     audiotagRestoreTags (const char *ffn, void *sdata);
void    audiotagCleanTags (const char *ffn);
void    audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_AUDIOTAG_H */
