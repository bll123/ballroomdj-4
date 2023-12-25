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
#include <assert.h>

#if _hdr_netinet_in
# include <netinet/in.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif

#include "ati.h"
#include "atibdj4.h"
#include "atibdj4asf.h"
#include "audiofile.h"
#include "fileop.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atisaved {
  bool                  hasdata;
  int                   filetype;
  int                   tagtype;
} atisaved_t;

static int atibdj4ASFReadObjectHead (FILE *fh, int *objtype, size_t *len);
static int atibdj4ASFProcessData (FILE *fh, atidata_t *atidata, slist_t *tagdata, int tagtype, int numobj, int level);
static int atibdj4ASFProcessMetaData (FILE *fh, atidata_t *atidata, slist_t *tagdata, int tagtype, int count);
static int atibdj4ASFProcessExtContentData (FILE *fh, atidata_t *atidata, slist_t *tagdata, int tagtype, int count);
static int atibdj4ASFProcessContentData (FILE *fh, atidata_t *atidata, slist_t *tagdata, int tagtype, asfcontent_t *content);
static int atibdj4ASFReadHeader (FILE *fh, asfheader_t *header);
static int atibdj4ASFReadData (FILE *fh, void *data, size_t dlen);
static void atibdj4ASFSkipData (FILE *fh, size_t len);
static void atibdj4ASFProcessTag (FILE *fh, atidata_t *atidata, slist_t *tagdata, int tagtype, const char *tagname, const char *nm, int datatype, int datalen);
static int atibdj4ASFCheckGUID (const char *buff);
static uint64_t atibdj4ASFle64toh (uint64_t val);
static uint32_t atibdj4ASFle32toh (uint32_t val);
static uint16_t atibdj4ASFle16toh (uint16_t val);

void
atibdj4LogASFVersion (void)
{
  logMsg (LOG_DBG, LOG_INFO, "internal asf version %s", "1.0.0");
}

void
atibdj4ParseASFTags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  FILE          *fh;
  asfheader_t   header;
  int           objtype;
  size_t        len;

  fh = fileopOpen (ffn, "rb");
  if (fh == NULL) {
    return;
  }
  mdextfopen (fh);

  atibdj4ASFReadObjectHead (fh, &objtype, &len);
  if (objtype != ASF_GUID_HEADER) {
    mdextfclose (fh);
    fclose (fh);
    return;
  }

  atibdj4ASFReadHeader (fh, &header);
  /* header is a packed structure */
  /* header.numobj is the first item in the structure so it will be */
  /* aligned properly */
  atibdj4ASFProcessData (fh, atidata, tagdata, tagtype, header.numobj, 0);

  mdextfclose (fh);
  fclose (fh);

  return;
}

int
atibdj4WriteASFTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  return -1;
}

atisaved_t *
atibdj4SaveASFTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return NULL;
}

void
atibdj4FreeSavedASFTags (atisaved_t *atisaved, int tagtype, int filetype)
{
  return;
}

int
atibdj4RestoreASFTags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  return -1;
}

void
atibdj4CleanASFTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  return;
}

/* internal routines */

static int
atibdj4ASFReadObjectHead (FILE *fh, int *objtype, size_t *len)
{
  char      gbuff [ASF_GUID_LEN];
  uint64_t  t64;

  *objtype = ASF_GUID_OTHER;
  *len = 0;

  if (fread (gbuff, ASF_GUID_LEN, 1, fh) != 1) {
    return -1;
  }
  *objtype = atibdj4ASFCheckGUID (gbuff);
  if (fread (&t64, sizeof (uint64_t), 1, fh) != 1) {
    return -1;
  }
  *len = atibdj4ASFle64toh (t64);
  return 0;
}

