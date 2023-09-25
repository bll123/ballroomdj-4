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

#include <opus/opusfile.h>

#include "atioggutil.h"
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"

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
};

static int  atibdj4WriteOggPage (ogg_page *p, FILE *fp);
static void atibdj4_oggpack_string (oggpack_buffer *b, const char *s, size_t len);

void
atioggProcessVorbisCommentCombined (taglookup_t tagLookup, slist_t *tagdata,
    int tagtype, const char *kw)
{
  const char  *val;
  char        ttag [300];

  val = atioggParseVorbisComment (kw, ttag, sizeof (ttag));
  /* vorbis comments are not case sensitive */
  stringAsciiToUpper (ttag);
  if (strcmp (ttag, "METADATA_BLOCK_PICTURE") == 0) {
    /* this is a lot of data to carry around, and it's not needed */
    return;
  }
  atioggProcessVorbisComment (tagLookup, tagdata, tagtype, ttag, val);
}

void
atioggProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata,
    int tagtype, const char *tag, const char *val)
{
  const char  *tagname;
  bool        exists = false;
  char        *tmp;

  /* vorbis comments are not case sensitive */
  tagname = tagLookup (tagtype, tag);
  if (tagname == NULL) {
    tagname = tag;
  }
  if (strcmp (tagname, "TOTALTRACKS") == 0) {
    tagname = "TRACKTOTAL";
  }
  if (strcmp (tagname, "TOTALDISCS") == 0) {
    tagname = "DISCTOTAL";
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s %s=%s", tagname, tag, val);
  if (slistGetStr (tagdata, tagname) != NULL) {
    const char  *oval;
    size_t      sz;

    oval = slistGetStr (tagdata, tagname);
    sz = strlen (oval) + strlen (val) + 3;
    tmp = mdmalloc (sz);
    snprintf (tmp, sz, "%s; %s\n", oval, val);
    val = tmp;
    exists = true;
  }
  slistSetStr (tagdata, tagname, val);
  if (exists) {
    mdfree (tmp);
  }
}

const char *
atioggParseVorbisComment (const char *kw, char *buff, size_t sz)
{
  const char  *val;
  size_t      len;

  val = strstr (kw, "=");
  if (val == NULL) {
    return NULL;
  }
  len = val - kw;
  if (len >= sz) {
    len = sz - 1;
  }
  strlcpy (buff, kw, len + 1);
  buff [len] = '\0';
  ++val;

  return val;
}

/* the caller must free the return value */
slist_t *
atioggSplitVorbisComment (int tagkey, const char *tagname, const char *val)
{
  slist_t     *vallist = NULL;
  char        *tokstr;
  char        *p;
  bool        split = false;

  /* known tags that should be split: */
  /* genre, artist, album-artist, composer */

  vallist = slistAlloc ("vc-list", LIST_UNORDERED, NULL);
  split = tagkey == TAG_ALBUMARTIST ||
      tagkey == TAG_ARTIST ||
      tagkey == TAG_COMPOSER ||
      tagkey == TAG_CONDUCTOR ||
      tagkey == TAG_GENRE;
//  split = tagdefs [tagkey].vorbisMulti;

  if (split && strstr (val, ";") != NULL) {
    char      *tval;

    tval = mdstrdup (val);
    p = strtok_r (tval, ";", &tokstr);
    while (p != NULL) {
      while (*p == ' ') {
        ++p;
      }
      slistSetNum (vallist, p, 0);
      p = strtok_r (NULL, ";", &tokstr);
    }
  } else {
    slistSetNum (vallist, val, 0);
  }

  return vallist;
}

int
atioggCheckCodec (const char *ffn, int filetype)
{
  ogg_sync_state    oy_in;
  ogg_page          og_in;
  ogg_stream_state  os_in;
  ogg_packet        op_in;
  FILE              *ifh;
  int               rc = 0;
  int               orc;

  if ((ifh = fileopOpen (ffn, "rb")) == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "open input failed %s", ffn);
    return filetype;
  }

  filetype = AFILE_TYPE_UNKNOWN;

  (void) ogg_sync_init (&oy_in);
  while ((orc = ogg_sync_pageout (&oy_in, &og_in)) != 1) {
    char    *buf;
    size_t  s = 0;

    if ((buf = ogg_sync_buffer (&oy_in, BUFSIZ)) == NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "sync failed");
      rc = -1;
    }
    if (rc == 0 && (s = fread (buf, sizeof (char), BUFSIZ, ifh)) == 0) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "fread failed");
      rc = -1;
    }
    if (rc == 0 && ogg_sync_wrote (&oy_in, s) == -1) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "sync-wrote failed");
      rc = -1;
    }
  }

  if (rc == 0 && ogg_stream_init (&os_in, ogg_page_serialno (&og_in)) == -1) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "stream-init failed");
    rc = -1;
  }

  if (rc == 0 && ogg_stream_pagein (&os_in, &og_in) == -1) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "page-in failed");
    rc = -1;
  }

  if (rc == 0 && ogg_stream_packetout (&os_in, &op_in) > 0) {
    if (vorbis_synthesis_idheader (&op_in) == 1) {
      filetype = AFILE_TYPE_VORBIS;
    }
    if (opus_head_parse (NULL, op_in.packet, op_in.bytes) == 0) {
      filetype = AFILE_TYPE_OPUS;
    }
  }

  ogg_stream_clear (&os_in);
  ogg_sync_clear (&oy_in);

  mdextfclose (ifh);
  fclose (ifh);

  return filetype;
}

