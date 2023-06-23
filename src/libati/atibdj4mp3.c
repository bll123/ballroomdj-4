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

#include <id3tag.h>

#include "ati.h"
#include "atibdj4.h"
#include "audiofile.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"

typedef struct atisaved {
  bool            hasdata;
  int             filetype;
  int             tagtype;
  struct id3_tag  *id3tags;
} atisaved_t;

static void atibdj4AddMP3Tag (atidata_t *atidata, nlist_t *datalist, struct id3_tag *id3tags, const char *tag, const char *val, int tagtype);
static const char * atibdj4GetMP3TagName (atidata_t *atidata, struct id3_frame *id3frame, int tagtype);

void
atibdj4LogMP3Version (void)
{
  logMsg (LOG_DBG, LOG_INFO, "libid3tag version %s", ID3_VERSION);
}

void
atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tags;
  struct id3_frame  *id3frame;
  int               idx;
  char              pbuff [100];

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READONLY);
  if (id3file == NULL) {
    return;
  }
  id3tags = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    const char                  *ufid;
    const char                  *tagname;

    ufid = NULL;
    tagname = atibdj4GetMP3TagName (atidata, id3frame, tagtype);

    if (tagname == NULL) {
      /* this is not a tag that bdj4 uses */
      ++idx;
      continue;
    }

    /* ufid: field 0 is the name, field 1 is the data */
    /* txxx: field 0 is the text encoding, field 1 is the name, field 2 is the data */
    /* t---: field 0 is the text encoding, field 1 is the data */
    for (size_t i = 0; i < id3frame->nfields; ++i) {
      const union id3_field *field;

      field = &id3frame->fields [i];

      switch (field->type) {
        case ID3_FIELD_TYPE_TEXTENCODING: {
          break;
        }
        case ID3_FIELD_TYPE_FRAMEID: {
          break;
        }
        case ID3_FIELD_TYPE_LATIN1: {
          id3_latin1_t const  *str;

          str = id3_field_getlatin1 (field);
          if (strcmp (id3frame->id, "UFID") == 0) {
            ufid = (const char *) str;
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (1): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (2): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1LIST: {
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (9): ERR: %s not handled", tagname);
          break;
        }
        case ID3_FIELD_TYPE_STRING: {
          id3_ucs4_t const *ustr = NULL;
          id3_utf8_t        *str = NULL;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          mdextalloc (str);
          if (i != 1 || strcmp (id3frame->id, "TXXX") != 0) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (3): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          dataFree (str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr = NULL;
          id3_utf8_t        *str = NULL;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          mdextalloc (str);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (4): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          dataFree (str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr = NULL;
            id3_utf8_t        *str = NULL;
            const char        *p = NULL;

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
            mdextalloc (str);
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (5): %s %s=%s", tagname, id3frame->id, str);
            p = (const char *) str;
            if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
// ### need to handle multiple items; each should have a different language
// or somesuch. does this need to be checked?  I need an example file.
            slistSetStr (tagdata, tagname, p);
            dataFree (str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LANGUAGE: {
          break;
        }
        case ID3_FIELD_TYPE_DATE: {
          id3_latin1_t const *str;

          str = id3_field_getlatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (6): %s %s=%s", tagname, id3frame->id, str);
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
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (7): %s %s=%s", tagname, id3frame->id, tmp);
          slistSetStr (tagdata, tagname, tmp);
          break;
        }
        case ID3_FIELD_TYPE_BINARYDATA: {
          id3_byte_t const  *bstr;
          id3_length_t      blen;
          char              *tmp;

          bstr = id3_field_getbinarydata (field, &blen);
          tmp = mdmalloc (blen + 1);
          memcpy (tmp, bstr, blen);

          if (ufid != NULL && strcmp ((const char *) ufid, MB_TAG) == 0) {
            tmp [blen] = '\0';
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (8): %s %s=%s", tagname, id3frame->id, tmp);
            /* the musicbrainz recording id is not binary */
            slistSetStr (tagdata, tagname, tmp);
          } else {
            slistSetData (tagdata, tagname, tmp);
          }
          mdfree (tmp);
          break;
        }
      }
    }

    ++idx;
  }

  id3_file_close (id3file);
}

int
atibdj4WriteMP3Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  struct id3_file     *id3file;
  struct id3_tag      *id3tags = NULL;
  struct id3_frame    *id3frame;
  int                 idx;
  slistidx_t          iteridx;
  const char          *key;
  const char          *tagname;

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  if (id3file == NULL) {
    return -1;
  }
  id3tags = id3_file_tag (id3file);
  id3_tag_options (id3tags, ID3_TAG_OPTION_COMPRESSION, 0);
  /* turning off crc turns off the extended header */
  id3_tag_options (id3tags, ID3_TAG_OPTION_CRC, 0);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    tagname = atibdj4GetMP3TagName (atidata, id3frame, tagtype);
    if (tagname == NULL) {
      /* this is not a tag that bdj4 uses */
      ++idx;
      continue;
    }

    if (slistGetStr (dellist, tagname) != NULL) {
      id3_tag_detachframe (id3tags, id3frame);
      id3_frame_delete (id3frame);
      /* when the frame is detached, */
      /* the index does not need to be incremented */
      /* as the next frame drops down to the current position */
      continue;
    }
    ++idx;
  }

  slistStartIterator (updatelist, &iteridx);
  while ((key = slistIterateKey (updatelist, &iteridx)) != NULL) {
    atibdj4AddMP3Tag (atidata, datalist, id3tags, key, slistGetStr (updatelist, key), tagtype);
  }

  if (id3_file_update (id3file) != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  file update failed %s", ffn);
  }
  id3_file_close (id3file);

  return -1;
}

atisaved_t *
atibdj4SaveMP3Tags (atidata_t *atidata, const char *ffn,
    int tagtype, int filetype)
{
  atisaved_t        *atisaved = NULL;
  struct id3_file   *id3file;
  struct id3_tag    *id3tags;
  struct id3_tag    *nid3tags;
  struct id3_frame  *id3frame;
  int               idx;

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READONLY);
  if (id3file == NULL) {
    return atisaved;
  }
  id3tags = id3_file_tag (id3file);
  nid3tags = id3_tag_new ();

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    id3_tag_detachframe (id3tags, id3frame);
    id3_tag_attachframe (nid3tags, id3frame);
    /* when the frame is detached, */
    /* the index does not need to be incremented */
    /* as the next frame drops down to the current position */
  }

  atisaved = mdmalloc (sizeof (atisaved_t));
  atisaved->hasdata = true;
  atisaved->tagtype = tagtype;
  atisaved->filetype = filetype;
  atisaved->id3tags = nid3tags;

  id3_file_close (id3file);

  return atisaved;
}

