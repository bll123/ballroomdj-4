/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "audiosrc.h"
#include "asconf.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"

typedef struct asconf {
  datafile_t      *df;
  ilist_t         *audiosources;
  slist_t         *audiosrclist;
} asconf_t;

static void asconfCreateList (asconf_t *asconf);
static void asconfConvType (datafileconv_t *conv);
static void asconfConvMode (datafileconv_t *conv);

/* must be sorted in ascii order */
static datafilekey_t asconfdfkeys [] = {
  { "MODE",       ASCONF_MODE,      VALUE_NUM, asconfConvMode, DF_NORM },
  { "NAME",       ASCONF_NAME,      VALUE_STR, NULL, DF_NORM },
  { "PASS",       ASCONF_PASS,      VALUE_STR, NULL, DF_NORM },
  { "PORT",       ASCONF_PORT,      VALUE_NUM, NULL, DF_NORM },
  { "TYPE",       ASCONF_TYPE,      VALUE_NUM, asconfConvType, DF_NORM },
  { "URI" ,       ASCONF_URI,       VALUE_STR, NULL, DF_NORM },
  { "USER",       ASCONF_USER,      VALUE_STR, NULL, DF_NORM },
};
enum {
  asconfdfcount = sizeof (asconfdfkeys) / sizeof (datafilekey_t),
};

static const char *asconftype [AUDIOSRC_TYPE_MAX] = {
  [AUDIOSRC_TYPE_NONE] = "none",
  [AUDIOSRC_TYPE_FILE] = "file",
  [AUDIOSRC_TYPE_BDJ4] = "bdj4",
  [AUDIOSRC_TYPE_RTSP] = "rtsp",
};

static const char *asconfmode [ASCONF_MODE_MAX] = {
  [ASCONF_MODE_OFF] = "off",
  [ASCONF_MODE_CLIENT] = "client",
  [ASCONF_MODE_SERVER] = "server",
};

asconf_t *
asconfAlloc (void)
{
  asconf_t    *asconf;
  char        fname [MAXPATHLEN];

  asconf = mdmalloc (sizeof (asconf_t));
  asconf->audiosrclist = NULL;

  pathbldMakePath (fname, sizeof (fname), ASCONF_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  asconf->df = datafileAllocParse ("asconf", DFTYPE_INDIRECT, fname,
      asconfdfkeys, asconfdfcount, DF_NO_OFFSET, NULL);
  asconf->audiosources = datafileGetList (asconf->df);

  asconfCreateList (asconf);

  return asconf;
}

void
asconfFree (asconf_t *asconf)
{
  if (asconf == NULL) {
    return;
  }

  datafileFree (asconf->df);
  slistFree (asconf->audiosrclist);
  mdfree (asconf);
}

void
asconfStartIterator (asconf_t *asconf, slistidx_t *iteridx)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return;
  }

  /* use the audiosrclist so that the return is always sorted */
  slistStartIterator (asconf->audiosrclist, iteridx);
}

ilistidx_t
asconfIterate (asconf_t *asconf, slistidx_t *iteridx)
{
  ilistidx_t     ikey;

  if (asconf == NULL || asconf->audiosources == NULL) {
    return LIST_LOC_INVALID;
  }

  ikey = slistIterateValueNum (asconf->audiosrclist, iteridx);
  return ikey;
}

ssize_t
asconfGetCount (asconf_t *asconf)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return 0;
  }
  return ilistGetCount (asconf->audiosources);
}

const char *
asconfGetStr (asconf_t *asconf, ilistidx_t key, ilistidx_t idx)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return NULL;
  }
  return ilistGetStr (asconf->audiosources, key, idx);
}

ssize_t
asconfGetNum (asconf_t *asconf, ilistidx_t key, ilistidx_t idx)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return LIST_VALUE_INVALID;
  }
  return ilistGetNum (asconf->audiosources, key, idx);
}

void
asconfSetStr (asconf_t *asconf, ilistidx_t key, ilistidx_t idx, const char *str)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return;
  }
  ilistSetStr (asconf->audiosources, key, idx, str);
}