/* ogg file writing */

/* partially from tagutil: BSD 2-Clause License */
/* originally posted at : https://kaworu.ch/blog/2013/09/29/writting-ogg-slash-vorbis-comment-in-c/ */
/* 'struct vorbis_comment' and 'OpusTags' are identical per the opus documentation */
/* modified to handle opus files based on code in easytag */
int
atioggWriteOggFile (const char *ffn, void *tnewvc, int filetype)
{
  struct vorbis_comment   *newvc = tnewvc;
  FILE              *ifh  = NULL;
  char              outfn [MAXPATHLEN];
  int               rc = -1;
  FILE              *ofh = NULL;
  int               state;
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
  unsigned long     numheaderpackets = 0;

  if (filetype == AFILE_TYPE_VORBIS) {
    numheaderpackets = 3;
  }
  if (filetype == AFILE_TYPE_OPUS) {
    numheaderpackets = 2;
  }
  if (numheaderpackets == 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unsupported %s", ffn);
    return -1;
  }

  /*
   * Replace the 2nd ogg packet (vorbis comment) and copy the rest
   * See "Metadata workflow":
   * https://xiph.org/vorbis/doc/libvorbis/overview.html
   */

  state = VC_BUILD_PACKET;
  if (filetype == AFILE_TYPE_VORBIS) {
    if (vorbis_commentheader_out (newvc, &my_vc_packet) != 0) {
      goto cleanup_label;
    }
  }
  if (filetype == AFILE_TYPE_OPUS) {
    oggpack_buffer  b;
    const char      *vendor = "";

    if (newvc->vendor != NULL) {
      vendor = newvc->vendor;
    }

    oggpack_writeinit (&b);
    atibdj4_oggpack_string (&b, "OpusTags", 8);
    oggpack_write (&b, strlen (vendor), 32);
    atibdj4_oggpack_string (&b, vendor, strlen (vendor));
    oggpack_write (&b, newvc->comments, 32);
    for (int i = 0; i < newvc->comments; i++) {
      if (newvc->user_comments[i]) {
        oggpack_write (&b, newvc->comment_lengths[i], 32);
        atibdj4_oggpack_string (&b, newvc->user_comments[i], newvc->comment_lengths[i]);
      } else {
        oggpack_write (&b, 0, 32);
      }
    }
    my_vc_packet.bytes = oggpack_bytes (&b);
    my_vc_packet.packet = malloc (my_vc_packet.bytes);
    memcpy (my_vc_packet.packet, b.buffer, my_vc_packet.bytes);
    my_vc_packet.b_o_s = 0;
    my_vc_packet.e_o_s = 0;
    my_vc_packet.granulepos = 0;
    oggpack_writeclear (&b);
  }

  state = VC_SETUP;
  (void) ogg_sync_init (&oy_in);
  if ((ifh = fileopOpen (ffn, "rb")) == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "open input failed %s", ffn);
    goto cleanup_label;
  }
  if (filetype == AFILE_TYPE_VORBIS) {
    snprintf (outfn, sizeof (outfn), "%s.ogg.tmp", ffn);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    snprintf (outfn, sizeof (outfn), "%s.opus.tmp", ffn);
  }
  if ((ofh = fileopOpen (outfn, "wb")) == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "open output failed %s", ffn);
    goto cleanup_label;
  }

  lastblocksz = 0;
  granulepos = 0;
  nstream_in = 0;

