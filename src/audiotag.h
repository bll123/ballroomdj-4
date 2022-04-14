#ifndef INC_AUDIOTAG_H
#define INC_AUDIOTAG_H

#include "slist.h"

#define AUDIOTAG_TAG_BUFF_SIZE  8192

char    * audiotagReadTags (char *ffn, long count);
slist_t * audiotagParseData (char *ffn, char *data);

#endif /* INC_AUDIOTAG_H */
