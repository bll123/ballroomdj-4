#ifndef INC_ATIBDJ4_H
#define INC_ATIBDJ4_H

#include "slist.h"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite);


#endif /* INC_ATIBDJ4_H */
