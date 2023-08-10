/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "ati.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "tagdef.h"

static void  atiParseNumberPair (const char *data, int *a, int *b);

/* used to parse disc/disc-total and track/track-total number pairs */
const char *
atiParsePair (slist_t *tagdata, const char *tagname,
    const char *value, char *pbuff, size_t sz)
{
  int         tnum, ttot;
  const char  *p;

  atiParseNumberPair (value, &tnum, &ttot);

  if (ttot != 0) {
    snprintf (pbuff, sz, "%d", ttot);
    slistSetStr (tagdata, tagname, pbuff);
  }

  snprintf (pbuff, sz, "%d", tnum);
  p = pbuff;
  return p;
}

int
atiReplaceFile (const char *ffn, const char *outfn)
{
  int     rc = -1;
  char    tbuff [MAXPATHLEN];
  time_t  omodtime;

  omodtime = fileopModTime (ffn);
  snprintf (tbuff, sizeof (tbuff), "%s.bak", ffn);
  if (filemanipMove (ffn, tbuff) == 0) {
    if (filemanipMove (outfn, ffn) == 0) {
      fileopSetModTime (ffn, omodtime);
      fileopDelete (tbuff);
      rc = 0;
    } else {
      logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rename failed");
      if (filemanipMove (tbuff, ffn) != 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "restore backup failed");
      }
    }
  } else {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "move to backup failed");
  }
  return rc;
}

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
  tagname = tagLookup (tagtype, tag);
  if (tagname == NULL) {
    tagname = tag;
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
atiParseVorbisComment (const char *kw, char *buff, size_t sz)
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
atiSplitVorbisComment (int tagkey, const char *tagname, const char *val)
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

/* internal routines */

static void
atiParseNumberPair (const char *data, int *a, int *b)
{
  const char  *p;

  *a = 0;
  *b = 0;

  /* apple style track number */
  p = data;
  while (*p == ' ') {
    ++p;
  }

  if (*p == '(') {
    ++p;
    if (isdigit (*p)) {
      *a = atoi (p);
      p = strstr (p, " ");
      if (p != NULL) {
        ++p;
        *b = atoi (p);
      }
    }
    return;
  }

  /* track/total style */
  if (isdigit (*p)) {
    p = strstr (data, "/");
    *a = atoi (data);
    if (p != NULL) {
      ++p;
      *b = atoi (p);
    }
  }

  /* and do a reasonableness check */
  if (*a < 0 || *a > 5000) {
    *a = 0;
  }
  if (*b < 0 || *b > 5000) {
    *b = 0;
  }
}


