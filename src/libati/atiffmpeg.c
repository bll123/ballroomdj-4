/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include "bdj4.h"
#include "ati.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

const char *
atiiDesc (void)
{
  return "ffmpeg";
}

static const char *atiffmpegTagLookup (atidata_t *atidata, const char *key, int tagtype);
static const char *atiffmpegReverseTagLookup (int tagkey, const char *key, int tagtype);
static void atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl);

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
  av_log_set_callback (atiffmpegLogCallback);
  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    mdfree (atidata);
  }
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char    *data;

  data = mdstrdup (ffn);
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data,
    int tagtype, int *rewrite)
{
  AVFormatContext         *ictx = NULL;
  const AVDictionary      *dict = NULL;
  const AVDictionaryEntry *tag = NULL;
  int                     rc;
  int64_t                 duration;
  char                    pbuff [100];
  int                     writetags;

  writetags = atidata->writetags;

  if ((rc = avformat_open_input (&ictx, data, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s\n", rc, data);
    return;
  }

  if ((rc = avformat_find_stream_info (ictx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s\n", rc, data);
    avformat_close_input (&ictx);
    return;
  }

  duration = ictx->duration;
  duration /= 1000;
  snprintf (pbuff, sizeof (pbuff), "%ld", (long) duration);
  slistSetStr (tagdata, atidata->tagName (TAG_DURATION), pbuff);

  dict = ictx->metadata;
  /* ogg stores the metadata in with the stream, apparently */
  if (av_dict_count (dict) == 0) {
    for (unsigned int i = 0; i < ictx->nb_streams; ++i) {
      dict = ictx->streams [i]->metadata;
      if (av_dict_count (dict) > 0) {
        break;
      }
    }
  }

  while ((tag = av_dict_get (dict, "", tag, AV_DICT_IGNORE_SUFFIX)) != NULL) {
    const char  *tagname;
    int         tagkey;
    int         tagnamecount = 0;     // for debugging

    tagname = atiffmpegTagLookup (atidata, tag->key, tagtype);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "%s (%d) raw: %s=%s", tagname, tagnamecount, tag->key, tag->value);

    if (tagname != NULL && *tagname) {
      const char  *p;

      p = tag->value;

      if (strcmp (tag->key, "VARIOUSARTISTS") == 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rewrite: various");
        *rewrite |= AF_REWRITE_VARIOUS;
        continue;
      }

      if (strcmp (tag->key, "DURATION") == 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rewrite: duration");
        *rewrite |= AF_REWRITE_DURATION;
        continue;
      }

      /* again, use the ffmpeg tag type here */
      tagkey = tagdefLookup (tagname);

      if (tagkey == TAG_TRACKNUMBER) {
        p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
            p, pbuff, sizeof (pbuff));
      }

      /* disc number / disc total handling */
      if (tagkey == TAG_DISCNUMBER) {
        p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
            p, pbuff, sizeof (pbuff));
      }

      slistSetStr (tagdata, tagname, p);
    }
  }

  avformat_close_input (&ictx);
  return;
}


int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  AVFormatContext   *ictx = NULL;
  const AVDictionary      *dict = NULL;
  const AVDictionaryEntry *avtag = NULL;
  AVDictionary      *ndict = NULL;
  AVFormatContext   *octx = NULL;
  AVOutputFormat    *ofmt = NULL;
  AVPacket          *pkt = NULL;
  int               rc = 0;
  int               status;
  int               audio_stream_index;
  slistidx_t        iteridx;
  const char        *key;
  int               metadata_idx = -1;
  pathinfo_t        *pi;
  char              tmpfn [MAXPATHLEN];

  if ((rc = avformat_open_input (&ictx, ffn, NULL, NULL))) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to open %d %s\n", rc, ffn);
    return -1;
  }

  if ((rc = avformat_find_stream_info (ictx, NULL)) < 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unable to load stream info %d %s\n", rc, ffn);
    avformat_close_input (&ictx);
    return -1;
  }

  av_dump_format (ictx, 0, ffn, 0);

  pi = pathInfo (ffn);
  snprintf (tmpfn, sizeof (tmpfn), "%.*s/tmp-%.*s",
      (int) pi->dlen, pi->dirname, (int) pi->flen, pi->filename);

  status = avformat_alloc_output_context2 (&octx, NULL, NULL, tmpfn);
  if (status < 0) {
    fprintf (stderr, "unable to open output %d\n", status);
    avformat_close_input (&ictx);
    return -1;
  }

  ofmt = octx->oformat;

  audio_stream_index = 0;
  for (unsigned i = 0; i < ictx->nb_streams; i++) {
    if (ictx->streams [i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      AVStream *ostream;

      audio_stream_index = i;
      ostream = avformat_new_stream (octx, NULL);
      if (ostream == NULL) {
        avformat_close_input (&ictx);
        avformat_free_context (octx);
        return -1;
      }
      avcodec_parameters_copy (ostream->codecpar, ictx->streams [i]->codecpar);
      ostream->codecpar->codec_tag = 0;
      break;
    }
  }

  /* first, copy all of the metadata */

  dict = ictx->metadata;
  /* ogg stores the metadata in with the stream, apparently */
  if (av_dict_count (dict) == 0) {
    for (unsigned int i = 0; i < ictx->nb_streams; ++i) {
      dict = ictx->streams [i]->metadata;
      if (av_dict_count (dict) > 0) {
        metadata_idx = i;
        break;
      }
    }
  }

  while ((avtag = av_dict_get (dict, "", avtag, AV_DICT_IGNORE_SUFFIX)) != NULL) {
    const char  *tagname;
    const char  *tag;
    int         tagkey;

    tagname = atiffmpegTagLookup (atidata, avtag->key, tagtype);
    if (tagname != NULL && *tagname) {
      if (slistGetStr (dellist, tagname) != NULL) {
        continue;
      }
    }
    tagkey = tagdefLookup (tagname);
    tag = atiffmpegReverseTagLookup (tagkey, tagname, tagtype);
    if (tag == NULL) {
      tag = avtag->key;
    }
    av_dict_set (&ndict, tag, avtag->value, 0);
  }

  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    int         tagkey;
    const char  *tag;

    tagkey = tagdefLookup (key);
    tag = atiffmpegReverseTagLookup (tagkey, key, tagtype);
    if (tag == NULL) {
      tag = key;
    }
    av_dict_set (&ndict, tag, slistGetStr (updatelist, key), 0);
  }

  if (metadata_idx >= 0) {
    octx->streams [metadata_idx]->metadata = ndict;
  } else {
    octx->metadata = ndict;
  }

  av_dump_format (octx, 0, tmpfn, 1);

  if (! (ofmt->flags & AVFMT_NOFILE)) {
    avio_open (&octx->pb, tmpfn, AVIO_FLAG_WRITE);
  }

  status = avformat_write_header (octx, NULL);
  if (status < 0) {
    avformat_close_input (&ictx);
    if (! (ofmt->flags & AVFMT_NOFILE)) {
      avio_closep (&octx->pb);
    }
    avformat_free_context (octx);
    return -1;
  }

  pkt = av_packet_alloc();
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;

  while (av_read_frame (ictx, pkt) == 0) {
    if (pkt->stream_index == audio_stream_index) {
      pkt->stream_index = 0;
      av_packet_rescale_ts (pkt, ictx->streams [audio_stream_index]->time_base,
          octx->streams [0]->time_base);
      pkt->pos = -1;
//      av_write_frame (octx, pkt);
      if (av_interleaved_write_frame (octx, pkt) < 0) {
        avformat_close_input (&ictx);
        if (! (ofmt->flags & AVFMT_NOFILE)) {
          avio_closep (&octx->pb);
        }
        avformat_free_context (octx);
      }
    }
  }

  av_packet_free (&pkt);

  av_write_trailer (octx);

  avformat_close_input (&ictx);
  if (! (ofmt->flags & AVFMT_NOFILE)) {
    avio_closep (&octx->pb);
  }
  avformat_free_context (octx);

  return rc;
}

