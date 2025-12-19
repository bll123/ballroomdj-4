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

#include "bdj4.h"
#include "ativcutil.h"
#include "log.h"
#include "mdebug.h"
/* it would be better to include tagdefs.h, */
/* but for linkage purposes, the ati libraries avoid linking in libbdj4 */

void
atiProcessVorbisCommentCombined (taglookup_t tagLookup, slist_t *tagdata,
    int tagtype, const char *kw)
{
  const char  *val;
  char        ttag [300];

  val = atiParseVorbisComment (kw, ttag, sizeof (ttag));
  /* vorbis comments are not case sensitive */
  stringAsciiToUpper (ttag);
  if (strcmp (ttag, "METADATA_BLOCK_PICTURE") == 0) {
    /* this is a lot of data to carry around, and it's not needed */
    return;
  }
  atiProcessVorbisComment (tagLookup, tagdata, tagtype, ttag, val);
}

void
atiProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata,
    int tagtype, const char *tag, const char *val)
{
  const char  *tagname;
  bool        exists = false;
  char        *tmp;

  /* vorbis comments are not case sensitive */
  /* the parse upper cases the tag name */
  tagname = tagLookup (tagtype, tag);

  if (tagname == NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: unk %s=%s", tag, val);
    return;
  }

  if (strcmp (tagname, "TOTALTRACKS") == 0) {
    tagname = "TRACKTOTAL";
  }
  if (strcmp (tagname, "TOTALDISCS") == 0) {
    tagname = "DISCTOTAL";
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s %s=%s", tagname, tag, val);

  /* any duplicated tagnames are appended to the tag with a semi-colon */
  if (slistGetStr (tagdata, tagname) != NULL) {
    const char  *oval;
    size_t      sz;

    oval = slistGetStr (tagdata, tagname);
    sz = strlen (oval) + strlen (val) + 3;
    tmp = mdmalloc (sz);
    snprintf (tmp, sz, "%s; %s", oval, val);
    val = tmp;
    exists = true;
  }
  slistSetStr (tagdata, tagname, val);
  if (exists) {
    mdfree (tmp);
  }
}

const char *
atiParseVorbisComment (const char *kw, char *buff, size_t sz)
{
  const char  *val;
  size_t      len;

  val = strstr (kw, "=");
  if (val == NULL) {
    return NULL;
  }
  len = val - kw + 1;
  if (len > sz) {
    len = sz;
  }
  stpecpy (buff, buff + len, kw);
  buff [len] = '\0';
  ++val;

  return val;
}

/* the caller must free the return value */
slist_t *
atiSplitVorbisComment (int tagkey, const char *tagname, const char *val)
{
  slist_t     *vallist = NULL;
  char        *tokstr;
  char        *p;
  bool        split = false;

  /* known tags that should be split: */
  /* album-artist, artist, composer, conductor, genre */

  vallist = slistAlloc ("vc-list", LIST_UNORDERED, NULL);
  /* do this rather than using tagdefs, so that libbdj4 will not be */
  /* linked in */
  if (tagkey == TAG_ALBUMARTIST ||
      tagkey == TAG_ARTIST ||
      tagkey == TAG_COMPOSER ||
      tagkey == TAG_CONDUCTOR ||
      tagkey == TAG_GENRE) {
    split = true;
  }

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

