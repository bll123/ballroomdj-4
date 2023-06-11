/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <id3tag.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define OV_EXCLUDE_STATIC_CALLBACKS 1
#include <vorbis/vorbisfile.h>
#undef OV_EXCLUDE_STATIC_CALLBACKS

#include <opus/opusfile.h>
#include <FLAC/metadata.h>

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"

static void atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static void atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata, const char *ffn, int tagtype, int *rewrite);
static int  atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static int  atibdj4WriteOggTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static int  atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static int  atibdj4WriteFlacTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static void atibdj4ProcessVorbisComment (atidata_t *atidata, slist_t *tagdata, int tagtype, const char *kw);
static const char * atibdj4ParseVorbisComment (const char *kw, char *buff, size_t sz);
static int  atibdj4WriteOggPage (ogg_page *p, FILE *fp);
static int  atibdj4WriteOggFile (const char *ffn, struct vorbis_comment *newvc);
static void atibdj4LogCallback (void *avcl, int level, const char *fmt, va_list vl);

const char *
atiiDesc (void)
{
  return "BDJ4 Internal";
}

atidata_t *
atiiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  atidata_t *atidata;

  atidata = mdmalloc (sizeof (atidata_t));
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;
  atidata->tagName = tagName;
  atidata->audioTagLookup = audioTagLookup;

  /* turn off logging */
  av_log_set_callback (atibdj4LogCallback);

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    mdfree (atidata);
  }
}

bool
atiiUseReader (void)
{
  return false;
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  return NULL;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, const char *ffn,
    char *data, int filetype, int tagtype, int *rewrite)
{
  AVFormatContext   *ictx = NULL;
  char              pbuff [100];
  int32_t           duration;
  int               rc;
  bool              needduration = true;

  if (! fileopFileExists (ffn)) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no file %s", ffn);
    return;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "parse tags %s", ffn);

  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    atibdj4ParseMP3Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OGG) {
    needduration = false;
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: ogg");
    atibdj4ParseOggTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: opus");
    atibdj4ParseOpusTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    needduration = false;
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: flac");
    atibdj4ParseFlacTags (atidata, tagdata, ffn, tagtype, rewrite);
  }
  if (tagtype == TAG_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp4");
    atibdj4ParseMP4Tags (atidata, tagdata, ffn, tagtype, rewrite);
  }

  if (needduration) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: use avformat");

    if ((rc = avformat_open_input (&ictx, ffn, NULL, NULL))) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s", rc, ffn);
      return;
    }

    if ((rc = avformat_find_stream_info (ictx, NULL)) < 0) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s", rc, ffn);
      avformat_close_input (&ictx);
      return;
    }

    duration = ictx->duration;
    duration /= 1000;
    snprintf (pbuff, sizeof (pbuff), "%ld", (long) duration);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", pbuff);
    slistSetStr (tagdata, atidata->tagName (TAG_DURATION), pbuff);

    avformat_close_input (&ictx);
  }

  return;
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  int         rc = -1;

  if (! fileopFileExists (ffn)) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no file %s", ffn);
    return -1;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write tags %s", ffn);

  if (tagtype == TAG_TYPE_MP3) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp3");
    rc = atibdj4WriteMP3Tags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_OGG) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: ogg");
    rc = atibdj4WriteOggTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: opus");
    rc = atibdj4WriteOpusTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: flac");
    rc = atibdj4WriteFlacTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }
  if (tagtype == TAG_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag-type: mp4");
    rc = atibdj4WriteMP4Tags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }

  return rc;
}