bos_label:
  state = VC_BEG_OF_STREAM;
  nstream_in += 1;
  npage_in = 0;
  npacket_in = 0;
  vorbis_info_init (&vi_in);
  vorbis_comment_init (&vc_in);

  state = VC_START_READING;
  while (state != VC_END_OF_STREAM) {
    switch (ogg_sync_pageout (&oy_in, &og_in)) {
      case 0:
        /* more data needed or an internal error occurred. */
      case -1: {
        /* stream has not yet captured sync (bytes were skipped). */
        if (feof (ifh)) {
          if (state < VC_READING_DATA) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "eof on input");
            goto cleanup_label;
          }
          state = VC_END_OF_STREAM;
        } else {
          char *buf;
          size_t s;

          if ((buf = ogg_sync_buffer (&oy_in, BUFSIZ)) == NULL) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "sync failed");
            goto cleanup_label;
          }
          if ((s = fread (buf, sizeof (char), BUFSIZ, ifh)) == 0) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "fread failed");
            goto cleanup_label;
          }
          if (ogg_sync_wrote (&oy_in, s) == -1) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "sync-write failed");
            goto cleanup_label;
          }
        }
        continue;
      }
    }

    if (npage_in == 0) {
      /* init both input and output streams with the serialno
         of the first page */
      if (ogg_stream_init (&os_in, ogg_page_serialno (&og_in)) == -1) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "init input failed");
        goto cleanup_label;
      }
      if (ogg_stream_init (&os_out, ogg_page_serialno (&og_in)) == -1) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "init output failed");
        ogg_stream_clear (&os_in);
        goto cleanup_label;
      }
      state = VC_STREAMS_INITIALIZED;
    }

    if (ogg_stream_pagein (&os_in, &og_in) == -1) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "page-in failed");
      goto cleanup_label;
    }

    while (ogg_stream_packetout (&os_in, &op_in) == 1) {
      ogg_packet *target;

      if (npacket_in == 1 && nstream_in == 1) {
        target = &my_vc_packet;
      } else {
        target = &op_in;
      }

      if (npacket_in < numheaderpackets) {
        if (filetype == AFILE_TYPE_VORBIS) {
          if (vorbis_synthesis_headerin (&vi_in, &vc_in, &op_in) != 0) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "header-in failed");
            goto cleanup_label;
          }
        }
        state = (npacket_in == (numheaderpackets - 1) ? VC_READING_DATA_NEED_FLUSH : VC_READING_HEADERS);
      } else {
        if (state == VC_READING_DATA_NEED_FLUSH) {
          while (ogg_stream_flush (&os_out, &og_out)) {
            if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
              logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-page a failed");
              goto cleanup_label;
            }
          }
        } else if (state == VC_READING_DATA_NEED_PAGEOUT) {
          while (ogg_stream_pageout (&os_out, &og_out)) {
            if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
              logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-page b failed");
              goto cleanup_label;
            }
          }
        }

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
        if (filetype == AFILE_TYPE_VORBIS) {
          blocksz = vorbis_packet_blocksize (&vi_in, &op_in);
          granulepos += (lastblocksz == 0 ? 0 : (blocksz + lastblocksz) / 4);
          lastblocksz = blocksz;
        }
        if (filetype == AFILE_TYPE_OPUS) {
          granulepos += opus_packet_get_samples_per_frame (op_in.packet, 48000);
        }

        /*
         * Decide whether we need to write a page based
         * on the granulepos in the page.
         */
        state = VC_READING_DATA;
        if (op_in.granulepos == -1) {
          op_in.granulepos = granulepos;
        } else if (granulepos <= op_in.granulepos) {
          state = VC_READING_DATA_NEED_PAGEOUT;
        } else {
          granulepos = op_in.granulepos;
          state = VC_READING_DATA_NEED_FLUSH;
        }
      }

      if (ogg_stream_packetin (&os_out, target) == -1) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "packet-in failed");
        goto cleanup_label;
      }

      ++npacket_in;
    }
    if (ogg_page_eos (&og_in)) {
      state = VC_END_OF_STREAM;
    }
    ++npage_in;
  }

  /* forces remaining packets into a last page */
  os_out.e_o_s = 1;
  while (ogg_stream_flush (&os_out, &og_out)) {
    if (atibdj4WriteOggPage (&og_out, ofh) == -1) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-page failed");
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
    mdextfclose (ifh);
    fclose (ifh);
    ifh = NULL;
  }

  state = VC_WRITE_FINISH;
  if (ofh != stdout && ofh != NULL) {
    mdextfclose (ofh);
    if (fclose (ofh) != 0) {
      goto cleanup_label;
    }
  }
  ofh = NULL;
  state = VC_DONE_SUCCESS;

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
    mdextfclose (ofh);
    fclose (ofh);
  }
  if (ifh != NULL) {
    mdextfclose (ifh);
    fclose (ifh);
  }
  ogg_packet_clear (&my_vc_packet);

  if (state == VC_DONE_SUCCESS) {
    atiReplaceFile (ffn, outfn);
  } else {
    fileopDelete (outfn);
  }

  return rc;
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

static void
atibdj4_oggpack_string (oggpack_buffer *b, const char *s, size_t len)
{
  while (len) {
    oggpack_write (b, *s, 8);
    --len;
    ++s;
  }
}
