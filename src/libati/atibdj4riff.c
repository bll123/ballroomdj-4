/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>

#if __has_include (<netinet/in.h>)
# include <netinet/in.h>
#endif
#if __has_include (<winsock2.h>)
# include <winsock2.h>
#endif

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "fileop.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

enum {
  RIFF_ID_LEN = 4,
};

static const char * const RIFF_ID_RIFF = "RIFF";
static const char * const RIFF_ID_WAVE = "WAVE";
static const char * const RIFF_ID_FMT  = "fmt ";
static const char * const RIFF_ID_DATA = "data";
static const char * const RIFF_ID_LIST = "LIST";
static const char * const RIFF_ID_INFO = "INFO";

#pragma pack(push, 1)
typedef struct {
  uint16_t      audioformat;
  uint16_t      numchannels;
  uint32_t      samplerate;
  uint32_t      byterate;
  uint16_t      blockalign;
  uint16_t      bitspersample;
} wavefmt_t;
#pragma pack(pop)

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
} atisaved_t;

static int atibdj4RIFFReadObjectHead (FILE *fh, char *riffid, uint32_t *len);
static int atibdj4RIFFParseMetadata (FILE *fh, atidata_t *ati, slist_t *tagdata, int tagtype, uint32_t metalen);
static int atibdj4RIFFSkipPadding (FILE *fh, uint32_t len);

void
atibdj4LogRIFFVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "internal riff version %s", "1.0.0");
}

void
atibdj4ParseRIFFTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  FILE          *fh;
  char          riffid [RIFF_ID_LEN + 1];
  uint32_t      len;
  int           rc;
  wavefmt_t     fmt;    /* need this for duration calculation */


  fh = fileopOpen (ffn, "rb");
  if (fh == NULL) {
    return;
  }
  mdextfopen (fh);

  if (atibdj4RIFFReadObjectHead (fh, riffid, &len) < 0 ||
      strcmp (riffid, RIFF_ID_RIFF) != 0) {
    mdextfclose (fh);
    fclose (fh);
    return;
  }

  /* the WAVE marker is not a chunk */
  rc = fread (riffid, RIFF_ID_LEN, 1, fh);
  riffid [RIFF_ID_LEN] = '\0';
  if (rc != 1 || strcmp (riffid, RIFF_ID_WAVE) != 0) {
    mdextfclose (fh);
    fclose (fh);
    return;
  }

  while (atibdj4RIFFReadObjectHead (fh, riffid, &len) == 0) {
    bool    doseek = true;

    if (strcmp (riffid, RIFF_ID_FMT) == 0) {
      if (fread (&fmt, sizeof (wavefmt_t), 1, fh) != 1) {
        mdextfclose (fh);
        fclose (fh);
        return;
      }
      /* wavefmt_t is a packed structure */
      /* audioformat is at the beginning, so it is safe to use */
      if (fmt.audioformat == 1) {
        doseek = false;
      }
      if (fmt.audioformat != 1) {
        uint16_t    t16;

        if (fread (&t16, sizeof (uint16_t), 1, fh) != 1) {
          mdextfclose (fh);
          fclose (fh);
          return;
        }
        len = t16;
        /* and leave the doseek flag on */
      }
    }

    if (strcmp (riffid, RIFF_ID_DATA) == 0) {
      slistSetStr (tagdata, atidata->tagName (TAG_DURATION), "0");
      if (fmt.blockalign > 0 && fmt.samplerate > 0) {
        uint64_t  numsamples;
        uint64_t  dur;
        char      tmp [40];

        numsamples = len / (uint32_t) fmt.blockalign;
        dur = numsamples * 1000 / fmt.samplerate;
        snprintf (tmp, sizeof (tmp), "%" PRIu64, dur);
        slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);
      }
    }

    if (strcmp (riffid, RIFF_ID_LIST) == 0) {
      atibdj4RIFFParseMetadata (fh, atidata, tagdata, tagtype, len);
      doseek = false;
    }

    if (doseek) {
      if (fileopSeek (fh, len, SEEK_CUR) != 0) {
        break;
      }
    }
  }

  mdextfclose (fh);
  fclose (fh);

  return;
}

/* internal routines */

static int
atibdj4RIFFReadObjectHead (FILE *fh, char *riffid, uint32_t *len)
{
  if (fread (riffid, RIFF_ID_LEN, 1, fh) != 1) {
    return -1;
  }
  riffid [RIFF_ID_LEN] = '\0';
  if (fread (len, sizeof (uint32_t), 1, fh) != 1) {
    return -1;
  }

  return 0;
}

static int
atibdj4RIFFParseMetadata (FILE *fh, atidata_t *atidata, slist_t *tagdata,
    int tagtype, uint32_t metalen)
{
  char      riffid [RIFF_ID_LEN + 1];
  uint32_t  len;
  int       rc;
  uint32_t  processlen = 0;

  /* the INFO marker is not a chunk */
  rc = fread (riffid, RIFF_ID_LEN, 1, fh);
  riffid [RIFF_ID_LEN] = '\0';
  if (rc != 1 || strcmp (riffid, RIFF_ID_INFO) != 0) {
    return -1;
  }
  processlen += RIFF_ID_LEN;

  while (processlen < metalen &&
      atibdj4RIFFReadObjectHead (fh, riffid, &len) == 0) {
    const char    *tagname;
    char          *data;
    const char    *p;
    char          pbuff [100];

    tagname = atidata->tagLookup (tagtype, riffid);
    if (tagname == NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: unk %s", riffid);
      if (fileopSeek (fh, len, SEEK_CUR) != 0) {
        break;
      }
      processlen += RIFF_ID_LEN + sizeof (uint32_t) + len;
      processlen += atibdj4RIFFSkipPadding (fh, len);
      continue;
    }

    data = mdmalloc (len + 1);
    if (fread (data, len, 1, fh) != 1) {
      return -1;
    }
    data [len] = '\0';

    p = data;
    if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
      p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
          p, pbuff, sizeof (pbuff));
    }
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, riffid, data);
    slistSetStr (tagdata, tagname, p);
    mdfree (data);
    processlen += RIFF_ID_LEN + sizeof (uint32_t) + len;
    processlen += atibdj4RIFFSkipPadding (fh, len);
  }

  return 0;
}

static int
atibdj4RIFFSkipPadding (FILE *fh, uint32_t len)
{
  int     rem;
  char    pbuff [40];

  /* ffmpeg et.al. write the length without including the padding bytes */
  /* in the length.  padding is to the int16 boundary */
  rem = len % sizeof (int16_t);
  if (rem) {
    (void) ! fread (pbuff, rem, 1, fh);
  }

  return rem;
}
