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
static long   dfkeyBinarySearch (const datafilekey_t *dfkeys,
                  size_t count, char *key);

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
  ret->u.list = listAlloc ("textlist", sizeof (char *), istringCompare, free);
  if (data != NULL) {
    p = strtok_r (data, " ,", &tokptr);
    while (p != NULL) {
      listSet (ret->u.list, strdup (p));
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
  df->dftype = DFTYPE_NONE;
  logProcEnd (LOG_LVL_8, "datafileAlloc", "");
  return df;
}

datafile_t *
datafileAllocParse (char *name, datafiletype_t dftype, char *fname,
    datafilekey_t *dfkeys, size_t dfkeycount)
{
  logProcBegin (LOG_LVL_5, "datafileAllocParse");
  logMsg (LOG_DBG, LOG_LVL_5, "alloc/parse %s", fname);
  datafile_t *df = datafileAlloc (name);
  if (df != NULL) {
    char *ddata = datafileLoad (df, dftype, fname);
    if (ddata != NULL) {
      df->data = datafileParse (ddata, name, dftype, dfkeys, dfkeycount);
      if (dftype == DFTYPE_KEY_STRING ||
          (dftype == DFTYPE_KEY_VAL && dfkeys == NULL)) {
        slistSort (df->data);
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
    datafilekey_t *dfkeys, size_t dfkeycount)
{
  list_t        *nlist = NULL;

  nlist = datafileParseMerge (nlist, data, name, dftype, dfkeys, dfkeycount);
  return nlist;
}

list_t *
datafileParseMerge (list_t *nlist, char *data, char *name,
    datafiletype_t dftype, datafilekey_t *dfkeys, size_t dfkeycount)
{
  char          **strdata = NULL;
  parseinfo_t   *pi = NULL;
  long          key = -1L;
  size_t        dataCount;
  list_t        *itemList = NULL;
  valuetype_t   vt = 0;
  size_t        inc = 2;
  char          *keyString = NULL;
  int           first = 1;
  long          ikey = 0;
  long          lval = 0;
  char          *ikeystr = NULL;
  char          *tkeystr;
  char          *tvalstr;
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
        nlist = listAlloc (name, sizeof (char *), NULL, free);
        listSetSize (nlist, dataCount);
      } else {
        listSetSize (nlist, dataCount + nlist->count);
      }
      break;
    }
    case DFTYPE_KEY_STRING: {
      inc = 2;
      if (nlist == NULL) {
        nlist = slistAlloc (name, LIST_UNORDERED, istringCompare, free, listFree);
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
          nlist = slistAlloc (name, LIST_UNORDERED, istringCompare, free, free);
          slistSetSize (nlist, dataCount / 2);
        } else {
          slistSetSize (nlist, dataCount / 2 + nlist->count);
        }
        logMsg (LOG_DBG, LOG_LVL_8, "key_val: slist");
      } else {
        if (nlist == NULL) {
          nlist = llistAlloc (name, LIST_UNORDERED, free);
          llistSetSize (nlist, dataCount / 2);
        } else {
          llistSetSize (nlist, dataCount / 2 + nlist->count);
        }
        logMsg (LOG_DBG, LOG_LVL_8, "key_val: llist");
      }
      break;
    }
    default: {
      break;
    }
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
      if (dftype == DFTYPE_KEY_STRING) {
        slistSetSize (nlist, (size_t) atol (tvalstr));
      }
      continue;
    }

    if (dftype == DFTYPE_KEY_LONG &&
        strcmp (tkeystr, "KEY") == 0) {
      if (key >= 0) {
        llistSetList (nlist, key, itemList);
        key = -1L;
      }
      key = atol (tvalstr);
      itemList = llistAlloc ("item", LIST_ORDERED, free);
      continue;
    }
    if (! first &&
        dftype == DFTYPE_KEY_STRING &&
        strcmp (keyString, tkeystr) == 0) {
      if (ikeystr != NULL) {
        slistSetList (nlist, ikeystr, itemList);
        ikeystr = NULL;
      }
      keyString = tkeystr;
      itemList = slistAlloc ("item", LIST_ORDERED, istringCompare, free, free);
      continue;
    }

    if (dftype == DFTYPE_LIST) {
      logMsg (LOG_DBG, LOG_LVL_8, "set: list");
      listSet (nlist, strdup (tkeystr));
    }
    if (dftype == DFTYPE_KEY_LONG ||
        (dftype == DFTYPE_KEY_VAL && dfkeys != NULL) ||
        (dftype == DFTYPE_KEY_STRING && dfkeys != NULL)) {
      logMsg (LOG_DBG, LOG_LVL_8, "use dfkeys");
      if (first && dftype == DFTYPE_KEY_STRING) {
        keyString = tkeystr;
        ikeystr = tvalstr;
      }

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

        /* key_string and key_long have a key pointing to a key/val list. */
        /* a list of data items, each of which is a list                  */
      if (dftype == DFTYPE_KEY_STRING) {
        logMsg (LOG_DBG, LOG_LVL_8, "set: key_string");
        if (vt == VALUE_DATA) {
          slistSetData (itemList, tkeystr, strdup (tvalstr));
        }
        if (vt == VALUE_LONG) {
          slistSetLong (itemList, tkeystr, lval);
        }
        if (vt == VALUE_LIST) {
          slistSetList (itemList, tkeystr, ret.u.list);
        }
      }
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
      slistSetData (nlist, tkeystr, strdup (tvalstr));
      key = -1L;
    }

    first = 0;
  }

  if (dftype == DFTYPE_KEY_LONG && key >= 0) {
    llistSetList (nlist, key, itemList);
  }
  if (dftype == DFTYPE_KEY_STRING && ikeystr != NULL) {
    slistSetList (nlist, ikeystr, itemList);
  }

  parseFree (pi);
  logProcEnd (LOG_LVL_8, "datafileParse", "");
  return nlist;
}

list_t *
datafileGetData (datafile_t *df)
{
  if (df != NULL) {
    return df->data;
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
    if (df->data != NULL) {
      switch (df->dftype) {
        case DFTYPE_LIST: {
          listFree (df->data);
          break;
        }
        case DFTYPE_KEY_STRING: {
          slistFree (df->data);
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
            slistFree (df->data);
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

static long
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

