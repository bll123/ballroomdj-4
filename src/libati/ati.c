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
#include "audiofile.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "pathbld.h"
#include "dylib.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "volsink.h"

typedef struct ati {
  dlhandle_t        *dlHandle;
  atidata_t         *(*atiiInit) (const char *, int, taglookup_t, tagcheck_t);
  void              (*atiiFree) (atidata_t *atidata);
  char              *(*atiiReadTags) (atidata_t *atidata, const char *ffn);
  void              (*atiiParseTags) (atidata_t *atidata, slist_t *tagdata, char *data, int tagtype, int *rewrite);
  int               (*atiiWriteTags) (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
  atidata_t         *atidata;
  taglookup_t       tagLookup;
} ati_t;

ati_t *
atiInit (const char *atipkg, taglookup_t tagLookup, tagcheck_t tagCheck)
{
  ati_t     *ati;
  char      dlpath [MAXPATHLEN];

  ati = mdmalloc (sizeof (ati_t));
  ati->atiiInit = NULL;
  ati->atiiFree = NULL;
  ati->atiiReadTags = NULL;
  ati->atiiParseTags = NULL;
  ati->atiiWriteTags = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      atipkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  ati->dlHandle = dylibLoad (dlpath);
  if (ati->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    mdfree (ati);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  ati->atiiInit = dylibLookup (ati->dlHandle, "atiiInit");
  ati->atiiFree = dylibLookup (ati->dlHandle, "atiiFree");
  ati->atiiReadTags = dylibLookup (ati->dlHandle, "atiiReadTags");
  ati->atiiParseTags = dylibLookup (ati->dlHandle, "atiiParseTags");
  ati->atiiWriteTags = dylibLookup (ati->dlHandle, "atiiWriteTags");
#pragma clang diagnostic pop

  if (ati->atiiInit != NULL) {
    ati->atidata = ati->atiiInit (atipkg,
        bdjoptGetNum (OPT_G_WRITETAGS),
        tagLookup, tagCheck);
  }
  return ati;
}

void
atiFree (ati_t *ati)
{
  if (ati != NULL) {
    if (ati->atiiFree != NULL) {
      ati->atiiFree (ati->atidata);
    }
    if (ati->dlHandle != NULL) {
      dylibClose (ati->dlHandle);
    }
    mdfree (ati);
  }
}

char *
atiReadTags (ati_t *ati, const char *ffn)
{
  if (ati != NULL && ati->atiiReadTags != NULL) {
    return ati->atiiReadTags (ati->atidata, ffn);
  }
  return NULL;
}

void
atiParseTags (ati_t *ati, slist_t *tagdata,
    char *data, int tagtype, int *rewrite)
{
  slistidx_t    iteridx;
  const char    *tag;

  if (ati != NULL && ati->atiiParseTags != NULL) {
    ati->atiiParseTags (ati->atidata, tagdata, data, tagtype, rewrite);
  }

  slistStartIterator (tagdata, &iteridx);
  while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
    /* some old audio file tag handling */
    if (strcmp (tag, tagdefs [TAG_DURATION].tag) == 0) {
      logMsg (LOG_DBG, LOG_DBUPDATE, "rewrite: duration");
      *rewrite |= AF_REWRITE_DURATION;
    }

    /* old songend/songstart handling */
    if (strcmp (tag, tagdefs [TAG_SONGSTART].tag) == 0 ||
        strcmp (tag, tagdefs [TAG_SONGEND].tag) == 0) {
      const char  *tagval;
      char        *p;
      char        *tokstr;
      char        *tmp;
      char        pbuff [40];
      double      tm = 0.0;

      tagval = slistGetStr (tagdata, tag);
      if (strstr (tagval, ":") != NULL) {
        tmp = mdstrdup (tagval);
        p = strtok_r (tmp, ":", &tokstr);
        if (p != NULL) {
          tm += atof (p) * 60.0;
          p = strtok_r (NULL, ":", &tokstr);
          tm += atof (p);
          tm *= 1000;
          snprintf (pbuff, sizeof (pbuff), "%.0f", tm);
          slistSetStr (tagdata, tag, pbuff);
        }
        mdfree (tmp);
      }
    }

    /* old volumeadjustperc handling */
    if (strcmp (tag, tagdefs [TAG_VOLUMEADJUSTPERC].tag) == 0) {
      const char  *tagval;
      const char  *radix;
      char        *tmp;
      char        *p;
      char        pbuff [40];
      double      tm = 0.0;

      tagval = slistGetStr (tagdata, tag);
      radix = sysvarsGetStr (SV_LOCALE_RADIX);

      /* the BDJ3 volume adjust percentage is a double */
      /* with or without a decimal point */
      /* convert it to BDJ4 style */
      /* this will fail for large BDJ3 values w/no decimal */
      if (strstr (tagval, ".") != NULL || strlen (tagval) <= 3) {
        tmp = mdstrdup (tagval);
        p = strstr (tmp, ".");
        if (p != NULL) {
          if (radix != NULL) {
            *p = *radix;
          }
        }
        tm = atof (tagval);
        tm /= 10.0;
        tm *= DF_DOUBLE_MULT;
        snprintf (pbuff, sizeof (pbuff), "%.0f", tm);
        slistSetStr (tagdata, tag, pbuff);
        mdfree (tmp);
      }
    }
  }

  return;
}

int
atiWriteTags (ati_t *ati, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  if (ati != NULL && ati->atiiWriteTags != NULL) {
    return ati->atiiWriteTags (ati->atidata, ffn,
        updatelist, dellist, datalist,
        tagtype, filetype);
  }
  return 0;
}
