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
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

void
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

atisaved_t *
atibdj4SaveFlacTags (atidata_t *atidata, const char *ffn,
    int tagtype, int filetype)
{
  return NULL;
}

void
atibdj4RestoreFlacTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  return;
}

void
atibdj4LogFlacVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libflac version %s", FLAC__VERSION_STRING);
}