void
asconfSetNum (asconf_t *asconf, ilistidx_t key, ilistidx_t idx, ssize_t value)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return;
  }
  ilistSetNum (asconf->audiosources, key, idx, value);
}

slist_t *
asconfGetAudioSourceList (asconf_t *asconf)
{
  if (asconf == NULL || asconf->audiosources == NULL) {
    return NULL;
  }
  return asconf->audiosrclist;
}

void
asconfSave (asconf_t *asconf, ilist_t *list, int newdistvers)
{
  int   distvers;

  if (asconf == NULL) {
    return;
  }
  if (list == NULL) {
    list = asconf->audiosources;
  }
  distvers = datafileDistVersion (asconf->df);
  if (newdistvers > distvers) {
    distvers = newdistvers;
  }
  ilistSetVersion (list, ASCONF_CURR_VERSION);
  datafileSave (asconf->df, NULL, list, DF_NO_OFFSET, distvers);
  asconfCreateList (asconf);
}

void
asconfDelete (asconf_t *asconf, ilistidx_t key)
{
  const char  *val;

  val = ilistGetStr (asconf->audiosources, key, ASCONF_NAME);
  slistDelete (asconf->audiosrclist, val);
  ilistDelete (asconf->audiosources, key);
}

ilistidx_t
asconfAdd (asconf_t *asconf, char *name)
{
  ilistidx_t    count;

  count = ilistGetCount (asconf->audiosources);
  ilistSetNum (asconf->audiosources, count, ASCONF_MODE, ASCONF_MODE_OFF);
  ilistSetNum (asconf->audiosources, count, ASCONF_TYPE, AUDIOSRC_TYPE_RTSP);
  ilistSetStr (asconf->audiosources, count, ASCONF_NAME, name);
  ilistSetStr (asconf->audiosources, count, ASCONF_URI, "");
  ilistSetNum (asconf->audiosources, count, ASCONF_PORT, 9011);
  ilistSetStr (asconf->audiosources, count, ASCONF_USER, "");
  ilistSetStr (asconf->audiosources, count, ASCONF_PASS, "");
  slistSetNum (asconf->audiosrclist, name, count);
  return count;
}

/* internal routines */

static void
asconfCreateList (asconf_t *asconf)
{
  ilistidx_t  iteridx;
  int         key;

  slistFree (asconf->audiosrclist);
  asconf->audiosrclist = slistAlloc ("asconf-list", LIST_UNORDERED, NULL);
  slistSetSize (asconf->audiosrclist, ilistGetCount (asconf->audiosources));

  ilistStartIterator (asconf->audiosources, &iteridx);
  while ((key = ilistIterateKey (asconf->audiosources, &iteridx)) >= 0) {
    const char  *val;

    val = ilistGetStr (asconf->audiosources, key, ASCONF_NAME);
    slistSetNum (asconf->audiosrclist, val, key);
  }
  slistSort (asconf->audiosrclist);
}

static void
asconfConvType (datafileconv_t *conv)
{
  const char  *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    sval = conv->str;
    conv->num = LIST_VALUE_INVALID;
    for (int i = 0; i < AUDIOSRC_TYPE_MAX; ++i) {
      if (strcmp (sval, asconftype [i]) == 0) {
        conv->num = i;
      }
    }
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    if (conv->num < 0 || conv->num >= AUDIOSRC_TYPE_MAX) {
      sval = asconftype [AUDIOSRC_TYPE_NONE]; // unknown -> none
    } else {
      sval = asconftype [conv->num];
    }
    conv->str = sval;
  }
}

static void
asconfConvMode (datafileconv_t *conv)
{
  const char  *sval;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    sval = conv->str;
    conv->num = LIST_VALUE_INVALID;
    for (int i = 0; i < ASCONF_MODE_MAX; ++i) {
      if (strcmp (sval, asconfmode [i]) == 0) {
        conv->num = i;
      }
    }
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    if (conv->num < 0 || conv->num >= ASCONF_MODE_MAX) {
      sval = asconfmode [ASCONF_MODE_OFF]; // unknown -> off
    } else {
      sval = asconfmode [conv->num];
    }
    conv->str = sval;
  }
}

