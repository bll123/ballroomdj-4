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

typedef struct audiotag audiotag_t;

void    audiotagInit (void);
void    audiotagCleanup (void);
char *  audiotagReadTags (const char *ffn);
slist_t * audiotagParseData (const char *ffn, char *data, int *rewrite);
int     audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist, int rewrite, int modTimeFlag);

#endif /* INC_AUDIOTAG_H */
