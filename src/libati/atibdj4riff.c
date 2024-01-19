/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#if _hdr_netinet_in
# include <netinet/in.h>
#endif
#if _hdr_winsock2
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

#define RIFF_ID_RIFF "RIFF"
#define RIFF_ID_WAVE "WAVE"
#define RIFF_ID_FMT  "fmt "
#define RIFF_ID_LIST "LIST"
#define RIFF_ID_INFO "INFO"

typedef struct {
  uint16_t      audioformat;
  uint16_t      numchannels;
  uint32_t      samplerate;
  uint32_t      byterate;
  uint16_t      blockalign;
  uint16_t      bitspersample;
} __attribute__((packed)) wavefmt_t;

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
} atisaved_t;

static int atibdj4RIFFReadObjectHead (FILE *fh, char *riffid, uint32_t *len);
static int atibdj4RIFFParseMetadata (FILE *fh, atidata_t *ati, slist_t *tagdata, int tagtype);

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
      wavefmt_t     fmt;

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

    if (strcmp (riffid, RIFF_ID_LIST) == 0) {
      atibdj4RIFFParseMetadata (fh, atidata, tagdata, tagtype);
      doseek = false;
    }

    if (doseek) {
      if (fseek (fh, len, SEEK_CUR) != 0) {
        break;
      }
    }
  }

  mdextfclose (fh);
  fclose (fh);

  return;
}

int
atibdj4WriteRIFFTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

atisaved_t *
atibdj4SaveRIFFTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return NULL;
}

void
atibdj4FreeSavedRIFFTags (atisaved_t *atisaved, int tagtype, int filetype)
{
  return;
}

int
atibdj4RestoreRIFFTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  return -1;
}

void
atibdj4CleanRIFFTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
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
    int tagtype)
{
  char      riffid [RIFF_ID_LEN + 1];
  uint32_t  len;
  int       rc;

  /* the INFO marker is not a chunk */
  rc = fread (riffid, RIFF_ID_LEN, 1, fh);
  riffid [RIFF_ID_LEN] = '\0';
  if (rc != 1 || strcmp (riffid, RIFF_ID_INFO) != 0) {
    return -1;
  }

  while (atibdj4RIFFReadObjectHead (fh, riffid, &len) == 0) {
    const char    *tagname;
    char          *data;
    const char    *p;
    char          pbuff [100];

    tagname = atidata->tagLookup (tagtype, riffid);
    if (tagname == NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: unk %s", riffid);
      if (fseek (fh, len, SEEK_CUR) != 0) {
        break;
      }
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
  }

  return 0;
}