static int
atibdj4ASFProcessData (FILE *fh, atidata_t *atidata, slist_t *tagdata,
    int tagtype, int numobj, int level)
{
  int         objtype;
  size_t      len;
  int         count = 0;
  bool        done = false;
  int         rc = 0;

  while (! done) {
    if (atibdj4ASFReadObjectHead (fh, &objtype, &len) < 0) {
      rc = -1;
      break;
    }
    if (len == 0) {
      /* there's junk after the end of the wma data */
      rc = -1;
      break;
    }
    len -= ASF_GUID_LEN;
    len -= sizeof (uint64_t);

    switch (objtype) {
      case ASF_GUID_FILE_PROP: {
        asffileprop_t   fileprop;
        uint64_t        dur;
        char            tmp [40];

        atibdj4ASFReadData (fh, &fileprop, len);
        /* the specs claim to be in nano seconds, but something is off */
        dur = atibdj4ASFle64toh (fileprop.duration) / 10000;
        dur -= atibdj4ASFle64toh (fileprop.preroll);
        snprintf (tmp, sizeof (tmp), "%" PRId64, dur);
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  duration: %s", tmp);
        slistSetStr (tagdata, atidata->tagName (TAG_DURATION), tmp);
        break;
      }
      case ASF_GUID_HEADER_EXT: {
        asfheaderext_t  headerext;

        atibdj4ASFReadData (fh, &headerext, sizeof (headerext));
        if (atibdj4ASFProcessData (fh, atidata, tagdata, tagtype, 0, level + 1) < 0) {
          /* do not want to do any more processing */
          rc = -1;
          done = true;
        }
        break;
      }
      case ASF_GUID_METADATA: {
        asfmetadata_t   metadata;

        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        metadata.count = atibdj4ASFle16toh (metadata.count);
        atibdj4ASFProcessMetaData (fh, atidata, tagdata, tagtype, metadata.count);
        break;
      }
      case ASF_GUID_METADATA_LIBRARY: {
        asfmetadata_t   metadata;

        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        metadata.count = atibdj4ASFle16toh (metadata.count);
        atibdj4ASFProcessMetaData (fh, atidata, tagdata, tagtype, metadata.count);
        break;
      }
      case ASF_GUID_EXT_CONTENT_DESC: {
        asfmetadata_t   metadata;

        /* the extended content description also has a 16-bit count */
        /* just re-usethe metadata structure */
        /* the data-types are the same, but the description */
        /* record is different */
        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        metadata.count = atibdj4ASFle16toh (metadata.count);
        atibdj4ASFProcessExtContentData (fh, atidata, tagdata, tagtype, metadata.count);
        break;
      }
      case ASF_GUID_CONTENT_DESC: {
        asfcontent_t    content;

        atibdj4ASFReadData (fh, &content, sizeof (content));
        atibdj4ASFProcessContentData (fh, atidata, tagdata, tagtype, &content);
        break;
      }
      default: {
        atibdj4ASFSkipData (fh, len);
        break;
      }
    }

    if (numobj > 0) {
      ++count;
      if (count >= numobj) {
        done = true;
      }
    }
  }

  return rc;
}

static int
atibdj4ASFProcessMetaData (FILE *fh, atidata_t *atidata, slist_t *tagdata,
    int tagtype, int count)
{
  wchar_t     *wnm;
  char        *nm = NULL;
  const char  *tagname;

  for (int i = 0; i < count; ++i) {
    asfmetadatadesc_t   metadatadesc;
    int32_t             tlen;

    if (fread (&metadatadesc, sizeof (metadatadesc), 1, fh) != 1) {
      break;
    }
    tlen = atibdj4ASFle16toh (metadatadesc.nmlen);
    wnm = mdmalloc (tlen);
    if (fread (wnm, tlen, 1, fh) != 1) {
      break;
    }
    dataFree (nm);
    nm = istring16ToUTF8 (wnm);
    tagname = atidata->tagLookup (tagtype, nm);
    mdfree (wnm);

    metadatadesc.datatype = atibdj4ASFle16toh (metadatadesc.datatype);
    tlen = atibdj4ASFle32toh (metadatadesc.datalen);
    atibdj4ASFProcessTag (fh, atidata, tagdata, tagtype, tagname, nm, metadatadesc.datatype, tlen);
  }

  dataFree (nm);

  return 0;
}

