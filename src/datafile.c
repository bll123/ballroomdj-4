#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "datafile.h"
#include "tmutil.h"
#include "list.h"
#include "fileop.h"
#include "bdjstring.h"
#include "log.h"
#include "portability.h"
#include "filedata.h"

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

  logProcBegin (LOG_LVL_8, "parseInit");
  pi = malloc (sizeof (parseinfo_t));
  assert (pi != NULL);
  pi->strdata = NULL;
  pi->allocCount = 0;
  pi->count = 0;
  logProcEnd (LOG_LVL_8, "parseInit", "");
  return pi;
}

void
parseFree (parseinfo_t *pi)
{
  logProcBegin (LOG_LVL_8, "parseFree");
  if (pi != NULL) {
    if (pi->strdata != NULL) {
      free (pi->strdata);
    }
    free (pi);
  }
  logProcEnd (LOG_LVL_8, "parseFree", "");
}

char **
parseGetData (parseinfo_t *pi)
{
  return pi->strdata;
}

size_t
parseSimple (parseinfo_t *pi, char *data)
{
  logProcBegin (LOG_LVL_8, "parseSimple");
  logProcEnd (LOG_LVL_8, "parseSimple", "");
  return parse (pi, data, PARSE_SIMPLE);
}

size_t
parseKeyValue (parseinfo_t *pi, char *data)
{
  logProcBegin (LOG_LVL_8, "parseKeyValue");
  logProcEnd (LOG_LVL_8, "parseKeyValue", "");
  return parse (pi, data, PARSE_KEYVALUE);
}

void
parseConvBoolean (char *data, datafileret_t *ret)
{
  logProcBegin (LOG_LVL_8, "parseConvBoolean");
  ret->valuetype = VALUE_LONG;
  ret->u.l = 0;
  if (strcmp (data, "on") == 0 ||
      strcmp (data, "yes") == 0 ||
      strcmp (data, "1") == 0) {
    ret->u.l = 1;
  }
  logProcEnd (LOG_LVL_8, "parseConvBoolean", "");
}

void
parseConvTextList (char *data, datafileret_t *ret)
{
  char          *tokptr;
  char          *p;

  logProcBegin (LOG_LVL_8, "parseConvTextList");
  ret->valuetype = VALUE_LIST;
  ret->u.list = NULL;
  ret->u.list = listAlloc ("textlist", LIST_ORDERED, istringCompare, free, NULL);
  if (data != NULL) {
    p = strtok_r (data, " ,", &tokptr);
    while (p != NULL) {
      listSetData (ret->u.list, strdup (p), NULL);
      p = strtok_r (NULL, " ,", &tokptr);
    }
  }
  logProcEnd (LOG_LVL_8, "parseConvTextList", "");
}

/* datafile loading routines */

datafile_t *
datafileAlloc (char *name)
{
  datafile_t      *df;

  logProcBegin (LOG_LVL_8, "datafileAlloc");
  df = malloc (sizeof (datafile_t));
  assert (df != NULL);
  df->name = name;
  df->fname = NULL;
  df->data = NULL;
  df->lookup = NULL;
  df->dftype = DFTYPE_NONE;
  logProcEnd (LOG_LVL_8, "datafileAlloc", "");
  return df;
}

datafile_t *
datafileAllocParse (char *name, datafiletype_t dftype, char *fname,
    datafilekey_t *dfkeys, size_t dfkeycount, long lookupKey)
{
  logProcBegin (LOG_LVL_5, "datafileAllocParse");
  logMsg (LOG_DBG, LOG_LVL_5, "alloc/parse %s", fname);
  datafile_t *df = datafileAlloc (name);
  if (df != NULL) {
    char *ddata = datafileLoad (df, dftype, fname);
    if (ddata != NULL) {
      df->data = datafileParse (ddata, name, dftype,
          dfkeys, dfkeycount, lookupKey, &df->lookup);
      if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
        listSort (df->data);
      } else if (dftype != DFTYPE_LIST) {
        llistSort (df->data);
      }
      free (ddata);
    }
  }
  logProcEnd (LOG_LVL_5, "datafileAllocParse", "");
  return df;
}

