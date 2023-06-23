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

#include "ati.h"
#include "bdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "slist.h"

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

/* internal routines */

static void
atiParseNumberPair (const char *data, int *a, int *b)
{
  const char  *p;

  *a = 0;
  *b = 0;

  /* apple style track number */
  if (*data == '(') {
    p = data;
    ++p;
    *a = atoi (p);
    p = strstr (p, " ");
    if (p != NULL) {
      ++p;
      *b = atoi (p);
    }
    return;
  }

  /* track/total style */
  p = strstr (data, "/");
  *a = atoi (data);
  if (p != NULL) {
    ++p;
    *b = atoi (p);
  }
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
atiProcessVorbisComment (taglookup_t tagLookup, slist_t *tagdata,
    int tagtype, const char *kw)
{
  const char  *val;
  const char  *tagname;
  char        ttag [300];

  val = atiParseVorbisComment (kw, ttag, sizeof (ttag));
  tagname = tagLookup (tagtype, ttag);
  if (tagname == NULL) {
    tagname = ttag;
  }
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s %s", tagname, kw);
  slistSetStr (tagdata, tagname, val);
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

