/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUDIOTAG_H
#define INC_AUDIOTAG_H

#include "slist.h"

enum {
  AT_KEEP_MOD_TIME,
  AT_UPDATE_MOD_TIME,
};

enum {
  AUDIOTAG_TAG_BUFF_SIZE = 16384,
  AUDIOTAG_WRITE_OK = 0,
  AUDIOTAG_WRITE_OFF = 251,
  AUDIOTAG_NOT_SUPPORTED = 252,
};

enum {
  /* some of the AF_ flags are only internal to audiotag.c */
  AF_REWRITE_NONE     = 0x0000,
  AF_REWRITE_MB       = 0x0001,
  AF_REWRITE_DURATION = 0x0002,
  AF_REWRITE_VARIOUS  = 0x0004,
  AF_FORCE_WRITE_BDJ  = 0x0008,
};

void audiotagInit (void);
void audiotagCleanup (void);
char * audiotagReadTags (const char *ffn);
slist_t * audiotagParseData (const char *ffn, char *data, int *rewrite);
int  audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist, int rewrite, int modTimeFlag);

#endif /* INC_AUDIOTAG_H */
