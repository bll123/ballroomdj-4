#ifndef INC_ATIBDJ4_H
#define INC_ATIBDJ4_H

#include "ati.h"
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

/* atibdj4flac.c */
void atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteFlacTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedFlacTags (atisaved_t *atisaved, int tagtype, int filetype);
int atibdj4RestoreFlacTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogFlacVersion (void);

/* atibdj4id3.c */
void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedMP3Tags (atisaved_t *atisaved, int tagtype, int filetype);
int atibdj4RestoreMP3Tags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogMP3Version (void);

/* atibdj4mp4.cpp */
void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveMP4Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedMP4Tags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreMP4Tags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanMP4Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogMP4Version (void);

/* atibdj4ogg.c */
void atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOggTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedOggTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreOggTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOggVersion (void);

int  atibdj4WriteOggFile (const char *ffn, void *newvc);
void atibdj4OggAddVorbisComment (void *newvc, int tagkey, const char *tagname, const char *val);

/* atibdj4opus.c */
void atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
atisaved_t * atibdj4SaveOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedOpusTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreOpusTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOpusVersion (void);

#endif /* INC_ATIBDJ4_H */
