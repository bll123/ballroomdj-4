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
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"

void
atibdj4ParseMP3Tags (atidata_t *atidata, slist_t *tagdata,
    const char *ffn, int tagtype, int *rewrite)
{
  struct id3_file   *id3file;
  struct id3_tag    *id3tag;
  struct id3_frame  *id3frame;
  int               idx;
  char              pbuff [100];

  id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  id3tag = id3_file_tag (id3file);

  idx = 0;
  while ((id3frame = id3_tag_findframe (id3tag, "", idx)) != NULL) {
    const char          *tagname;
    id3_latin1_t const  *ufid;

    tagname = NULL;
    ufid = NULL;
    if (strcmp (id3frame->id, "TXXX") != 0 &&
        strcmp (id3frame->id, "UFID") != 0) {
      tagname = atidata->tagLookup (tagtype, id3frame->id);
      if (tagname == NULL) {
        tagname = id3frame->id;
      }
    }

    for (size_t i = 0; i < id3frame->nfields; ++i) {
      const union id3_field   *field;

      field = &id3frame->fields [i];

      switch (id3frame->fields [i].type) {
        case ID3_FIELD_TYPE_TEXTENCODING: {
          enum id3_field_textencoding   te;

          te = id3_field_gettextencoding (field);
          break;
        }
        case ID3_FIELD_TYPE_FRAMEID: {
          char const *str;

          str = id3_field_getframeid (field);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1: {
          id3_latin1_t const  *str;

          str = id3_field_getlatin1 (field);
          if (strcmp (id3frame->id, "UFID") == 0) {
            ufid = str;
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (1): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (2): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1LIST: {
          break;
        }
        case ID3_FIELD_TYPE_STRING: {
          id3_ucs4_t const *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          if (tagname == NULL) {
            tagname = atidata->tagLookup (tagtype, (const char *) str);
          } else {
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (3): %s %s=%s", tagname, id3frame->id, str);
            slistSetStr (tagdata, tagname, (const char *) str);
          }
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (4): %s %s=%s", tagname, id3frame->id, str);
          slistSetStr (tagdata, tagname, (const char *) str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr;
            id3_utf8_t        *str;
            const char        *p;

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (5): %s %s=%s", tagname, id3frame->id, str);
            p = (const char *) str;
            if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
              p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                  p, pbuff, sizeof (pbuff));
            }
            slistSetStr (tagdata, tagname, p);
          }
          break;
        }
        case ID3_FIELD_TYPE_LANGUAGE: {
          break;
        }
        case ID3_FIELD_TYPE_DATE: {
          id3_latin1_t const *str;

          str = id3_field_getlatin1 (field);
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (6): %s %s=%s", tagname, id3frame->id, str);
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
          logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (7): %s %s=%s", tagname, id3frame->id, tmp);
          slistSetStr (tagdata, tagname, tmp);
          break;
        }
        case ID3_FIELD_TYPE_BINARYDATA: {
          id3_byte_t const  *bstr;
          id3_length_t      blen;
          char              tmp [100];

          if (ufid != NULL && strcmp ((const char *) ufid, MB_TAG) == 0) {
            bstr = id3_field_getbinarydata (field, &blen);
            memcpy (tmp, bstr, blen);
            tmp [blen] = '\0';
            logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw (8): %s %s=%s=%s", tagname, id3frame->id, ufid, tmp);
            slistSetStr (tagdata, tagname, tmp);
          }
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
  return -1;
}