static int
atibdj4ASFProcessExtContentData (FILE *fh, atidata_t *atidata,
    slist_t *tagdata, int tagtype, int count)
{
  wchar_t     *wnm;
  char        *nm = NULL;
  const char  *tagname;

  for (int i = 0; i < count; ++i) {
    asfcontentnm_t      contentnm;
    asfcontentdata_t    contentdata;
    int32_t             tlen;

    if (fread (&contentnm, sizeof (contentnm), 1, fh) != 1) {
      break;
    }
    tlen = atibdj4ASFle16toh (contentnm.nmlen);
    wnm = mdmalloc (tlen);
    if (fread (wnm, tlen, 1, fh) != 1) {
      mdfree (wnm);
      break;
    }
    dataFree (nm);
    nm = istring16ToUTF8 (wnm);
    tagname = atidata->tagLookup (tagtype, nm);
    mdfree (wnm);

    if (fread (&contentdata, sizeof (contentdata), 1, fh) != 1) {
      break;
    }

    contentdata.datatype = atibdj4ASFle16toh (contentdata.datatype);
    if (contentdata.datatype == ASF_DATA_BOOL16) {
      contentdata.datatype = ASF_DATA_BOOL32;
    }
    tlen = atibdj4ASFle16toh (contentdata.datalen);
    atibdj4ASFProcessTag (fh, atidata, tagdata, tagtype, tagname, nm, contentdata.datatype, tlen);
  }

  return 0;
}

static int
atibdj4ASFProcessContentData (FILE *fh, atidata_t *atidata, slist_t *tagdata,
    int tagtype, asfcontent_t *content)
{
  wchar_t     *wdata;
  char        *data;
  const char  *tagname;

  for (int i = 0; i < 5; ++i) {
    if (content->len [i] == 0) {
      continue;
    }

    wdata = mdmalloc (content->len [i]);
    if (fread (wdata, content->len [i], 1, fh) != 1) {
      break;
    }
    tagname = atidata->tagLookup (tagtype, asf_content_names [i]);
    if (tagname == NULL) {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG,"  raw: unk %s", asf_content_names [i]);
    }

    if (tagname != NULL) {
      data = istring16ToUTF8 (wdata);
      /* only use these if the tag is not already set */
      /* assumption being that whatever is in the metadata block is better */
      if (slistGetStr (tagdata, tagname) == NULL) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG,"  raw: %s %s=%s", tagname, asf_content_names [i], data);
        slistSetStr (tagdata, tagname, data);
      }
      mdfree (data);
    }
    mdfree (wdata);
  }

  return 0;
}

static int
atibdj4ASFReadHeader (FILE *fh, asfheader_t *header)
{
  if (fread (header, sizeof (asfheader_t), 1, fh) != 1) {
    return -1;
  }
  header->numobj = atibdj4ASFle32toh (header->numobj);

  return 0;
}

static int
atibdj4ASFReadData (FILE *fh, void *data, size_t dlen)
{
  if (fread (data, dlen, 1, fh) != 1) {
    return -1;
  }

  return 0;
}

static void
atibdj4ASFSkipData (FILE *fh, size_t len)
{
  fseek (fh, len, SEEK_CUR);
}


