#ifndef INC_ATIBDJ4_H
#define INC_ATIBDJ4_H

#include "slist.h"

typedef struct atidatatag atidatatag_t;

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
  atidatatag_t      *data;
} atidata_t;

/* atibdj4.c */
void atibdj4ProcessVorbisComment (atidata_t *atidata, slist_t *tagdata, int tagtype, const char *kw);
const char * atibdj4ParseVorbisComment (const char *kw, char *buff, size_t sz);

/* atibdj4flac.c */
void atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteFlacTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4RestoreFlacTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogFlacVersion (void);

/* atibdj4id3.c */
void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4RestoreMP3Tags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogMP3Version (void);

/* atibdj4mp4.cpp */
void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);

/* atibdj4ogg.c */
void atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOggTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4RestoreOggTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOggVersion (void);

/* atibdj4opus.c */
void atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4RestoreOpusTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOpusVersion (void);

#endif /* INC_ATIBDJ4_H */
