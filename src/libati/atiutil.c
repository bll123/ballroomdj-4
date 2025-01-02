/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

  *pbuff = '\0';
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

/* internal routines */

static void
atiParseNumberPair (const char *data, int *a, int *b)
{
  int     rc;

  *a = 0;
  *b = 0;

  rc = sscanf (data, "%d/%d", a, b);
  if (rc != 1 && rc != 2) {
    rc = sscanf (data, "(%d,%d)", a, b);
  }

  if (rc == 0) {
    *a = 0;
    *b = 0;
  }

  /* and do a reasonableness check */
  if (*a < 0 || *a > 5000) {
    *a = 0;
  }
  if (*b < 0 || *b > 5000) {
    *b = 0;
  }
}


