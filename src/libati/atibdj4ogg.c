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

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define OV_EXCLUDE_STATIC_CALLBACKS 1
#include <vorbis/vorbisfile.h>
#undef OV_EXCLUDE_STATIC_CALLBACKS

#include "ati.h"
#include "atibdj4.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

static int  atibdj4WriteOggPage (ogg_page *p, FILE *fp);
static int  atibdj4WriteOggFile (const char *ffn, struct vorbis_comment *newvc);

void
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

int
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

atisaved_t *
atibdj4SaveOggTags (atidata_t *atidata, const char *ffn,
    int tagtype, int filetype)
{
  return NULL;
}

void
atibdj4RestoreOggTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  return;
}

void
atibdj4LogOggVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libvorbis version %s", vorbis_version_string());
}

/* internal routines */

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
  char              outfn [MAXPATHLEN];
  int               rc = -1;
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

  atiReplaceFile (ffn, outfn);

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