static const char *
atiffmpegTagLookup (atidata_t *atidata, const char *key, int tagtype)
{
  const char  *tagname = NULL;

  /* use the ffmpeg tag type, not the audio file type, */
  /* as ffmpeg converts the tag names */
  /* then if not found, try the vorbis name */
  tagname = atidata->tagLookup (TAG_TYPE_FFMPEG, key);
  if (tagname == NULL) {
    tagname = atidata->tagLookup (TAG_TYPE_VORBIS, key);
    if (tagname == NULL) {
      tagname = atidata->tagLookup (tagtype, key);
    }
  }

  return tagname;
}

static const char *
atiffmpegReverseTagLookup (int tagkey, const char *key, int tagtype)
{
  const char  *tag = NULL;

  if (tagkey < 0) {
    return tag;
  }

  tag = tagdefs [tagkey].audiotags [TAG_TYPE_FFMPEG].tag;
  /* the vorbis tags are not translated back by ffmpeg */
  /* bpm and the musicbrainz tags are not translated back by ffmpeg */
  if (tag == NULL ||
      tagtype == TAG_TYPE_VORBIS ||
      tagkey == TAG_BPM ||
      tagkey == TAG_TAGS ||
      tagkey == TAG_RECORDING_ID ||
      tagkey == TAG_TRACK_ID ||
      tagkey == TAG_WORK_ID) {
    if (tagtype == TAG_TYPE_MP3) {
      /* ffmpeg automatically puts the TXXX in front */
      tag = tagdefs [tagkey].audiotags [tagtype].desc;
    } else {
      tag = tagdefs [tagkey].audiotags [tagtype].tag;
    }
  }

  return tag;
}

static void
atiffmpegLogCallback (void *avcl, int level, const char *fmt, va_list vl)
{
//  vfprintf (stderr, fmt, vl);
  return;
}

