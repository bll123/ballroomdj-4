/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

static const char * const MB_TAG = "http://musicbrainz.org";

typedef struct atisaved {
  bool            hasdata;
  int             filetype;
  int             tagtype;
  struct id3_tag  *id3tags;
} atisaved_t;

static void atibdj4AddMP3Tag (atidata_t *atidata, nlist_t *datalist, struct id3_tag *id3tags, const char *tag, const char *val, int tagtype, const char *is639_2);
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
  bool              commvalid = false;

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READONLY);
  if (id3file == NULL) {
    return;
  }
  id3tags = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tags, "", idx)) != NULL) {
    const char                  *ufid = NULL;
    const char                  *tagname = NULL;

    ufid = NULL;
    tagname = atibdj4GetMP3TagName (atidata, id3frame, tagtype);
    if (strcmp (id3frame->id, "COMM") == 0) {
      commvalid = false;
    }

    /* null tagnames are checked in each field type */
    /* so that the raw data can be logged */

    /* note that the ordering is not enforced by the id3tag library */
    /* t---: field 0 is the text encoding, field 1 is the data */
    /* ufid: field 0 is the name, field 1 is the data */
    /* txxx: field 0 is the text encoding, field 1 is the name, */
    /*       field 2 is the data */
    /* comm: field 0 is the text encoding, field 1 is the language, */
    /*       field 2 is the data */
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
            if (tagname != NULL) {
              slistSetStr (tagdata, tagname, (const char *) str);
            }
          }
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (2): %s %s=%s", tagname, id3frame->id, str);
          if (tagname != NULL) {
            slistSetStr (tagdata, tagname, (const char *) str);
          }
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
          if (ustr != NULL) {
            str = id3_ucs4_utf8duplicate (ustr);
            mdextalloc (str);
          }
          if (strcmp (id3frame->id, "COMM") == 0) {
            if (*str == '\0' || strcmp ((const char *) str, "description") == 0) {
              commvalid = true;
            }
          }
          if (i != 1 ||
              strcmp (id3frame->id, "TXXX") != 0 ||
              strcmp (id3frame->id, "COMM") != 0) {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (3): %s %s=%s", tagname, id3frame->id, str);
            if (tagname != NULL) {
              slistSetStr (tagdata, tagname, (const char *) str);
            }
          }
          dataFree (str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr = NULL;
          id3_utf8_t        *str = NULL;
          bool              valid = true;

          ustr = id3_field_getfullstring (field);
          if (ustr != NULL) {
            str = id3_ucs4_utf8duplicate (ustr);
            mdextalloc (str);
          }
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (4): %s %s=%s", tagname, id3frame->id, str);
          if (tagname == NULL) {
            valid = false;
          }
          if (strcmp (id3frame->id, "COMM") == 0 && ! commvalid) {
            valid = false;
          }
          if (valid) {
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          dataFree (str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          /* the processing here is limited, only the first string */
          /* of the stringlist is handled */

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr = NULL;
            id3_utf8_t        *str = NULL;
            const char        *p = NULL;

            ustr = id3_field_getstrings (field, j);

            /* genre tags may need to be converted from the id3 */
            /* numeric value */
            if (tagname != NULL &&
                strcmp (tagname, atidata->tagName (TAG_GENRE)) == 0) {
              ustr = id3_genre_name (ustr);
            }

            if (ustr != NULL) {
              str = id3_ucs4_utf8duplicate (ustr);
              mdextalloc (str);
            }

            /* gcc is incorrectly complaining 2024-2-16 */
            /* if a pragma gcc is used, then clang complains */
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  raw (5): %s %s=%s", tagname, id3frame->id, str);

            p = (const char *) str;
            if (tagname != NULL &&
                strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            if (tagname != NULL &&
                strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            if (tagname != NULL &&
                strcmp (tagname, atidata->tagName (TAG_MOVEMENTNUM)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_MOVEMENTCOUNT),
                  p, pbuff, sizeof (pbuff));
            }
            if (tagname != NULL) {
              slistSetStr (tagdata, tagname, p);
            }
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
          if (tagname != NULL) {
            slistSetStr (tagdata, tagname, (const char *) str);
          }
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
          if (tagname != NULL) {
            slistSetStr (tagdata, tagname, tmp);
          }
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
            if (tagname != NULL) {
              slistSetStr (tagdata, tagname, tmp);
            }
          } else {
            if (tagname != NULL) {
              slistSetData (tagdata, tagname, tmp);
            }
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
    int tagtype, int filetype, const char *iso639_2, int32_t flags)
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

    if (slistGetNum (dellist, tagname) == 1) {
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
    atibdj4AddMP3Tag (atidata, datalist, id3tags, key, slistGetStr (updatelist, key), tagtype, iso639_2);
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
  mdextalloc (nid3tags);

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
atibdj4FreeSavedMP3Tags (atisaved_t *atisaved, int tagtype, int filetype)
{
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

  atisaved->hasdata = false;
  mdextfree (atisaved->id3tags);
  id3_tag_delete (atisaved->id3tags);
  mdfree (atisaved);
}

int
atibdj4RestoreMP3Tags (atidata_t *atidata,
    atisaved_t *atisaved, const char *ffn, int tagtype, int filetype)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tags;
  struct id3_frame  *id3frame;
  int               idx;

  if (atisaved == NULL) {
    return -1;
  }

  if (! atisaved->hasdata) {
    return -1;
  }

  if (atisaved->tagtype != tagtype) {
    return -1;
  }
  if (atisaved->filetype != filetype) {
    return -1;
  }

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  if (id3file == NULL) {
    return -1;
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

  return 0;
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
    struct id3_tag *id3tags, const char *tag, const char *val, int tagtype,
    const char *iso639_2)
{
  int                 writetags;
  int                 tagkey;
  const tagaudiotag_t *audiotag;
  id3_ucs4_t          *id3val;
  struct id3_frame    *frame = NULL;
  int                 idx;
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

  if (tagkey == TAG_TRACKTOTAL ||
      tagkey == TAG_DISCTOTAL ||
      tagkey == TAG_MOVEMENTCOUNT) {
    return;
  }

  if (tagkey == TAG_TRACKNUMBER ||
      tagkey == TAG_DISCNUMBER ||
      tagkey == TAG_MOVEMENTNUM) {
    if (val != NULL && *val) {
      const char  *tot = NULL;

      if (tagkey == TAG_TRACKNUMBER) {
        tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
      }
      if (tagkey == TAG_DISCNUMBER) {
        tot = nlistGetStr (datalist, TAG_DISCTOTAL);
      }
      if (tagkey == TAG_MOVEMENTNUM) {
        tot = nlistGetStr (datalist, TAG_MOVEMENTCOUNT);
      }
      if (tot != NULL && *tot) {
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
  } else if (tagkey == TAG_COMMENT) {
    id3_ucs4_t    *empty = NULL;

    /* match itunes */
    empty = id3_utf8_ucs4duplicate ((id3_utf8_t *) "");
    mdextalloc (empty);
    id3_field_settextencoding (id3_frame_field (frame, 0), ID3_FIELD_TEXTENCODING_UTF_8);
    id3_field_setlanguage (id3_frame_field (frame, 1), iso639_2);
    id3_field_setstring (id3_frame_field (frame, 2), empty);
    id3_field_setfullstring (id3_frame_field (frame, 3), id3val);
    dataFree (empty);
  } else if (audiotag->base != NULL) {
    id3_ucs4_t    *ttag = NULL;

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
  } else if (strcmp (id3frame->id, "COMM") == 0 && id3frame->nfields >= 2) {
    const id3_ucs4_t  *ustr;
    id3_utf8_t        *str = NULL;

    field = &id3frame->fields [2];
    ustr = id3_field_getstring (field);
    if (ustr != NULL) {
      str = id3_ucs4_utf8duplicate (ustr);
      mdextalloc (str);
    }
    /* comments may have different names */
    /* iTunes writes out an empty name */
    /* musicbrainz indicates that 'description' is used */
    /* ignore anything else */
    if (str == NULL ||
        *str == '\0' ||
        strcmp ((const char *) str, "description") == 0) {
      tagname = atidata->tagLookup (tagtype, id3frame->id);
    }
    dataFree (str);
  } else if (strcmp (id3frame->id, "TXXX") == 0 && id3frame->nfields >= 2) {
    const id3_ucs4_t  *ustr;
    id3_utf8_t        *str = NULL;

    field = &id3frame->fields [1];
    ustr = id3_field_getstring (field);
    if (ustr != NULL) {
      str = id3_ucs4_utf8duplicate (ustr);
      mdextalloc (str);
    }
    tagname = atidata->tagLookup (tagtype, (const char *) str);
    dataFree (str);
  } else {
    tagname = atidata->tagLookup (tagtype, id3frame->id);
  }

  return tagname;
}