void
datafileFree (void *tdf)
{
  datafile_t    *df = (datafile_t *) tdf;

  logProcBegin (LOG_LVL_8, "datafileFree");
  if (df != NULL) {
    datafileFreeInternal (df);
    free (df);
  }
  logProcEnd (LOG_LVL_8, "datafileFree", "");
}

char *
datafileLoad (datafile_t *df, datafiletype_t dftype, char *fname)
{
  char    *data;

  logProcBegin (LOG_LVL_8, "datafileLoad");
  logMsg (LOG_DBG, LOG_LVL_5, "load %s", fname);
  df->dftype = dftype;
  if (df->fname == NULL) {
    df->fname = strdup (fname);
    assert (df->fname != NULL);
  }
  data = filedataReadAll (df->fname);
  logProcEnd (LOG_LVL_8, "datafileLoad", "");
  return data;
}

list_t *
datafileParse (char *data, char *name, datafiletype_t dftype,
    datafilekey_t *dfkeys, size_t dfkeycount, long lookupKey,
    list_t **lookup)
{
  list_t        *nlist = NULL;

  nlist = datafileParseMerge (nlist, data, name, dftype,
      dfkeys, dfkeycount, lookupKey, lookup);
  return nlist;
}

list_t *
datafileParseMerge (list_t *nlist, char *data, char *name,
    datafiletype_t dftype, datafilekey_t *dfkeys,
    size_t dfkeycount, long lookupKey, list_t **lookup)
{
  char          **strdata = NULL;
  parseinfo_t   *pi = NULL;
  long          key = -1L;
  size_t        dataCount;
  list_t        *itemList = NULL;
  valuetype_t   vt = 0;
  size_t        inc = 2;
  int           first = 1;
  long          ikey = 0;
  long          lval = 0;
  char          *tkeystr;
  char          *tvalstr;
  char          *tlookupkey = NULL;
  datafileret_t ret;

  logProcBegin (LOG_LVL_8, "datafileParse");

  pi = parseInit ();
  if (dftype == DFTYPE_LIST) {
    dataCount = parseSimple (pi, data);
  } else {
    dataCount = parseKeyValue (pi, data);
  }
  strdata = parseGetData (pi);

  logMsg (LOG_DBG, LOG_LVL_8, "dftype: %ld", dftype);
  switch (dftype) {
    case DFTYPE_LIST: {
      inc = 1;
      if (nlist == NULL) {
        nlist = listAlloc (name, LIST_UNORDERED, NULL, free, NULL);
        listSetSize (nlist, dataCount);
      } else {
        listSetSize (nlist, dataCount + listGetSize (nlist));
      }
      break;
    }
    case DFTYPE_KEY_LONG: {
      inc = 2;
      if (nlist == NULL) {
        nlist = llistAlloc (name, LIST_UNORDERED, listFree);
      }
      break;
    }
    case DFTYPE_KEY_VAL: {
      inc = 2;
      if (dfkeys == NULL) {
        if (nlist == NULL) {
          nlist = listAlloc (name, LIST_UNORDERED,
              istringCompare, free, free);
          listSetSize (nlist, dataCount / 2);
        } else {
          listSetSize (nlist, dataCount / 2 + listGetSize (nlist));
        }
        logMsg (LOG_DBG, LOG_LVL_8, "key_val: list");
      } else {
        if (nlist == NULL) {
          nlist = llistAlloc (name, LIST_UNORDERED, free);
          llistSetSize (nlist, dataCount / 2);
        } else {
          llistSetSize (nlist, dataCount / 2 + listGetSize (nlist));
        }
        logMsg (LOG_DBG, LOG_LVL_8, "key_val: llist");
      }
      break;
    }
    default: {
      break;
    }
  }

  if (lookupKey != DATAFILE_NO_LOOKUP) {
    char      temp [80];

    snprintf (temp, sizeof (temp), "%s-lookup", name);
// ### FIX: optimize whether to use istringCompare or stringCompare.
    *lookup = listAlloc (temp, LIST_UNORDERED, istringCompare, free, NULL);
    listSetSize (*lookup, listGetSize (nlist));
  }

  for (size_t i = 0; i < dataCount; i += inc) {
    tkeystr = strdata [i];
    tvalstr = strdata [i + 1];

    if (inc == 2 && strcmp (strdata [i], "version") == 0) {
//      version = atol (tvalstr);
// ### FIX TOD: want to save version somewhere...
      continue;
    }
    if (strcmp (strdata [i], "count") == 0) {
      if (dftype == DFTYPE_KEY_LONG) {
        llistSetSize (nlist, (size_t) atol (tvalstr));
      }
      listSetSize (*lookup, listGetSize (nlist));
      continue;
    }

    if (dftype == DFTYPE_KEY_LONG &&
        strcmp (tkeystr, "KEY") == 0) {
      char      temp [80];

      if (key >= 0) {
        llistSetList (nlist, key, itemList);
        if (lookup != NULL &&
            lookupKey != DATAFILE_NO_LOOKUP &&
            tlookupkey != NULL) {
          listSetLong (*lookup, tlookupkey, key);
        }
        key = -1L;
      }
      key = atol (tvalstr);
      snprintf (temp, sizeof (temp), "%s-item-%ld", name, key);
      itemList = llistAlloc (temp, LIST_ORDERED, free);
      continue;
    }

    if (dftype == DFTYPE_LIST) {
      logMsg (LOG_DBG, LOG_LVL_8, "set: list");
      listSetData (nlist, strdup (tkeystr), NULL);
    }
    if (dftype == DFTYPE_KEY_LONG ||
        (dftype == DFTYPE_KEY_VAL && dfkeys != NULL)) {
      logMsg (LOG_DBG, LOG_LVL_8, "use dfkeys");

      long idx = dfkeyBinarySearch (dfkeys, dfkeycount, tkeystr);
      if (idx >= 0) {
        logMsg (LOG_DBG, LOG_LVL_8, "found %s idx: %ld", tkeystr, idx);
        ikey = dfkeys [idx].itemkey;
        vt = dfkeys [idx].valuetype;
        logMsg (LOG_DBG, LOG_LVL_8, "ikey:%ld vt:%d tvalstr:%s", ikey, vt, tvalstr);

        ret.valuetype = VALUE_NONE;
        if (dfkeys [idx].convFunc != NULL) {
          dfkeys [idx].convFunc (tvalstr, &ret);
          vt = ret.valuetype;
          if (vt == VALUE_LONG) {
            lval = ret.u.l;
            logMsg (LOG_DBG, LOG_LVL_8, "converted value: %s to %ld", tvalstr, lval);
          }
        } else {
          if (vt == VALUE_LONG) {
            lval = atol (tvalstr);
            logMsg (LOG_DBG, LOG_LVL_8, "value: %ld", lval);
          }
        }
      } else {
        logMsg (LOG_DBG, LOG_LVL_8, "ERR: Unable to locate key: %s", tkeystr);
      }

      if (lookup != NULL &&
          lookupKey != DATAFILE_NO_LOOKUP &&
          ikey == lookupKey) {
        tlookupkey = tvalstr;
      }

        /* key_long has a key pointing to a key/val list. */
      if (dftype == DFTYPE_KEY_LONG) {
        logMsg (LOG_DBG, LOG_LVL_8, "set: key_long");
        if (vt == VALUE_DATA) {
          llistSetData (itemList, ikey, strdup (tvalstr));
        }
        if (vt == VALUE_LONG) {
          llistSetLong (itemList, ikey, lval);
        }
        if (vt == VALUE_LIST) {
          llistSetList (itemList, ikey, ret.u.list);
        }
      }
        /* a key/val type has a single list keyed by a long (dfkeys is set) */
      if (dftype == DFTYPE_KEY_VAL) {
        logMsg (LOG_DBG, LOG_LVL_8, "set: key_val");
        if (vt == VALUE_DATA) {
          llistSetData (nlist, ikey, strdup (tvalstr));
        }
        if (vt == VALUE_LONG) {
          llistSetLong (nlist, ikey, lval);
        }
        if (vt == VALUE_LIST) {
          llistSetList (nlist, ikey, ret.u.list);
        }
      }
    }
    if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
      logMsg (LOG_DBG, LOG_LVL_8, "set: key_val");
      listSetData (nlist, tkeystr, strdup (tvalstr));
      key = -1L;
    }

    first = 0;
  }

  if (dftype == DFTYPE_KEY_LONG && key >= 0) {
    llistSetList (nlist, key, itemList);
    if (lookup != NULL &&
        lookupKey != DATAFILE_NO_LOOKUP &&
        tlookupkey != NULL) {
      listSetLong (*lookup, tlookupkey, key);
    }
  }

  if (lookupKey != DATAFILE_NO_LOOKUP) {
    listSort (*lookup);
  }

  parseFree (pi);
  logProcEnd (LOG_LVL_8, "datafileParse", "");
  return nlist;
}

