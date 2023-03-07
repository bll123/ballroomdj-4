/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ATI_H
#define INC_ATI_H

#include "nlist.h"
#include "slist.h"

enum {
  ATI_TAG_BUFF_SIZE = 16384,
};

typedef struct ati ati_t;
typedef struct atidata atidata_t;
typedef const char *(*taglookup_t)(int, const char *);
typedef int (*tagcheck_t)(int, int, const char *, int);

ati_t   *atiInit (const char *atipkg, taglookup_t tagLookup, tagcheck_t tagCheck);
void    atiFree (ati_t *ati);
char    *atiReadTags (ati_t *ati, const char *ffn);
void    atiParseTags (ati_t *ati, slist_t *tagdata, char *data, int tagtype, int *rewrite);
int     atiWriteTags (ati_t *ati, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);

atidata_t *atiiInit (const char *atipkg, int writetags, taglookup_t tagLookup, tagcheck_t tagCheck);
void    atiiFree (atidata_t *atidata);
char    *atiiReadTags (atidata_t *atidata, const char *ffn);
void    atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data, int tagtype, int *rewrite);
int     atiiWriteTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);

#endif /* INC_ATI_H */