void
atibdj4RestoreMP3Tags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tags;
  struct id3_frame  *id3frame;
  int               idx;

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

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  if (id3file == NULL) {
    return;
  }
  id3tags = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    id3_tag_detachframe (id3tags, id3frame);
    id3_frame_delete (id3frame);
    /* when the frame is detached, */
    /* the index does not need to be incremented */
    /* as the next frame drops down to the current position */
  }

  idx = 0;
  while ((id3frame = id3_tag_findframe (atisaved->id3tags, "", idx)) != NULL) {
    id3_tag_detachframe (atisaved->id3tags, id3frame);
    id3_tag_attachframe (id3tags, id3frame);
  }

  if (id3_file_update (id3file) != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  file update failed %s", ffn);
  }
  id3_file_close (id3file);
  atisaved->hasdata = false;
  id3_tag_delete (atisaved->id3tags);
  mdfree (atisaved);

  return;
}

void
atibdj4CleanMP3Tags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tags;
  struct id3_frame  *id3frame;
  int               idx;

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  if (id3file == NULL) {
    return;
  }
  id3tags = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    id3_tag_detachframe (id3tags, id3frame);
    id3_frame_delete (id3frame);
  }

  if (id3_file_update (id3file) != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  file update failed %s", ffn);
  }
  id3_file_close (id3file);

  return;
}

/* internal routines */

/* bdj4 only uses string tags and ufid (binary) */
/* support for the other types is not needed here */
static void
atibdj4AddMP3Tag (atidata_t *atidata, nlist_t *datalist,
    struct id3_tag *id3tags, const char *tag, const char *val, int tagtype)
{
  int                 writetags;
  int                 tagkey;
  const tagaudiotag_t *audiotag;
  id3_ucs4_t          *id3val;
  struct id3_frame    *frame = NULL;
  int                 idx;
  id3_ucs4_t          *ttag = NULL;
  const char          *id3tagname = NULL;
  bool                found = false;
  char                tbuff [200];