long
dfkeyBinarySearch (const datafilekey_t *dfkeys, size_t count, char *key)
{
  long      l = 0;
  long      r = (long) count - 1;
  long      m = 0;
  int       rc;

  while (l <= r) {
    m = l + (r - l) / 2;

    rc = stringCompare (dfkeys [m].name, key);
    if (rc == 0) {
      return m;
    }

    if (rc < 0) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return -1;
}

list_t *
datafileGetList (datafile_t *df)
{
  if (df != NULL) {
    return df->data;
  }
  return NULL;
}

list_t *
datafileGetLookup (datafile_t *df)
{
  if (df != NULL) {
    return df->lookup;
  }
  return NULL;
}

void
datafileSetData (datafile_t *df, void *data)
{
  if (df != NULL) {
    return;
  }
  df->data = data;
  return;
}

int
datafileSave (datafilekey_t *dfkeys, size_t dfkeycount, datafile_t *data)
{
/* ### FIX : TODO */
  datafileBackup (data->fname, 2);
  return 0;
}

void
datafileBackup (char *fname, int count)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];

  for (int i = count; i >= 1; --i) {
    snprintf (nfn, MAXPATHLEN, "%s.bak.%d", fname, i);
    snprintf (ofn, MAXPATHLEN, "%s.bak.%d", fname, i - 1);
    if (i - 1 == 0) {
      strlcpy (ofn, fname, MAXPATHLEN);
    }
    if (fileopExists (ofn)) {
      if ((i - 1) != 0) {
        fileopMove (ofn, nfn);
      } else {
        fileopCopy (ofn, nfn);
      }
    }
  }
  return;
}