static void
atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tag;
  struct id3_frame  *id3frame;
  int               idx;
  char              pbuff [100];

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  id3tag = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tag, "", idx)) != NULL) {
    const char          *tagname;
    id3_latin1_t const  *ufid;

    tagname = NULL;
    ufid = NULL;
    if (strcmp (id3frame->id, "TXXX") != 0 &&
        strcmp (id3frame->id, "UFID") != 0) {
      tagname = atidata->tagLookup (tagtype, id3frame->id);
      if (tagname == NULL) {
        tagname = id3frame->id;
      }
    }

    for (size_t i = 0; i < id3frame->nfields; ++i) {
      const union id3_field   *field;

      field = &id3frame->fields [i];

      switch (id3frame->fields [i].type) {
        case ID3_FIELD_TYPE_TEXTENCODING: {
          enum id3_field_textencoding   te;

          te = id3_field_gettextencoding (field);
          break;
        }
        case ID3_FIELD_TYPE_FRAMEID: {
          char const *str;

          str = id3_field_getframeid (field);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1: {
          id3_latin1_t const  *str;

          str = id3_field_getlatin1 (field);
          if (strcmp (id3frame->id, "UFID") == 0) {
            ufid = str;
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (1): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (2): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1LIST: {
          break;
        }
        case ID3_FIELD_TYPE_STRING: {
          id3_ucs4_t const *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          if (tagname == NULL) {
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (3): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (4): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr;
            id3_utf8_t        *str;
            const char        *p;

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (5): %s %s=%s", tagname, id3frame->id, str);
            p = (const char *) str;
            if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            slistSetStr (tagdata, tagname, p);
          }
          break;
        }
        case ID3_FIELD_TYPE_LANGUAGE: {
          break;
        }
        case ID3_FIELD_TYPE_DATE: {
          id3_latin1_t const *str;

          str = id3_field_getlatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (6): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_INT8:
        case ID3_FIELD_TYPE_INT16:
        case ID3_FIELD_TYPE_INT24:
        case ID3_FIELD_TYPE_INT32:
        case ID3_FIELD_TYPE_INT32PLUS: {
          long    val;
          char    tmp [40];

          val = id3_field_getint (field);
          snprintf (tmp, sizeof (tmp), "%ld", val);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (7): %s %s=%s", tagname, id3frame->id, tmp);
          slistSetStr (tagdata, tagname, tmp);
          break;
        }
        case ID3_FIELD_TYPE_BINARYDATA: {
          id3_byte_t const  *bstr;
          id3_length_t      blen;
          char              tmp [100];

          if (ufid != NULL && strcmp ((const char *) ufid, MB_TAG) == 0) {
            bstr = id3_field_getbinarydata (field, &blen);
            memcpy (tmp, bstr, blen);
            tmp [blen] = '\0';
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (8): %s %s=%s=%s", tagname, id3frame->id, ufid, tmp);
            slistSetStr (tagdata, tagname, tmp);
          }
          break;
        }
      }
    }
    ++idx;
  }

  id3_file_close (id3file);
}

static void
atibdj4ParseOggTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggVorbis_File        ovf;
  int                   rc;
  struct vorbis_comment *vc;
  double                ddur;
  char                  tmp [40];

  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return;
  }

  vc = ovf.vc;
  if (vc == NULL) {
    return;
  }

  ddur = ov_time_total (&ovf, -1);
  snprintf (tmp, sizeof (tmp), "%.0f", ddur * 1000);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", tmp);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);

  for (int i = 0; i < vc->comments; ++i) {
    const char  *kw;

    kw = vc->user_comments [i];
    atibdj4ProcessVorbisComment (atidata, tagdata, tagtype, kw);
  }
  ov_clear (&ovf);
  return;
}

static void
atibdj4ParseOpusTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  OggOpusFile           *of;
  int                   linknum;
  const OpusTags        *tags = NULL;
  int                   rc;

  of = op_open_file (ffn, &rc);
  if (rc < 0 || of == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return;
  }

  linknum = op_current_link (of);
  tags = op_tags (of, linknum);
  if (tags == NULL) {
    return;
  }

  for (int i = 0; i < tags->comments; ++i) {
    const char  *kw;

    kw = tags->user_comments [i];
    if (opus_tagncompare ("METADATA_BLOCK_PICTURE", 22, kw) == 0) {
      continue;
    }

    atibdj4ProcessVorbisComment (atidata, tagdata, tagtype, kw);
  }
  op_free (of);
  return;
}

