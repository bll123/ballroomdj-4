#ifndef INC_AUDIOTAG_H
#define INC_AUDIOTAG_H

#include "slist.h"

#define AUDIOTAG_TAG_BUFF_SIZE  16384

void audiotagInit (void);
void audiotagCleanup (void);
char * audiotagReadTags (const char *ffn);
slist_t * audiotagParseData (const char *ffn, char *data);
void audiotagWriteTags (const char *ffn, slist_t *tagdata);

#endif /* INC_AUDIOTAG_H */
