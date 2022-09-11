#ifndef INC_AUDIOTAG_H
#define INC_AUDIOTAG_H

#include "slist.h"

enum {
  AUDIOTAG_TAG_BUFF_SIZE = 16384,
  AT_KEEP_MOD_TIME,
  AT_UPDATE_MOD_TIME,
};

void audiotagInit (void);
void audiotagCleanup (void);
char * audiotagReadTags (const char *ffn);
slist_t * audiotagParseData (const char *ffn, char *data, int *rewrite);
void audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist, int rewrite, int modTimeFlag);

#endif /* INC_AUDIOTAG_H */
