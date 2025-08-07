/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ATIBDJ4_H
#define INC_ATIBDJ4_H

#include "ati.h"
#include "dylib.h"
#include "slist.h"

#include <libavformat/avformat.h>
#include <libavutil/error.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct atidatatag atidatatag_t;

typedef struct atidata {
  dlhandle_t        *avfmtdlh;
  dlhandle_t        *avutildlh;
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
  atidatatag_t      *data;
  void              (*av_log_set_callback)(void (*callback)(void*, int, const char*, va_list));
  void              (*av_strerror)(int, char *, size_t);
  int               (*avformat_open_input)(AVFormatContext **ictx, const char *, void *, void *);
  int               (*avformat_find_stream_info)(AVFormatContext *ictx, void *);
  int               (*avformat_close_input)(AVFormatContext **ictx);
} atidata_t;

/* atibdj4flac.c */
void atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteFlacTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedFlacTags (atisaved_t *atisaved, int tagtype, int filetype);
int atibdj4RestoreFlacTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanFlacTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogFlacVersion (void);

/* atibdj4id3.c */
void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, const char *iso639_2, int32_t flags);
atisaved_t * atibdj4SaveMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedMP3Tags (atisaved_t *atisaved, int tagtype, int filetype);
int atibdj4RestoreMP3Tags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanMP3Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogMP3Version (void);

/* atibdj4mp4.c */
void atibdj4ParseMP4Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteMP4Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveMP4Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedMP4Tags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreMP4Tags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanMP4Tags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogMP4Version (void);

/* atibdj4ogg.c */
void atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOggTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedOggTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreOggTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOggTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOggVersion (void);

/* atibdj4oggutil.c */
void atioggProcessVorbisCommentCombined (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *kw);
void atioggProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata, int tagtype, const char *tag, const char *val);
const char * atioggParseVorbisComment (const char *kw, char *buff, size_t sz);
slist_t *atioggSplitVorbisComment (int tagkey, const char *tagname, const char *val);
int  atioggWriteOggFile (const char *ffn, void *newvc, int filetype);

/* atibdj4opus.c */
void atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedOpusTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreOpusTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanOpusTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogOpusVersion (void);

/* atibdj4asf.c */
void atibdj4ParseASFTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteASFTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveASFTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedASFTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreASFTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanASFTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogASFVersion (void);

/* atibdj4riff.c */
void atibdj4ParseRIFFTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
int  atibdj4WriteRIFFTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int32_t flags);
atisaved_t * atibdj4SaveRIFFTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4FreeSavedRIFFTags (atisaved_t *atisaved, int tagtype, int filetype);
int  atibdj4RestoreRIFFTags (atidata_t *atidata, atisaved_t *atisaved, const char *ffn, int tagtype, int filetype);
void atibdj4CleanRIFFTags (atidata_t *atidata, const char *ffn, int tagtype, int filetype);
void atibdj4LogRIFFVersion (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ATIBDJ4_H */
