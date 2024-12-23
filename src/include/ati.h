/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ATI_H
#define INC_ATI_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <stdint.h>

#include "ilist.h"
#include "nlist.h"
#include "tagdef.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define LIBATI_PFX  "libati"

enum {
  ATI_NONE,
  ATI_READ,
  ATI_READ_WRITE,
};

enum {
  ATI_FLAGS_NONE        = 0,
  ATI_FLAGS_FORCE_WRITE = (1 << 0),
};

enum {
  /* is this large enough to hold a pic &etc.? */
  /* lyrics &etc. might come out to 200k */
  ATI_TAG_BUFF_SIZE = (5 * 1024 * 1024),
};

typedef struct ati ati_t;
typedef struct atisaved atisaved_t;
typedef struct atidata atidata_t;
typedef const char *(*taglookup_t)(int, const char *);
typedef int (*tagcheck_t)(int, int, const char *, int);
typedef const char *(*tagname_t)(int);
typedef const tagaudiotag_t *(*audiotaglookup_t)(int, int);

bool    atiCheck (const char *atipkg);
ati_t   *atiInit (const char *atipkg, int writetags, taglookup_t tagLookup, tagcheck_t tagCheck, tagname_t tagName, audiotaglookup_t tagRawLookup);
void    atiFree (ati_t *ati);
void    atiSupportedTypes (ati_t *ati, int supported []);
void    atiParseTags (ati_t *ati, slist_t *tagdata, const char *ffn, int filetype, int tagtype, int *rewrite);
int     atiWriteTags (ati_t *ati, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t *atiSaveTags (ati_t *ati, const char *ffn, int tagtype, int filetype);
void    atiFreeSavedTags (ati_t *ati, atisaved_t *atisaved, int tagtype, int filetype);
int     atiRestoreTags (ati_t *ati, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void    atiCleanTags (ati_t *ati, const char *ffn, int tagtype, int filetype);

int     atiCheckCodec (const char *ffn, int filetype);
void    atiGetSupportedTypes (const char *atipkg, int supported []);

void    atiiSupportedTypes (int supported []);
atidata_t *atiiInit (const char *atipkg, int writetags, taglookup_t tagLookup, tagcheck_t tagCheck, tagname_t tagName, audiotaglookup_t tagRawLookup);
void    atiiFree (atidata_t *atidata);
void    atiiParseTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int filetype, int tagtype, int *rewrite);
int     atiiWriteTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t *atiiSaveTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void    atiiFreeSavedTags (atisaved_t *atisaved, int tagtype, int filetype);
int     atiiRestoreTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void    atiiCleanTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);

/* atiutil.c */
/* utility routines */
const char *atiParsePair (slist_t *tagdata, const char *tagname, const char *value, char *pbuff, size_t sz);
int atiReplaceFile (const char *ffn, const char *outfn);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ATI_H */