  writetags = atidata->writetags;
  tagkey = atidata->tagCheck (writetags, tagtype, tag, AF_REWRITE_NONE);
  audiotag = atidata->audioTagLookup (tagkey, tagtype);
  if (audiotag->base != NULL) {
    id3tagname = audiotag->base;
    tag = audiotag->desc;
  } else {
    id3tagname = audiotag->tag;
  }

  if (tagkey == TAG_TRACKTOTAL || tagkey == TAG_DISCTOTAL) {
    return;
  }

  if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER) {
    if (val != NULL && *val) {
      const char  *tot = NULL;

      if (tagkey == TAG_TRACKNUMBER) {
        tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
      }
      if (tagkey == TAG_DISCNUMBER) {
        tot = nlistGetStr (datalist, TAG_DISCTOTAL);
      }
      if (tot != NULL) {
        snprintf (tbuff, sizeof (tbuff), "%s/%s", val, tot);
        val = tbuff;
      }
    }
  }


  /* find any existing frame with this tag */
  idx = 0;
  while ((frame = id3_tag_findframe (id3tags, id3tagname, idx)) != NULL) {
    const char  *ttagname;

    ttagname = atibdj4GetMP3TagName (atidata, frame, tagtype);
    if (ttagname == NULL) {
      /* this is not a tag that bdj4 uses */
      ++idx;
      continue;
    }

    if (audiotag->base != NULL) {
      if (strcmp (audiotag->base, frame->id) == 0 &&
          strcmp (audiotag->desc, ttagname) == 0) {
        found = true;
        break;
      }
    } else {
      found = true;
      break;
    }
    ++idx;
  }

  if (! found) {
    frame = id3_frame_new (id3tagname);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: new: %s=%s", tag, val);
  } else {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write-raw: upd: %s=%s", tag, val);
  }

  id3val = id3_utf8_ucs4duplicate ((id3_utf8_t const *) val);
  mdextalloc (id3val);

  if (tagkey == TAG_RECORDING_ID) {
    id3_field_setlatin1 (id3_frame_field (frame, 0),
        (id3_latin1_t const *) MB_TAG);
    id3_field_setbinarydata (id3_frame_field (frame, 1),
        (id3_byte_t const *) val, strlen (val));
  } else if (audiotag->base != NULL) {
    ttag = id3_utf8_ucs4duplicate ((id3_utf8_t *) tag);
    mdextalloc (ttag);
    id3_field_settextencoding (id3_frame_field (frame, 0), ID3_FIELD_TEXTENCODING_UTF_8);
    id3_field_setstring (id3_frame_field (frame, 1), ttag);
    id3_field_setstring (id3_frame_field (frame, 2), id3val);
    dataFree (ttag);
  } else {
    /* all of the non-txxx id3 tags are string lists */
    id3_field_settextencoding (id3_frame_field (frame, 0), ID3_FIELD_TEXTENCODING_UTF_8);
    id3_field_setstrings (id3_frame_field (frame, 1), 1, &id3val);
  }
  if (! found) {
    id3_tag_attachframe (id3tags, frame);
  }
  dataFree (id3val);
}

static const char *
atibdj4GetMP3TagName (atidata_t *atidata, struct id3_frame *id3frame, int tagtype)
{
  const char            *tagname = NULL;
  const union id3_field *field;

  if (strcmp (id3frame->id, "UFID") == 0 && id3frame->nfields >= 2) {
    const id3_latin1_t *str;

    /* for binary data, there is no encoding field */
    field = &id3frame->fields [0];
    str = id3_field_getlatin1 (field);
    tagname = atidata->tagLookup (tagtype, (const char *) str);
  } else if (strcmp (id3frame->id, "TXXX") == 0 && id3frame->nfields >= 2) {
    const id3_ucs4_t  *ustr;
    id3_utf8_t        *str;

    field = &id3frame->fields [1];
    ustr = id3_field_getstring (field);
    str = id3_ucs4_utf8duplicate (ustr);
    mdextalloc (str);
    tagname = atidata->tagLookup (tagtype, (const char *) str);
    dataFree (str);
  } else {
    tagname = atidata->tagLookup (tagtype, id3frame->id);
  }

  return tagname;
}
