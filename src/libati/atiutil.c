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
#include "slist.h"

static void  atiParseNumberPair (const char *data, int *a, int *b);

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
      filemanipMove (tbuff, ffn);
    }
  }
  return rc;
}
