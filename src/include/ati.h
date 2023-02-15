/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ATI_H
#define INC_ATI_H

#include "nlist.h"
#include "slist.h"

typedef struct ati ati_t;

ati_t   *atiInit (const char *atipkg);
void    atiFree (ati_t *ati);
char    *atiReadTags (ati_t *ati, const char *ffn);
void    atiParseTags (ati_t *ati, slist_t *tagdata, char *data, int tagtype, int *rewrite);
int     atiWriteTags (ati_t *ati, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);

void    atiiInit (const char *atipkg);
char    *atiiReadTags (const char *ffn);
void    atiiParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite);
int     atiiWriteTags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int filetype, int writetags);

#endif /* INC_ATI_H */