static void
atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  FLAC__StreamMetadata    *block;
  bool                    cont;

  chain = FLAC__metadata_chain_new ();
  FLAC__metadata_chain_read (chain, ffn);
  iterator = FLAC__metadata_iterator_new ();
  FLAC__metadata_iterator_init (iterator, chain);
  cont = true;
  while (cont) {
    block = FLAC__metadata_iterator_get_block (iterator);
    switch (block->type) {
      case FLAC__METADATA_TYPE_PICTURE:
      case FLAC__METADATA_TYPE_CUESHEET:
      case FLAC__METADATA_TYPE_SEEKTABLE:
      case FLAC__METADATA_TYPE_APPLICATION:
      case FLAC__METADATA_TYPE_PADDING: {
        break;
      }
      case FLAC__METADATA_TYPE_STREAMINFO: {
        int64_t   dur;
        char      tmp [40];

        /* want duration in milliseconds */
        dur = block->data.stream_info.total_samples * 1000;
        dur /= block->data.stream_info.sample_rate;
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG,
            "total-samples: %" PRId64 " sample-rate: %d",
            block->data.stream_info.total_samples, block->data.stream_info.sample_rate);
        snprintf (tmp, sizeof (tmp), "%" PRId64, dur);
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "duration: %s", tmp);
        slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);
        break;
      }
      case FLAC__METADATA_TYPE_VORBIS_COMMENT: {
        for (FLAC__uint32 i = 0; i < block->data.vorbis_comment.num_comments; i++) {
          FLAC__StreamMetadata_VorbisComment_Entry *entry;

          entry = &block->data.vorbis_comment.comments [i];
          atibdj4ProcessVorbisComment (atidata, tagdata, tagtype,
              (const char *) entry->entry);
        }
        break;
      }
      default: {
        break;
      }
    }
    cont = FLAC__metadata_iterator_next (iterator);
  }

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);
  return;
}

static int
atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

static int
atibdj4WriteOggTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  OggVorbis_File        ovf;
  int                   rc = -1;
  struct vorbis_comment *vc;
  struct vorbis_comment newvc;
  slistidx_t            iteridx;
  slist_t               *upddone;
  const char            *key;

  rc = ov_fopen (ffn, &ovf);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bad return %d %s", rc, ffn);
    return -1;
  }

  vc = ovf.vc;
  if (vc == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no vc %s", ffn);
    return -1;
  }

  vorbis_comment_init (&newvc);
  upddone = slistAlloc ("upd-done", LIST_ORDERED, NULL);

  for (int i = 0; i < vc->comments; ++i) {
    const char  *kw;
    const char  *val;
    char        ttag [300];

    kw = vc->user_comments [i];
    val = atibdj4ParseVorbisComment (kw, ttag, sizeof (ttag));
    if (slistGetStr (dellist, ttag) != NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: del: %s", ttag);
      continue;
    }
    if (slistGetStr (updatelist, ttag) != NULL) {
      val = slistGetStr (updatelist, ttag);
      vorbis_comment_add_tag (&newvc, ttag, val);
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", ttag, val);
      slistSetNum (upddone, ttag, 1);
    } else {
      /* the tag has not changed, or is unknown to bdj4 */
      vorbis_comment_add (&newvc, kw);
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: existing: %s", kw);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    if (slistGetNum (upddone, key) > 0) {
      continue;
    }
    vorbis_comment_add_tag (&newvc, key, slistGetStr (updatelist, key));
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: new: %s=%s", key, slistGetStr (updatelist, key));
  }
  slistFree (upddone);

  rc = atibdj4WriteOggFile (ffn, &newvc);

  vorbis_comment_clear (&newvc);
  ov_clear (&ovf);

  return rc;
}

static int
atibdj4WriteOpusTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

