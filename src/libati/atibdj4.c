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
#include "audiofile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tagdef.h"

#define MB_TAG      "http://musicbrainz.org"
#define MB_TAG_LEN  strlen (MB_TAG)

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
  struct id3_file   *id3file;
  struct id3_tag    *id3tag;
} atidata_t;

const char *
atiiDesc (void)
{
  return "BDJ4: MP3 Only";
}

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
  atidata->id3file = NULL;
  atidata->id3tag = NULL;

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    if (atidata->id3file != NULL) {
      id3_file_close (atidata->id3file);
    }
    mdfree (atidata);
  }
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char    *data = NULL;

  atidata->id3file = id3_file_open (ffn, ID3_FILE_MODE_READWRITE);
  atidata->id3tag = id3_file_tag (atidata->id3file);
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data,
    int tagtype, int *rewrite)
{
  struct id3_frame  *id3frame;
  int               idx;

  idx = 0;
  while ((id3frame = id3_tag_findframe (atidata->id3tag, "", idx)) != NULL) {
fprintf (stderr, "id: %s %d\n", id3frame->id, id3frame->nfields);
    for (size_t i = 0; i < id3frame->nfields; ++i) {
      const union id3_field   *field;

      field = &id3frame->fields [i];

fprintf (stderr, "  %d %d : ", (int) i, field->type);
      switch (id3frame->fields [i].type) {
        case ID3_FIELD_TYPE_TEXTENCODING: {
fprintf (stderr, "  \n");
          break;
        }
        case ID3_FIELD_TYPE_FRAMEID: {
          char const *str;

          str = id3_field_getframeid (field);
fprintf (stderr, "  %s\n", str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1: {
          id3_latin1_t const *str;

          str = id3_field_getlatin1 (field);
fprintf (stderr, "  %s\n", str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1FULL: {
          id3_latin1_t const *str;

          str = id3_field_getfulllatin1 (field);
fprintf (stderr, "  %s\n", str);
          break;
        }
        case ID3_FIELD_TYPE_LATIN1LIST: {
fprintf (stderr, "  \n");
          break;
        }
        case ID3_FIELD_TYPE_STRING: {
          id3_ucs4_t const *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
fprintf (stderr, "  %s\n", str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGFULL: {
          id3_ucs4_t const  *ustr;
          id3_utf8_t        *str;

          ustr = id3_field_getstring (field);
          str = id3_ucs4_utf8duplicate (ustr);
fprintf (stderr, "  %s\n", str);
          break;
        }
        case ID3_FIELD_TYPE_STRINGLIST: {
          size_t    nstr;

          nstr = id3_field_getnstrings (field);
          for (size_t j = 0; j < nstr; ++j) {
            id3_ucs4_t const  *ustr;
            id3_utf8_t        *str;

            ustr = id3_field_getstrings (field, j);
            str = id3_ucs4_utf8duplicate (ustr);
fprintf (stderr, "  %s\n", str);
          }
          break;
        }
        case ID3_FIELD_TYPE_LANGUAGE: {
fprintf (stderr, "  \n");
          break;
        }
        case ID3_FIELD_TYPE_DATE: {
fprintf (stderr, "  \n");
          break;
        }
        case ID3_FIELD_TYPE_INT8:
        case ID3_FIELD_TYPE_INT16:
        case ID3_FIELD_TYPE_INT24:
        case ID3_FIELD_TYPE_INT32:
        case ID3_FIELD_TYPE_INT32PLUS: {
          long    val;

          val = id3_field_getint (field);
fprintf (stderr, "  %ld\n", val);
          break;
        }
        case ID3_FIELD_TYPE_BINARYDATA: {
          id3_byte_t const  *bstr;
          id3_length_t      blen;

          bstr = id3_field_getbinarydata (field, &blen);
fprintf (stderr, "  %.*s\n", (int) blen, bstr);
          break;
        }
      }
    }
    ++idx;
  }

  return;
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  int         rc = -1;

  return rc;
}

