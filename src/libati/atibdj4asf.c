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
#include <wchar.h>

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
static int atibdj4ASFProcessData (FILE *fh, int numobj, int level);
static int atibdj4ASFProcessMetaData (FILE *fh, int count);
static int atibdj4ASFProcessExtContentData (FILE *fh, int count);
static int atibdj4ASFProcessContentData (FILE *fh, asfcontent_t *content);
static int atibdj4ASFReadHeader (FILE *fh, asfheader_t *header);
static int atibdj4ASFReadData (FILE *fh, void *data, size_t dlen);
static void atibdj4ASFSkipData (FILE *fh, size_t len);
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

  atibdj4ASFProcessData (fh, header.numobj, 0);

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
atibdj4ASFProcessData (FILE *fh, int numobj, int level)
{
  int         objtype;
  size_t      len;
  int         count = 0;
  bool        done = false;

  while (! done) {
    if (atibdj4ASFReadObjectHead (fh, &objtype, &len) < 0) {
      break;
    }
    len -= ASF_GUID_LEN;
    len -= sizeof (uint64_t);

    switch (objtype) {
      case ASF_GUID_FILE_PROP: {
        asffileprop_t   fileprop;
        uint64_t        dur;

        atibdj4ASFReadData (fh, &fileprop, len);
        /* the specs claim to be in nano seconds, but something is off */
        dur = fileprop.duration / 10000;
        dur -= fileprop.preroll;
fprintf (stdout, "duration=%" PRIu64 "\n", dur);
        break;
      }
      case ASF_GUID_HEADER_EXT: {
        asfheaderext_t  headerext;

        atibdj4ASFReadData (fh, &headerext, sizeof (headerext));
        atibdj4ASFProcessData (fh, 0, level + 1);
        break;
      }
      case ASF_GUID_METADATA: {
        asfmetadata_t   metadata;

        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        atibdj4ASFProcessMetaData (fh, metadata.count);
        break;
      }
      case ASF_GUID_METADATA_LIBRARY: {
        asfmetadata_t   metadata;

        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        atibdj4ASFProcessMetaData (fh, metadata.count);
        break;
      }
      case ASF_GUID_EXT_CONTENT_DESC: {
        asfmetadata_t   metadata;

        /* the extended content description also has a 16-bit count */
        /* just re-used the metadata structure */
        /* the data-types are the same, but the description */
        /* record is different */
        atibdj4ASFReadData (fh, &metadata, sizeof (metadata));
        atibdj4ASFProcessExtContentData (fh, metadata.count);
        break;
      }
      case ASF_GUID_CONTENT_DESC: {
        asfcontent_t    content;

        atibdj4ASFReadData (fh, &content, sizeof (content));
        atibdj4ASFProcessContentData (fh, &content);
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

  return 0;
}

static int
atibdj4ASFProcessMetaData (FILE *fh, int count)
{
  wchar_t     *wnm;
  char        *nm;

  for (int i = 0; i < count; ++i) {
    asfmetadatadesc_t   metadatadesc;
    int32_t             tlen;

    if (fread (&metadatadesc, sizeof (metadatadesc), 1, fh) != 1) {
      break;
    }
    tlen = metadatadesc.nmlen;
    wnm = mdmalloc (tlen);
    if (fread (wnm, tlen, 1, fh) != 1) {
      break;
    }
    nm = istring16ToUTF8 (wnm);
fprintf (stdout, "%s=", nm);
    mdfree (wnm);
    mdfree (nm);
    switch (metadatadesc.datatype) {
      case ASF_DATA_UTF8: {
        wchar_t   *wbuff;
        char      *buff;

        tlen = metadatadesc.datalen;
        wbuff = mdmalloc (tlen);
        if (fread (wbuff, tlen, 1, fh) != 1) {
          break;
        }
        buff = istring16ToUTF8 (wbuff);
fprintf (stdout, "%s\n", buff);
        mdfree (wbuff);
        mdfree (buff);
        break;
      }
      case ASF_DATA_BIN: {
        wchar_t   *buff;

        tlen = metadatadesc.datalen;
        buff = mdmalloc (tlen);
        if (fread (buff, tlen, 1, fh) != 1) {
          break;
        }
fprintf (stdout, "(binary data: %d)\n", tlen);
        mdfree (buff);
        break;
      }
      case ASF_DATA_BOOL:
      case ASF_DATA_U16: {
        uint16_t    t16;

        if (fread (&t16, sizeof (uint16_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%d\n", t16);
        break;
      }
      case ASF_DATA_U32: {
        uint32_t    t32;

        if (fread (&t32, sizeof (uint32_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%d\n", t32);
        break;
      }
      case ASF_DATA_U64: {
        uint64_t    t64;

        if (fread (&t64, sizeof (uint64_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%ld\n", t64);
        break;
      }
    }
  }

  return 0;
}

static int
atibdj4ASFProcessExtContentData (FILE *fh, int count)
{
  wchar_t     *wnm;
  char        *nm;

  for (int i = 0; i < count; ++i) {
    asfcontentnm_t      contentnm;
    asfcontentdata_t    contentdata;
    int32_t             tlen;

    if (fread (&contentnm, sizeof (contentnm), 1, fh) != 1) {
      break;
    }
    tlen = contentnm.nmlen;
    wnm = mdmalloc (tlen);
    if (fread (wnm, tlen, 1, fh) != 1) {
      break;
    }
    nm = istring16ToUTF8 (wnm);
fprintf (stdout, "%s=", nm);
    mdfree (wnm);
    mdfree (nm);

    if (fread (&contentdata, sizeof (contentdata), 1, fh) != 1) {
      break;
    }
    switch (contentdata.datatype) {
      case ASF_DATA_UTF8: {
        wchar_t   *wbuff;
        char      *buff;

        tlen = contentdata.datalen;
        wbuff = mdmalloc (tlen);
        if (fread (wbuff, tlen, 1, fh) != 1) {
          break;
        }
        buff = istring16ToUTF8 (wbuff);
fprintf (stdout, "%s\n", buff);
        mdfree (wbuff);
        mdfree (buff);
        break;
      }
      case ASF_DATA_BIN: {
        wchar_t   *buff;

        tlen = contentdata.datalen;
        buff = mdmalloc (tlen);
        if (fread (buff, tlen, 1, fh) != 1) {
          break;
        }
fprintf (stdout, "(binary data: %d)\n", tlen);
        mdfree (buff);
        break;
      }
      case ASF_DATA_U16: {
        uint16_t    t16;

        if (fread (&t16, sizeof (uint16_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%d\n", t16);
        break;
      }
      case ASF_DATA_BOOL:
      case ASF_DATA_U32: {
        uint32_t    t32;

        if (fread (&t32, sizeof (uint32_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%d\n", t32);
        break;
      }
      case ASF_DATA_U64: {
        uint64_t    t64;

        if (fread (&t64, sizeof (uint64_t), 1, fh) != 1) {
          break;
        }
fprintf (stdout, "%ld\n", t64);
        break;
      }
    }
  }

  return 0;
}

static int
atibdj4ASFProcessContentData (FILE *fh, asfcontent_t *content)
{
  wchar_t     *wdata;
  char        *data;

  for (int i = 0; i < 5; ++i) {
    wdata = mdmalloc (content->len [i]);
    if (fread (wdata, content->len [i], 1, fh) != 1) {
      break;
    }
    data = istring16ToUTF8 (wdata);
fprintf (stdout, "%s=%s\n", asf_content_names [i], data);
    mdfree (wdata);
    mdfree (data);
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