static int
atibdj4WriteFlacTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__StreamMetadata    *block = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  const char              *key;
  slistidx_t              iteridx;
  bool                    cont;

  chain = FLAC__metadata_chain_new ();
  FLAC__metadata_chain_read (chain, ffn);

  /* find the comment block */
  iterator = FLAC__metadata_iterator_new ();
  FLAC__metadata_iterator_init (iterator, chain);
  cont = true;
  while (cont) {
    block = FLAC__metadata_iterator_get_block (iterator);
    if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
      break;
    }
    cont = FLAC__metadata_iterator_next (iterator);
  }

  if (block == NULL) {
    /* if the comment block was not found, create a new one */
    block = FLAC__metadata_object_new (FLAC__METADATA_TYPE_VORBIS_COMMENT);
    while (FLAC__metadata_iterator_next (iterator)) {
      ;
    }
    if (! FLAC__metadata_iterator_insert_block_after (iterator, block)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: unable to insert new comment block");
      return -1;
    }
  }

  /* when updating, remove the entry from the vorbis comment first */
  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    FLAC__metadata_object_vorbiscomment_remove_entries_matching (block, key);
  }
  slistStartIterator (dellist, &iteridx);
  while ((key = slistIterateKey (dellist, &iteridx)) != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: del: %s", key);
    FLAC__metadata_object_vorbiscomment_remove_entries_matching (block, key);
  }

  /* add all the updates back into the vorbis comment */
  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    FLAC__StreamMetadata_VorbisComment_Entry  entry;

    if (! FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair (
        &entry, key, slistGetStr (updatelist, key))) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: invalid data: %s %s", key, slistGetStr (updatelist, key));
      continue;
    }
    entry.length = strlen ((const char *) entry.entry);
    if (! FLAC__format_vorbiscomment_entry_is_legal (entry.entry, entry.length)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: not legal: %s %s", key, slistGetStr (updatelist, key));
      continue;
    }

    if (! FLAC__metadata_object_vorbiscomment_append_comment (
        block, entry, /*copy*/ false)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: unable to append: %s %s", key, slistGetStr (updatelist, key));
      continue;
    }
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", key, slistGetStr (updatelist, key));
  }

  FLAC__metadata_chain_sort_padding (chain);
  FLAC__metadata_chain_write (chain, true, true);

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);
  return 0;
}