static void
atibdj4ASFProcessTag (FILE *fh, atidata_t *atidata, slist_t *tagdata,
    int tagtype, const char *tagname, const char *nm,
    int datatype, int datalen)
{
  char      tmp [40];

  switch (datatype) {
    case ASF_DATA_UTF8: {
      wchar_t     *wbuff;
      char        *buff;

      wbuff = mdmalloc (datalen);
      if (fread (wbuff, datalen, 1, fh) != 1) {
        mdfree (wbuff);
        break;
      }
      buff = istring16ToUTF8 (wbuff);
      mdfree (wbuff);

      if (tagname != NULL) {
        const char    *p;
        char          pbuff [100];

        p = buff;
        if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
          p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
              p, pbuff, sizeof (pbuff));
        }
        if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
          p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
              p, pbuff, sizeof (pbuff));
        }
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, nm, buff);
        slistSetStr (tagdata, tagname, p);
      }
      mdfree (buff);
      break;
    }
    case ASF_DATA_BIN: {
      wchar_t   *buff;

      buff = mdmalloc (datalen);
      if (fread (buff, datalen, 1, fh) != 1) {
        break;
      }
      mdfree (buff);
      break;
    }
    case ASF_DATA_GUID: {
      wchar_t   *buff;

      buff = mdmalloc (datalen);
      if (fread (buff, datalen, 1, fh) != 1) {
        break;
      }
      mdfree (buff);
      break;
    }
    case ASF_DATA_BOOL16:
    case ASF_DATA_U16: {
      uint16_t    t16;

      if (fread (&t16, sizeof (uint16_t), 1, fh) != 1) {
        break;
      }
      if (tagname != NULL) {
        snprintf (tmp, sizeof (tmp), "%d", atibdj4ASFle16toh (t16));
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, nm, tmp);
        slistSetStr (tagdata, tagname, tmp);
      }
      break;
    }
    case ASF_DATA_BOOL32:
    case ASF_DATA_U32: {
      uint32_t    t32;

      if (fread (&t32, sizeof (uint32_t), 1, fh) != 1) {
        break;
      }
      if (tagname != NULL) {
        snprintf (tmp, sizeof (tmp), "%d", atibdj4ASFle32toh (t32));
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, nm, tmp);
        slistSetStr (tagdata, tagname, tmp);
      }
      break;
    }
    case ASF_DATA_U64: {
      uint64_t    t64;

      if (fread (&t64, sizeof (uint64_t), 1, fh) != 1) {
        break;
      }
      if (tagname != NULL) {
        snprintf (tmp, sizeof (tmp), "%" PRId64, atibdj4ASFle64toh (t64));
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw: %s %s=%s", tagname, nm, tmp);
        slistSetStr (tagdata, tagname, tmp);
      }
      break;
    }
  }
}

static int
atibdj4ASFCheckGUID (const char *buff)
{
  guid_t  tuuid;

  /* this is not very efficient, but saves a lot of trouble */
  for (int i = 0; i < ASF_GUID_MAX; ++i) {
    int           tmp;
    const char    *dptr;
    unsigned char *uptr;

    /* libuuid/uuid_parse returns the uuid in big-endian form */
    /* not useful here */

    dptr = asf_guids [i].guid;
    uptr = (unsigned char *) &tuuid;
    for (int j = 0; j < 16; ++j) {
      sscanf (dptr, "%2x", &tmp);
      uptr [j] = (uint8_t) tmp;
      dptr += 2;
      if (*dptr == '-') {
        ++dptr;
      }
    }

    tuuid.a = ntohl (tuuid.a);
    tuuid.b = ntohs (tuuid.b);
    tuuid.c = ntohs (tuuid.c);

    if (memcmp (buff, &tuuid, ASF_GUID_LEN) == 0) {
      return i;
    }
  }

  return ASF_GUID_OTHER;
}

static uint64_t
atibdj4ASFle64toh (uint64_t val)
{
#if BYTE_ORDER == LITTLE_ENDIAN
  return val;
#else
  return (((uint64_t) htonl((uint32_t) (v & 0xFFFFFFFFUL))) << 32) |
      (uint64_t) htonl ((uint32_t) (v >> 32));
#endif
}

static uint32_t
atibdj4ASFle32toh (uint32_t val)
{
  uint32_t res;

#if BYTE_ORDER == LITTLE_ENDIAN
  res = val;
#else
  uint32_t b0,b1,b2,b3;

  b0 = (val & 0x000000ff) << 24u;
  b1 = (val & 0x0000ff00) << 8u;
  b2 = (val & 0x00ff0000) >> 8u;
  b3 = (val & 0xff000000) >> 24u;

  res = b0 | b1 | b2 | b3;
#endif
  return res;
}

static uint16_t
atibdj4ASFle16toh (uint16_t val)
{
  uint16_t res;

#if __BYTE_ORDER == __LITTLE_ENDIAN
  res = val;
#else
  uint16_t b0,b1;

  b0 = (val & 0x00ff) << 8u;
  b1 = (val & 0xff00) >> 8u;

  res = b0 | b1;
#endif

  return res;
}

