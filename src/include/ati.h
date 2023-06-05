/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ATI_H
#define INC_ATI_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "nlist.h"
#include "slist.h"
#include "tagdef.h"

enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MP4,
  AFILE_TYPE_MP3,
  AFILE_TYPE_OGGOPUS,
  AFILE_TYPE_OGGVORBIS,
  AFILE_TYPE_WMA,
};

enum {
  ATI_TAG_BUFF_SIZE = 16384,
};

typedef struct ati ati_t;
typedef struct atidata atidata_t;
typedef const char *(*taglookup_t)(int, const char *);
typedef int (*tagcheck_t)(int, int, const char *, int);
typedef const char *(*tagname_t)(int);
typedef const tagaudiotag_t *(*audiotaglookup_t)(int, int);

ati_t   *atiInit (const char *atipkg, int writetags, taglookup_t tagLookup, tagcheck_t tagCheck, tagname_t tagName, audiotaglookup_t tagRawLookup);
void    atiFree (ati_t *ati);
void    *atiReadTags (ati_t *ati, const char *ffn);
void    atiParseTags (ati_t *ati, slist_t *tagdata, void *tdata, int filetype, int tagtype, int *rewrite);
int     atiWriteTags (ati_t *ati, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
slist_t *atiInterfaceList (void);

const char *atiiDesc (void);
atidata_t *atiiInit (const char *atipkg, int writetags, taglookup_t tagLookup, tagcheck_t tagCheck, tagname_t tagName, audiotaglookup_t tagRawLookup);
void    atiiFree (atidata_t *atidata);
void    *atiiReadTags (atidata_t *atidata, const char *ffn);
void    atiiParseTags (atidata_t *atidata, slist_t *tagdata, void *tdata, int filetype, int tagtype, int *rewrite);
int     atiiWriteTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);

/* atiutil.c */
/* utility routines */
const char *atiParsePair (slist_t *tagdata, const char *tagname, const char *value, char *pbuff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ATI_H */