static void
atibdj4ProcessVorbisComment (atidata_t *atidata, slist_t *tagdata,
    int tagtype, const char *kw)
{
  const char  *val;
  const char  *tagname;
  char        ttag [300];

  val = atibdj4ParseVorbisComment (kw, ttag, sizeof (ttag));
  tagname = atidata->tagLookup (tagtype, ttag);
  if (tagname == NULL) {
    tagname = ttag;
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s %s", tagname, kw);
  slistSetStr (tagdata, tagname, val);
}

static const char *
atibdj4ParseVorbisComment (const char *kw, char *buff, size_t sz)
{
  const char  *val;
  size_t      len;

  val = strstr (kw, "=");
  if (val == NULL) {
    return NULL;
  }
  len = val - kw;
  if (len > sz) {
    len = sz - 1;
  }
  strlcpy (buff, kw, len + 1);
  buff [len] = '\0';
  ++val;

  return val;
}


/* from tagutil: BSD 2-Clause License */
/* originally posted at : https://kaworu.ch/blog/2013/09/29/writting-ogg-slash-vorbis-comment-in-c/ */
static int
atibdj4WriteOggPage (ogg_page *p, FILE *fp)
{
  if (fwrite (p->header, 1, p->header_len, fp) != (size_t) p->header_len) {
    return -1;
  }
  if (fwrite (p->body, 1, p->body_len, fp) != (size_t) p->body_len) {
    return -1;
  }
  return 0;
}

/* from tagutil: BSD 2-Clause License */
/* originally posted at : https://kaworu.ch/blog/2013/09/29/writting-ogg-slash-vorbis-comment-in-c/ */
static int
atibdj4WriteOggFile (const char *ffn, struct vorbis_comment *newvc)
{
  FILE             *ifh  = NULL;
  char              tbuff [MAXPATHLEN];
  char              outfn [MAXPATHLEN];
  int               rc = -1;
  time_t            omodtime;
  FILE             *ofh = NULL;
  ogg_sync_state    oy_in;
  ogg_stream_state  os_in;
  ogg_stream_state  os_out;
  ogg_page          og_in;
  ogg_page          og_out;
  ogg_packet        op_in;
  ogg_packet        my_vc_packet;
  vorbis_info       vi_in;
  vorbis_comment    vc_in;
  unsigned long     nstream_in;
  unsigned long     npage_in;
  unsigned long     npacket_in;
  unsigned long     blocksz;
  unsigned long     lastblocksz;
  ogg_int64_t       granulepos;
  enum {
    VC_BUILD_PACKET,
    VC_SETUP,
    VC_BEG_OF_STREAM,
    VC_START_READING,
    VC_STREAMS_INITIALIZED,
    VC_READING_HEADERS,
    VC_READING_DATA,
    VC_READING_DATA_NEED_FLUSH,
    VC_READING_DATA_NEED_PAGEOUT,
    VC_END_OF_STREAM,
    VC_WRITE_FINISH,
    VC_DONE_SUCCESS,
  } state;

  /*
   * Replace the 2nd ogg packet (vorbis comment) and copy the rest
   * See "Metadata workflow":
   * https://xiph.org/vorbis/doc/libvorbis/overview.html
   */

  state = VC_BUILD_PACKET;
  if (vorbis_commentheader_out (newvc, &my_vc_packet) != 0) {
    goto cleanup_label;
  }

  state = VC_SETUP;
  (void) ogg_sync_init (&oy_in); /* always return 0 */
  if ((ifh = fileopOpen (ffn, "r")) == NULL) {
    goto cleanup_label;
  }
  snprintf (outfn, sizeof (outfn), "%s.ogg-tmp", ffn);
  if ((ofh = fileopOpen (outfn, "w")) == NULL) {
    goto cleanup_label;
  }
  lastblocksz = granulepos = 0;

  nstream_in = 0;

bos_label:
  state = VC_BEG_OF_STREAM;
  nstream_in += 1;
  npage_in = npacket_in = 0;
  vorbis_info_init (&vi_in);
  vorbis_comment_init (&vc_in);

  state = VC_START_READING;
  while (state != VC_END_OF_STREAM) {
    switch (ogg_sync_pageout (&oy_in, &og_in)) {
    case 0:
      /* more data needed or an internal error occurred. */
    case -1:
      /* stream has not yet captured sync (bytes were skipped). */
      if (feof (ifh)) {
        if (state < VC_READING_DATA) {
          goto cleanup_label;
        }
        state = VC_END_OF_STREAM;
      } else {
        char *buf;
        size_t s;

        if ((buf = ogg_sync_buffer (&oy_in, BUFSIZ)) == NULL) {
          goto cleanup_label;
        }
        if ((s = fread (buf, sizeof (char), BUFSIZ, ifh)) == 0) {
          goto cleanup_label;
        }
        if (ogg_sync_wrote (&oy_in, s) == -1) {
          goto cleanup_label;
        }
      }
      continue;
    }
    if (++npage_in == 1) {
      /* init both input and output streams with the serialno
         of the first page */
      if (ogg_stream_init (&os_in, ogg_page_serialno (&og_in)) == -1) {
        goto cleanup_label;
      }
      if (ogg_stream_init (&os_out, ogg_page_serialno (&og_in)) == -1) {
        ogg_stream_clear (&os_in);
        goto cleanup_label;
      }
      state = VC_STREAMS_INITIALIZED;
    }

    if (ogg_stream_pagein (&os_in, &og_in) == -1) {
      goto cleanup_label;
    }
    while (ogg_stream_packetout (&os_in, &op_in) == 1) {
      ogg_packet *target;

      if (++npacket_in == 2 && nstream_in == 1) {
        target = &my_vc_packet;
      } else {
        target = &op_in;
      }

      if (npacket_in <= 3) {
        if (vorbis_synthesis_headerin (&vi_in, &vc_in, &op_in) != 0) {
          goto cleanup_label;
        }
        state = (npacket_in == 3 ? VC_READING_DATA_NEED_FLUSH : VC_READING_HEADERS);
      } else {
        /*
         * granulepos computation.
         *
         * The granulepos is stored into the *pages* and
         * is used by the codec to seek through the
         * bitstream.  Its value is codec dependent (in
         * the Vorbis case it is the number of samples
         * elapsed).
         *
         * The vorbis_packet_blocksize () actually
         * compute the number of sample that would be
         * stored by the packet (without decoding it).
         * This is the same formula as in vcedit example
         * from vorbis-tools.
         *
         * We use here the vorbis_info previously filled
         * when reading header packets.
         *
         * XXX: check if this is not a vorbis stream ?
         */
        blocksz = vorbis_packet_blocksize (&vi_in, &op_in);
        granulepos += (lastblocksz == 0 ? 0 : (blocksz + lastblocksz) / 4);
        lastblocksz = blocksz;

        if (state == VC_READING_DATA_NEED_FLUSH) {
          while (ogg_stream_flush (&os_out, &og_out)) {
            if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
              goto cleanup_label;
            }
          }
        } else if (state == VC_READING_DATA_NEED_PAGEOUT) {
          while (ogg_stream_pageout (&os_out, &og_out)) {
            if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
              goto cleanup_label;
            }
          }
        }

        /*
         * Decide wether we need to write a page based
         * on our granulepos computation. The -1 case is
         * very common because only the last packet of a
         * page has its granulepos set by the ogg layer
         * (which only store a granulepos per page), so
         * all the other have a value of -1 (we need to
         * set the granulepos for each packet though).
         *
         * The other cases logic are borrowed from
         * vcedit and I fail to understand how
         * granulepos could mismatch because we don't
         * change the data packet.
         */
        state = VC_READING_DATA;
        if (op_in.granulepos == -1) {
          op_in.granulepos = granulepos;
        } else if (granulepos <= op_in.granulepos) {
          state = VC_READING_DATA_NEED_PAGEOUT;
        } else /* if granulepos > op_in.granulepos */ {
          state = VC_READING_DATA_NEED_FLUSH;
          granulepos = op_in.granulepos;
        }
      }
      if (ogg_stream_packetin (&os_out, target) == -1) {
        goto cleanup_label;
      }
    }
    if (ogg_page_eos (&og_in)) {
      state = VC_END_OF_STREAM;
    }
  }

  /* forces remaining packets into a last page */
  os_out.e_o_s = 1;
  while (ogg_stream_flush (&os_out, &og_out)) {
    if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
      goto cleanup_label;
    }
  }

  if (! feof (ifh)) {
    ogg_stream_clear (&os_in);
    ogg_stream_clear (&os_out);
    vorbis_comment_clear (&vc_in);
    vorbis_info_clear (&vi_in);
    /* ogg/vorbis supports multiple streams; don't lose this data */
    /* even though bdj4 doesn't support these */
    goto bos_label;
  } else {
    fclose (ifh);
    ifh = NULL;
  }

  state = VC_WRITE_FINISH;
  if (ofh != stdout && fclose (ofh) != 0) {
    goto cleanup_label;
  }
  ofh = NULL;
  state = VC_DONE_SUCCESS;

  omodtime = fileopModTime (ffn);
  snprintf (tbuff, sizeof (tbuff), "%s.bak", ffn);
  if (filemanipMove (ffn, tbuff) == 0) {
    if (filemanipMove (outfn, ffn) == 0) {
      fileopSetModTime (ffn, omodtime);
      fileopDelete (tbuff);
      rc = 0;
    } else {
      filemanipMove (tbuff, ffn);
    }
  }

cleanup_label:
  if (state >= VC_STREAMS_INITIALIZED) {
    ogg_stream_clear (&os_in);
    ogg_stream_clear (&os_out);
  }
  if (state >= VC_START_READING) {
    vorbis_comment_clear (&vc_in);
    vorbis_info_clear (&vi_in);
  }
  ogg_sync_clear (&oy_in);
  if (ofh != stdout && ofh != NULL) {
    fclose (ofh);
  }
  if (ifh != NULL) {
    fclose (ifh);
  }
  ogg_packet_clear (&my_vc_packet);

  return rc;
}


/* for logging ffmpeg output */
static void
atibdj4LogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
//  vfprintf (stderr, fmt, vl);
  return;
}

