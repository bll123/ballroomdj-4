#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "datafile.h"
#include "tmutil.h"
#include "list.h"
#include "fileutil.h"

typedef enum {
  PARSE_SIMPLE,
  PARSE_KEYVALUE
} parsetype_t;

static size_t parse (parseinfo_t *pi, char *data, parsetype_t parsetype);
static void   datafileFreeInternal (datafile_t *df);

/* parsing routines */

parseinfo_t *
parseInit (void)
{
  parseinfo_t   *pi;

  pi = malloc (sizeof (parseinfo_t));
  assert (pi != NULL);
  pi->strdata = NULL;
  pi->allocCount = 0;
  pi->count = 0;
  return pi;
}

void
parseFree (parseinfo_t *pi)
{
  if (pi != NULL) {
    if (pi->strdata != NULL) {
      free (pi->strdata);
    }
    free (pi);
  }
}

char **
parseGetData (parseinfo_t *pi)
{
  return pi->strdata;
}

size_t
parseSimple (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_SIMPLE);
}

size_t
parseKeyValue (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_KEYVALUE);
}

/* datafile loading routines */

datafile_t *
datafileAlloc (datafilekey_t *dfkeys, size_t dfkeycount, char *fname)
{
  datafile_t      *df;

  df = malloc (sizeof (datafile_t));
  assert (df != NULL);
  df->fname = NULL;
  df->data = NULL;
  datafileLoad (dfkeys, dfkeycount, df, fname);
  return df;
}

void
datafileFree (void *tdf)
{
  datafile_t    *df = (datafile_t *) tdf;

  if (df != NULL) {
    datafileFreeInternal (df);
    free (df);
  }
}

void
datafileLoad (datafilekey_t *dfkeys, size_t dfkeycount,
    datafile_t *df, char *fname)
{
  char          *data;
  char          **strdata;
  parseinfo_t   *pi;
  long          key = -1L;
  long          ikey;
  size_t        dataCount;
  listkey_t     lkey;
  list_t        *itemList = NULL;
  valuetype_t   vt;

  /* may be re-using the song list */
  datafileFreeInternal (df);
  df->fname = strdup (fname);

  data = fileReadAll (df->fname);
  pi = parseInit ();
  dataCount = parseKeyValue (pi, data);
  strdata = parseGetData (pi);

  for (size_t i = 0; i < dataCount; i += 2) {
    if (strcmp (strdata [i], "version") == 0) {
      continue;
    }
    if (strcmp (strdata [i], "count") == 0) {
      df->data = vlistAlloc (KEY_LONG, LIST_UNORDERED, NULL, NULL, listFree);
      vlistSetSize (df->data, (size_t) atol (strdata [i + 1]));
      continue;
    }
    if (strcmp (strdata [i], "KEY") == 0) {
      if (key >= 0) {
        lkey.key = key;
        vlistSetData (df->data, lkey, itemList);
        key = -1L;
      }
      key = atol (strdata [i + 1]);
      itemList = vlistAlloc (KEY_LONG, LIST_ORDERED, NULL, NULL, free);
      continue;
    }

    /* for the moment, do a brute force search */
    for (size_t j = 0; j < dfkeycount; ++j) {
      if (strcmp (strdata [i], dfkeys [j].name) == 0) {
        ikey = dfkeys [j].itemkey;
        vt = dfkeys [j].valuetype;
      }
    }
    lkey.key = ikey;
    if (vt == VALUE_DATA) {
      vlistSetData (itemList, lkey, strdup (strdata [i + 1]));
    }
    if (vt == VALUE_LONG) {
      vlistSetLong (itemList, lkey, atol (strdata [i + 1]));
    }
  }
  if (key >= 0) {
    lkey.key = key;
    vlistSetData (df->data, lkey, itemList);
    key = -1L;
  }
  parseFree (pi);
  free (data);
}

int
datafileSave (datafilekey_t *dfkeys, size_t dfkeycount, datafile_t *data)
{
/* ### FIX : TODO */
  makeBackups (data->fname, 2);
  return 0;
}

/* internal parse routines */

static size_t
parse (parseinfo_t *pi, char *data, parsetype_t parsetype)
{
  char        *tokptr;
  char        *str;
  size_t      dataCounter;

  tokptr = NULL;
  if (pi->allocCount < 60) {
    pi->allocCount = 60;
    pi->strdata = realloc (pi->strdata, sizeof (char *) * pi->allocCount);
    assert (pi->strdata != NULL);
  }

  str = strtok_r (data, "\n", &tokptr);
  dataCounter = 0;
  while (str != NULL) {
    if (*str == '#') {
      str = strtok_r (NULL, "\n", &tokptr);
      continue;
    }

    if ((size_t) dataCounter >= pi->allocCount) {
      pi->allocCount += 10;
      pi->strdata = realloc (pi->strdata, sizeof (char *) * pi->allocCount);
      assert (pi->strdata != NULL);
    }
    if (parsetype == PARSE_KEYVALUE && dataCounter % 2 == 1) {
      /* value data is indented */
      str += 2;
    }
    pi->strdata [dataCounter] = str;
    ++dataCounter;
    str = strtok_r (NULL, "\n", &tokptr);
  }
  pi->count = dataCounter;
  return dataCounter;
}

/* internal datafile routines */

static void
datafileFreeInternal (datafile_t *df)
{
  if (df != NULL) {
    if (df->fname != NULL) {
      free (df->fname);
      df->fname = NULL;
    }
    if (df->data != NULL) {
      vlistFree (df->data);
      df->data = NULL;
    }
  }
}