/* internal parse routines */

static size_t
parse (parseinfo_t *pi, char *data, parsetype_t parsetype)
{
  char        *tokptr;
  char        *str;
  size_t      dataCounter;

  logProcBegin (LOG_LVL_8, "parse");
  tokptr = NULL;
  if (data == NULL) {
    logProcEnd (LOG_LVL_8, "parse", "null-data");
    return 0;
  }

  if (pi->allocCount < 60) {
    pi->allocCount = 60;
    pi->strdata = realloc (pi->strdata, sizeof (char *) * pi->allocCount);
    assert (pi->strdata != NULL);
  }

  dataCounter = 0;
  str = strtok_r (data, "\n", &tokptr);
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
  logProcEnd (LOG_LVL_8, "parse", "");
  return dataCounter;
}

/* internal datafile routines */

static void
datafileFreeInternal (datafile_t *df)
{
  logProcBegin (LOG_LVL_8, "datafileFreeInternal");
  if (df != NULL) {
    if (df->fname != NULL) {
      free (df->fname);
      df->fname = NULL;
    }
    if (df->lookup != NULL) {
      listFree (df->lookup);
    }
    if (df->data != NULL) {
      switch (df->dftype) {
        case DFTYPE_LIST: {
          listFree (df->data);
          break;
        }
        case DFTYPE_KEY_LONG: {
          llistFree (df->data);
          break;
        }
        case DFTYPE_KEY_VAL:
        {
          list_t    *tlist;

          tlist = df->data;
          if (tlist->keytype == KEY_STR) {
            listFree (df->data);
          } else {
            llistFree (df->data);
          }
          break;
        }
        default: {
          break;
        }
      }
      df->data = NULL;
    }
  }
  logProcEnd (LOG_LVL_8, "datafileFreeInternal", "");
}

