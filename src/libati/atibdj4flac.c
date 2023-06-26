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

#include <FLAC/format.h>
#include <FLAC/metadata.h>

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
  FLAC__StreamMetadata  *vcblock;
  bool                  haspicture;
} atisaved_t;

static void atibdj4FlacAddVorbisComment (FLAC__StreamMetadata *block, FLAC__StreamMetadata_VorbisComment_Entry *entry, int tagkey, const char *tagname, const char *val);

void
atibdj4ParseFlacTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  FLAC__StreamMetadata    *block;
  bool                    cont;

  chain = FLAC__metadata_chain_new ();
  if (! FLAC__metadata_chain_read (chain, ffn)) {
    return;
  }
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
          atiProcessVorbisCommentCombined (atidata->tagLookup, tagdata, tagtype,
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

int
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
  int                     writetags;
  int                     tagkey;

  writetags = atidata->writetags;
  chain = FLAC__metadata_chain_new ();
  if (! FLAC__metadata_chain_read (chain, ffn)) {
    return -1;
  }

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
    tagkey = atidata->tagCheck (writetags, tagtype, key, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

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

    tagkey = atidata->tagCheck (writetags, tagtype, key, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    atibdj4FlacAddVorbisComment (block, &entry, tagkey, key, slistGetStr (updatelist, key));
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", key, slistGetStr (updatelist, key));
  }

  FLAC__metadata_chain_sort_padding (chain);
  FLAC__metadata_chain_write (chain, true, true);

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);
  return 0;
}

atisaved_t *
atibdj4SaveFlacTags (atidata_t *atidata, const char *ffn,
    int tagtype, int filetype)
{
  atisaved_t              *atisaved;
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__StreamMetadata    *block = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  bool                    cont;
  bool                    haspicture = false;

  chain = FLAC__metadata_chain_new ();
  FLAC__metadata_chain_read (chain, ffn);

  /* find the comment block */
  /* and check and see if any pictures are present */
  iterator = FLAC__metadata_iterator_new ();
  FLAC__metadata_iterator_init (iterator, chain);
  cont = true;
  while (cont) {
    FLAC__StreamMetadata    *tblock;

    tblock = FLAC__metadata_iterator_get_block (iterator);
    if (tblock->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
      block = tblock;
      if (haspicture) {
        break;
      }
    }
    if (tblock->type == FLAC__METADATA_TYPE_PICTURE) {
      haspicture = true;
      if (block != NULL) {
        break;
      }
    }
    cont = FLAC__metadata_iterator_next (iterator);
  }

  if (block == NULL) {
    return NULL;
  }

  atisaved = mdmalloc (sizeof (atisaved_t));
  atisaved->hasdata = true;
  atisaved->tagtype = tagtype;
  atisaved->filetype = filetype;
  atisaved->vcblock = FLAC__metadata_object_new (FLAC__METADATA_TYPE_VORBIS_COMMENT);
  atisaved->haspicture = haspicture;
  for (FLAC__uint32 i = 0; i < block->data.vorbis_comment.num_comments; i++) {
    FLAC__StreamMetadata_VorbisComment_Entry *entry;

    entry = &block->data.vorbis_comment.comments [i];
    FLAC__metadata_object_vorbiscomment_append_comment (atisaved->vcblock, *entry, true);
  }

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);

  return atisaved;
}

void
atibdj4RestoreFlacTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__StreamMetadata    *block = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  bool                    cont;

  if (atisaved == NULL) {
    return;
  }

  if (! atisaved->hasdata) {
    return;
  }

  if (atisaved->tagtype != tagtype) {
    return;
  }
  if (atisaved->filetype != filetype) {
    return;
  }

  chain = FLAC__metadata_chain_new ();
  if (! FLAC__metadata_chain_read (chain, ffn)) {
    return;
  }

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
      return;
    }
  }

  FLAC__metadata_object_vorbiscomment_resize_comments (block, 0);
  for (FLAC__uint32 i = 0; i < atisaved->vcblock->data.vorbis_comment.num_comments; i++) {
    FLAC__StreamMetadata_VorbisComment_Entry *entry;

    entry = &atisaved->vcblock->data.vorbis_comment.comments [i];
    FLAC__metadata_object_vorbiscomment_append_comment (block, *entry, true);
  }

  FLAC__metadata_chain_sort_padding (chain);
  FLAC__metadata_chain_write (chain, true, true);

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);

  if (atisaved->haspicture) {
    /* in this case, the entire file must be re-written */
  }

  FLAC__metadata_object_delete (atisaved->vcblock);
  atisaved->hasdata = false;
  mdfree (atisaved);
  return;
}

void
atibdj4CleanFlacTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  FLAC__Metadata_Chain    *chain = NULL;
  FLAC__StreamMetadata    *block = NULL;
  FLAC__Metadata_Iterator *iterator = NULL;
  bool                    cont;

  chain = FLAC__metadata_chain_new ();
  if (! FLAC__metadata_chain_read (chain, ffn)) {
    return;
  }

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
      return;
    }
  }

  FLAC__metadata_object_vorbiscomment_resize_comments (block, 0);
  FLAC__metadata_chain_sort_padding (chain);
  FLAC__metadata_chain_write (chain, true, true);

  FLAC__metadata_iterator_delete (iterator);
  FLAC__metadata_chain_delete (chain);
  return;
}

void
atibdj4LogFlacVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libflac version %s", FLAC__VERSION_STRING);
}

static void
atibdj4FlacAddVorbisComment (FLAC__StreamMetadata *block,
    FLAC__StreamMetadata_VorbisComment_Entry *entry,
    int tagkey, const char *tagname, const char *val)
{
  slist_t     *vallist;
  slistidx_t  viteridx;
  const char  *tval;

  vallist = atiSplitVorbisComment (tagkey, tagname, val);
  slistStartIterator (vallist, &viteridx);
  while ((tval = slistIterateKey (vallist, &viteridx)) != NULL) {
    if (! FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair (
        entry, tagname, tval)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: invalid data: %s %s", tagname, val);
      return;
    }

    entry->length = strlen ((const char *) entry->entry);
    if (! FLAC__format_vorbiscomment_entry_is_legal (entry->entry, entry->length)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: not legal: %s %s", tagname, val);
      return;
    }

    if (! FLAC__metadata_object_vorbiscomment_append_comment (
        block, *entry, /*copy*/ false)) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "ERR: flac: write: unable to append: %s %s", tagname, val);
      return;
    }
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: update: %s=%s", tagname, val);
  slistFree (vallist);
}

