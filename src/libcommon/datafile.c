#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "datafile.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "nlist.h"
#include "slist.h"
#include "tmutil.h"

typedef enum {
  PARSE_SIMPLE,
  PARSE_KEYVALUE
} parsetype_t;

static ssize_t  parse (parseinfo_t *pi, char *data, parsetype_t parsetype);
static void     datafileFreeInternal (datafile_t *df);
static bool     datafileCheckDfkeys (char *name, datafilekey_t *dfkeys, ssize_t dfkeycount);
static FILE *   datafileSavePrep (char *fn, char *tag);
static void     datafileSaveItem (FILE *fh, char *name, dfConvFunc_t convFunc,
    datafileconv_t *conv);
static void     datafileDumpItem (char *tag, char *name, dfConvFunc_t convFunc,
    datafileconv_t *conv);

/* parsing routines */

parseinfo_t *
parseInit (void)
{
  parseinfo_t   *pi;

  logProcBegin (LOG_PROC, "parseInit");
  pi = malloc (sizeof (parseinfo_t));
  assert (pi != NULL);
  pi->strdata = NULL;
  pi->allocCount = 0;
  pi->count = 0;
  logProcEnd (LOG_PROC, "parseInit", "");
  return pi;
}

void
parseFree (parseinfo_t *pi)
{
  logProcBegin (LOG_PROC, "parseFree");
  if (pi != NULL) {
    if (pi->strdata != NULL) {
      free (pi->strdata);
    }
    free (pi);
  }
  logProcEnd (LOG_PROC, "parseFree", "");
}

char **
parseGetData (parseinfo_t *pi)
{
  return pi->strdata;
}

ssize_t
parseSimple (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_SIMPLE);
}

ssize_t
parseKeyValue (parseinfo_t *pi, char *data)
{
  return parse (pi, data, PARSE_KEYVALUE);
}

void
convBoolean (datafileconv_t *conv)
{
  ssize_t   num;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    num = 0;
    if (strcmp (conv->u.str, "on") == 0 ||
        strcmp (conv->u.str, "yes") == 0 ||
        strcmp (conv->u.str, "1") == 0) {
      num = 1;
    }
    conv->u.num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;

    num = conv->u.num;
    conv->u.str = "no";
    if (num) {
      conv->u.str = "yes";
    }
  }
}

void
convTextList (datafileconv_t *conv)
{
  char  *p;

  logProcBegin (LOG_PROC, "convTextList");
  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    char    *tokptr;
    char    *str;
    slist_t *tlist;

    str = conv->u.str;

    conv->valuetype = VALUE_LIST;
    tlist = slistAlloc ("textlist", LIST_ORDERED, NULL);
    assert (conv->u.list != NULL);

    if (conv->u.str != NULL && *conv->u.str) {
      p = strtok_r (str, " ,;", &tokptr);
      while (p != NULL) {
        slistSetStr (tlist, p, NULL);
        p = strtok_r (NULL, " ,;", &tokptr);
      }
    }
    conv->u.list = tlist;
    conv->allocated = true;
  } else if (conv->valuetype == VALUE_LIST) {
    slist_t     *list;
    slistidx_t  iteridx;
    char        tbuff [200];

    conv->valuetype = VALUE_STR;
    list = conv->u.list;

    *tbuff = '\0';
    slistStartIterator (list, &iteridx);
    while ((p = slistIterateKey (list, &iteridx)) != NULL) {
      strlcat (tbuff, p, sizeof (tbuff));
      strlcat (tbuff, " ", sizeof (tbuff));
    }
    stringTrimChar (tbuff, ' ');
    conv->u.str = strdup (tbuff);
    conv->allocated = true;
  }

  logProcEnd (LOG_PROC, "convTextList", "");
}

void
convMS (datafileconv_t *conv)
{
  ssize_t   num;
  char      tbuff [40];

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    char      *p;
    char      *tstr;
    char      *tokstr;

    conv->valuetype = VALUE_NUM;
    tstr = strdup (conv->u.str);
    num = 0;
    p = strtok_r (tstr, ":", &tokstr);
    num += atoi (p) * 60;
    p = strtok_r (NULL, ":", &tokstr);
    num += atoi (p);
    conv->u.num = num;
    free (tstr);
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    tmutilToMS (conv->u.num, tbuff, sizeof (tbuff));
    conv->allocated = true;
    conv->u.str = strdup (tbuff);
  }
}

/* datafile loading routines */

datafile_t *
datafileAlloc (char *name)
{
  datafile_t      *df;

  logProcBegin (LOG_PROC, "datafileAlloc");
  df = malloc (sizeof (datafile_t));
  assert (df != NULL);
  df->name = strdup (name);
  df->fname = NULL;
  df->data = NULL;
  df->lookup = NULL;
  df->dftype = DFTYPE_NONE;
  logProcEnd (LOG_PROC, "datafileAlloc", "");
  return df;
}

datafile_t *
datafileAllocParse (char *name, datafiletype_t dftype, char *fname,
    datafilekey_t *dfkeys, ssize_t dfkeycount, listidx_t lookupKey)
{
  datafile_t      *df;


  logProcBegin (LOG_PROC, "datafileAllocParse");
  logMsg (LOG_DBG, LOG_BASIC, "datafile alloc/parse %s", fname);
  df = datafileAlloc (name);
  if (df != NULL) {
    char *ddata = datafileLoad (df, dftype, fname);
    if (ddata != NULL) {
      df->data = datafileParse (ddata, name, dftype,
          dfkeys, dfkeycount, lookupKey, &df->lookup);
      if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
        slistSort (df->data);
      } else if (dftype == DFTYPE_KEY_VAL) {
        nlistSort (df->data);
      } else if (dftype == DFTYPE_INDIRECT) {
        ilistSort (df->data);
      }
      free (ddata);
    }
  }
  logProcEnd (LOG_PROC, "datafileAllocParse", "");
  return df;
}

void
datafileFree (void *tdf)
{
  datafile_t    *df = (datafile_t *) tdf;

  logProcBegin (LOG_PROC, "datafileFree");
  if (df != NULL) {
    datafileFreeInternal (df);
    if (df->name != NULL) {
      free (df->name);
    }
    free (df);
  }
  logProcEnd (LOG_PROC, "datafileFree", "");
}

char *
datafileLoad (datafile_t *df, datafiletype_t dftype, char *fname)
{
  char    *data;

  logProcBegin (LOG_PROC, "datafileLoad");
  logMsg (LOG_DBG, LOG_BASIC, "datafile load %s", fname);
  df->dftype = dftype;
  if (df->fname == NULL) {
    df->fname = strdup (fname);
    assert (df->fname != NULL);
  }
    /* load the new filename */
  data = filedataReadAll (fname, NULL);
  logProcEnd (LOG_PROC, "datafileLoad", "");
  return data;
}

list_t *
datafileParse (char *data, char *name, datafiletype_t dftype,
    datafilekey_t *dfkeys, ssize_t dfkeycount, slistidx_t lookupKey,
    slist_t **lookup)
{
  list_t        *datalist = NULL;

  datalist = datafileParseMerge (datalist, data, name, dftype,
      dfkeys, dfkeycount, lookupKey, lookup);
  return datalist;
}

list_t *
datafileParseMerge (list_t *datalist, char *data, char *name,
    datafiletype_t dftype, datafilekey_t *dfkeys,
    ssize_t dfkeycount, slistidx_t lookupKey, slist_t **lookup)
{
  char          **strdata = NULL;
  parseinfo_t   *pi = NULL;
  listidx_t     key = -1L;
  ssize_t       dataCount;
  nlist_t       *itemList = NULL;
  nlist_t       *setlist = NULL;
  valuetype_t   vt = 0;
  size_t        inc = 2;
  nlistidx_t    nikey = 0;
  nlistidx_t    ikey = 0;
  ssize_t       lval = 0;
  double        dval = 0.0;
  char          *tkeystr;
  char          *tvalstr;
  char          *tlookupkey = NULL;
  datafileconv_t conv;


  logProcBegin (LOG_PROC, "datafileParseMerge");
  logMsg (LOG_DBG, LOG_DATAFILE, "begin parse %s", name);

  if (dfkeys != NULL) {
    assert (datafileCheckDfkeys (name, dfkeys, dfkeycount));
  }

  pi = parseInit ();
  if (dftype == DFTYPE_LIST) {
    dataCount = parseSimple (pi, data);
  } else {
    dataCount = parseKeyValue (pi, data);
  }
  strdata = parseGetData (pi);

  logMsg (LOG_DBG, LOG_DATAFILE, "dftype: %ld", dftype);
  switch (dftype) {
    case DFTYPE_LIST: {
      inc = 1;
      if (datalist == NULL) {
        datalist = slistAlloc (name, LIST_UNORDERED, NULL);
        slistSetSize (datalist, dataCount);
      } else {
        slistSetSize (datalist, dataCount + slistGetCount (datalist));
      }
      break;
    }
    case DFTYPE_INDIRECT: {
      inc = 2;
      if (datalist == NULL) {
        datalist = ilistAlloc (name, LIST_UNORDERED);
      }
      break;
    }
    case DFTYPE_KEY_VAL: {
      inc = 2;
      if (dfkeys == NULL) {
        if (datalist == NULL) {
          datalist = slistAlloc (name, LIST_UNORDERED, free);
          slistSetSize (datalist, dataCount / 2);
        } else {
          slistSetSize (datalist, dataCount / 2 + slistGetCount (datalist));
        }
        logMsg (LOG_DBG, LOG_DATAFILE, "key_val: list");
      } else {
        if (datalist == NULL) {
          datalist = nlistAlloc (name, LIST_UNORDERED, free);
          nlistSetSize (datalist, dataCount / 2);
        } else {
          nlistSetSize (datalist, dataCount / 2 + nlistGetCount (datalist));
        }
        logMsg (LOG_DBG, LOG_DATAFILE, "key_val: datalist");
      }
      break;
    }
    default: {
      break;
    }
  }

  if (lookupKey != DATAFILE_NO_LOOKUP) {
    char      temp [80];

    logMsg (LOG_DBG, LOG_DATAFILE, "with lookup: key:%ld", lookupKey);
    snprintf (temp, sizeof (temp), "%s-lookup", name);
    *lookup = slistAlloc (temp, LIST_UNORDERED, NULL);
    slistSetSize (*lookup, nlistGetCount (datalist));
  }

  if (dfkeys != NULL) {
    logMsg (LOG_DBG, LOG_DATAFILE, "use dfkeys");
  }

  nikey = 0;
  for (ssize_t i = 0; i < dataCount; i += inc) {
    tkeystr = strdata [i];
    if (inc > 1) {
      tvalstr = strdata [i + 1];
    }

    if (inc == 2 && strcmp (tkeystr, "version") == 0) {
      ssize_t version = atol (tvalstr);
      listSetVersion (datalist, version);
      continue;
    }
    if (strcmp (tkeystr, "count") == 0) {
      if (dftype == DFTYPE_INDIRECT) {
        nlistSetSize (datalist, atol (tvalstr));
      }
      if (lookup != NULL) {
        slistSetSize (*lookup, slistGetCount (datalist));
      }
      continue;
    }

    if (dftype == DFTYPE_INDIRECT &&
        strcmp (tkeystr, "KEY") == 0) {
      char      temp [80];

      /* rather than using the indirect key in the file, renumber the data */
      if (key >= 0) {
        nlistSetList (datalist, nikey, itemList);
        if (lookup != NULL &&
            lookupKey != DATAFILE_NO_LOOKUP &&
            tlookupkey != NULL) {
          logMsg (LOG_DBG, LOG_DATAFILE, "lookup: set %s to %ld", tlookupkey, key);
          slistSetNum (*lookup, tlookupkey, nikey);
        }
        key = -1L;
        nikey++;
      }
      key = atol (tvalstr);
      snprintf (temp, sizeof (temp), "%s-item-%zd", name, nikey);
      itemList = nlistAlloc (temp, LIST_ORDERED, free);
      continue;
    }

    if (dftype == DFTYPE_LIST) {
      logMsg (LOG_DBG, LOG_DATAFILE, "set: list %s", tkeystr);
      slistSetData (datalist, tkeystr, NULL);
    }
    if (dftype == DFTYPE_INDIRECT ||
        (dftype == DFTYPE_KEY_VAL && dfkeys != NULL)) {
      listidx_t idx = dfkeyBinarySearch (dfkeys, dfkeycount, tkeystr);
      if (idx >= 0) {
        logMsg (LOG_DBG, LOG_DATAFILE, "found %s idx: %ld", tkeystr, idx);
        ikey = dfkeys [idx].itemkey;
        vt = dfkeys [idx].valuetype;
        logMsg (LOG_DBG, LOG_DATAFILE, "ikey:%ld vt:%d tvalstr:%s", ikey, vt, tvalstr);

        conv.valuetype = VALUE_NONE;
        if (dfkeys [idx].convFunc != NULL) {
          conv.allocated = false;
          conv.valuetype = VALUE_STR;
          conv.u.str = tvalstr;
          dfkeys [idx].convFunc (&conv);

          vt = conv.valuetype;
          if (vt == VALUE_NUM) {
            if (conv.u.num == LIST_VALUE_INVALID && dfkeys [idx].backupKey != -1) {
              logMsg (LOG_DBG, LOG_DATAFILE, "invalid value; backup key %ld set data to %s", dfkeys [idx].backupKey, tvalstr);
              if (dftype == DFTYPE_INDIRECT) {
                nlistSetStr (itemList, dfkeys [idx].backupKey, tvalstr);
              }
              if (dftype == DFTYPE_KEY_VAL) {
                nlistSetStr (datalist, dfkeys [idx].backupKey, tvalstr);
              }
            } else {
              lval = conv.u.num;
              logMsg (LOG_DBG, LOG_DATAFILE, "converted value: %s to %ld", tvalstr, lval);
            }
          }
        } else {
          if (vt == VALUE_NUM) {
            if (strcmp (tvalstr, "") == 0) {
              lval = LIST_VALUE_INVALID;
            } else {
              lval = atol (tvalstr);
            }
            logMsg (LOG_DBG, LOG_DATAFILE, "value: %ld", lval);
          }
          if (vt == VALUE_DOUBLE) {
            if (strcmp (tvalstr, "") == 0) {
              dval = LIST_DOUBLE_INVALID;
            } else {
              dval = atof (tvalstr);
            }
            logMsg (LOG_DBG, LOG_DATAFILE, "value: %.2f", dval);
          }
        }
      } else {
        logMsg (LOG_DBG, LOG_DATAFILE, "ERR: Unable to locate key: %s", tkeystr);
        continue;
      }

      if (lookup != NULL &&
          lookupKey != DATAFILE_NO_LOOKUP &&
          ikey == lookupKey) {
        tlookupkey = tvalstr;
      }

      logMsg (LOG_DBG, LOG_DATAFILE, "set: dftype:%ld vt:%d", dftype, vt);

      if (dftype == DFTYPE_INDIRECT) {
        setlist = itemList;
      }
      if (dftype == DFTYPE_KEY_VAL) {
        setlist = datalist;
      }

      /* use the 'setlist' temporary variable to hold the correct list var */
      if (vt == VALUE_STR) {
        nlistSetStr (setlist, ikey, tvalstr);
      }
      if (vt == VALUE_DATA) {
        nlistSetData (setlist, ikey, tvalstr);
      }
      if (vt == VALUE_NUM) {
        nlistSetNum (setlist, ikey, lval);
      }
      if (vt == VALUE_DOUBLE) {
        nlistSetDouble (setlist, ikey, dval);
      }
      if (vt == VALUE_LIST) {
        nlistSetList (setlist, ikey, conv.u.list);
      }

    }

    if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
      logMsg (LOG_DBG, LOG_DATAFILE, "set: key_val");
      slistSetStr (datalist, tkeystr, tvalstr);
      key = -1L;
    }
  }

  if (dftype == DFTYPE_INDIRECT && key >= 0) {
    nlistSetList (datalist, key, itemList);
    if (lookup != NULL &&
        lookupKey != DATAFILE_NO_LOOKUP &&
        tlookupkey != NULL) {
      slistSetNum (*lookup, tlookupkey, key);
    }
  }

  if (lookupKey != DATAFILE_NO_LOOKUP) {
    slistidx_t    iteridx;

    slistSort (*lookup);
    slistStartIterator (*lookup, &iteridx);
    while ((tkeystr = slistIterateKey (*lookup, &iteridx)) != NULL) {
      lval = slistGetNum (*lookup, tkeystr);
      logMsg (LOG_DBG, LOG_DATAFILE | LOG_BASIC,
          "%s: %s / %zd", name, tkeystr, lval);
    }
  }

  parseFree (pi);
  logProcEnd (LOG_PROC, "datafileParseMerge", "");
  return datalist;
}

void
datafileSaveKeyVal (char *tag, char *fn, datafilekey_t *dfkeys,
    ssize_t dfkeycount, nlist_t *list)
{
  FILE            *fh;
  datafileconv_t  conv;
  valuetype_t     vt;

  fh = datafileSavePrep (fn, tag);
  if (fh == NULL) {
    return;
  }

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    vt = dfkeys [i].valuetype;
    conv.allocated = false;
    conv.valuetype = vt;

    /* load the data value into the conv structure so that retrieval is */
    /* the same for both non-converted and converted values */
    if (vt == VALUE_NUM) {
      conv.u.num = nlistGetNum (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_STR) {
      conv.u.str = nlistGetStr (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_LIST) {
      conv.u.list = nlistGetList (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_DOUBLE) {
      conv.u.dval = nlistGetDouble (list, dfkeys [i].itemkey);
    }

    datafileSaveItem (fh, dfkeys [i].name, dfkeys [i].convFunc, &conv);
  }
  fclose (fh);
}

void
datafileSaveIndirect (char *tag, char *fn, datafilekey_t *dfkeys,
    ssize_t dfkeycount, ilist_t *list)
{
  FILE            *fh;
  datafileconv_t  conv;
  valuetype_t     vt;
  ilistidx_t      count;
  ilistidx_t      iteridx;
  ilistidx_t      key;

  fh = datafileSavePrep (fn, tag);
  if (fh == NULL) {
    return;
  }

  count = ilistGetCount (list);
  ilistStartIterator (list, &iteridx);

  fprintf (fh, "version\n..1\n");
  fprintf (fh, "count\n..%zd\n", count);

  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    conv.allocated = false;
    conv.valuetype = VALUE_NUM;
    /* on save, re-order the keys */
    conv.u.num = count++;
    datafileSaveItem (fh, "KEY", NULL, &conv);

    for (ssize_t i = 0; i < dfkeycount; ++i) {
      vt = dfkeys [i].valuetype;
      conv.valuetype = vt;

      /* load the data value into the conv structure so that retrieval is */
      /* the same for both non-converted and converted values */
      if (vt == VALUE_NUM) {
        conv.u.num = ilistGetNum (list, key, dfkeys [i].itemkey);
      }
      if (vt == VALUE_STR) {
        conv.u.str = ilistGetStr (list, key, dfkeys [i].itemkey);
      }
      if (vt == VALUE_LIST) {
        conv.u.list = ilistGetList (list, key, dfkeys [i].itemkey);
      }
      if (vt == VALUE_DOUBLE) {
        conv.u.dval = ilistGetDouble (list, key, dfkeys [i].itemkey);
      }

      datafileSaveItem (fh, dfkeys [i].name, dfkeys [i].convFunc, &conv);
    }
  }
  fclose (fh);
}

void
datafileSaveList (char *tag, char *fn, slist_t *list)
{
  FILE            *fh;
  slistidx_t      iteridx;
  char            *str;

  fh = datafileSavePrep (fn, tag);
  if (fh == NULL) {
    return;
  }

  slistStartIterator (list, &iteridx);

  while ((str = slistIterateKey (list, &iteridx)) != NULL) {
    fprintf (fh, "%s\n", str);
  }
  fclose (fh);
}

listidx_t
dfkeyBinarySearch (const datafilekey_t *dfkeys, ssize_t count, char *key)
{
  listidx_t     l = 0;
  listidx_t     r = count - 1;
  listidx_t     m = 0;
  int           rc;

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

void
datafileBackup (char *fname, int count)
{
  char      ofn [MAXPATHLEN];
  char      nfn [MAXPATHLEN];

  for (int i = count; i >= 1; --i) {
    snprintf (nfn, sizeof (nfn), "%s.bak.%d", fname, i);
    snprintf (ofn, sizeof (ofn), "%s.bak.%d", fname, i - 1);
    if (i - 1 == 0) {
      strlcpy (ofn, fname, sizeof (ofn));
    }
    if (fileopFileExists (ofn)) {
      if ((i - 1) != 0) {
        filemanipMove (ofn, nfn);
      } else {
        filemanipCopy (ofn, nfn);
      }
    }
  }
  return;
}

void
datafileDumpKeyVal (char *tag, datafilekey_t *dfkeys,
    ssize_t dfkeycount, nlist_t *list)
{
  datafileconv_t  conv;
  valuetype_t     vt;

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    vt = dfkeys [i].valuetype;
    conv.allocated = false;
    conv.valuetype = vt;

    /* load the data value into the conv structure so that retrieval is */
    /* the same for both non-converted and converted values */
    if (vt == VALUE_NUM) {
      conv.u.num = nlistGetNum (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_STR) {
      conv.u.str = nlistGetStr (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_LIST) {
      conv.u.list = nlistGetList (list, dfkeys [i].itemkey);
    }
    if (vt == VALUE_DOUBLE) {
      conv.u.dval = nlistGetDouble (list, dfkeys [i].itemkey);
    }

    datafileDumpItem (tag, dfkeys [i].name, dfkeys [i].convFunc, &conv);
  }
}

/* internal parse routines */

static ssize_t
parse (parseinfo_t *pi, char *data, parsetype_t parsetype)
{
  char        *tokptr;
  char        *str;
  ssize_t     dataCounter;

  logProcBegin (LOG_PROC, "parse");
  tokptr = NULL;
  if (data == NULL) {
    logProcEnd (LOG_PROC, "parse", "null-data");
    return 0;
  }

  if (pi->allocCount < 60) {
    pi->allocCount = 60;
    pi->strdata = realloc (pi->strdata,
        sizeof (char *) * (size_t) pi->allocCount);
    assert (pi->strdata != NULL);
  }

  dataCounter = 0;
  str = strtok_r (data, "\r\n", &tokptr);
  while (str != NULL) {
    if (*str == '#') {
      str = strtok_r (NULL, "\r\n", &tokptr);
      continue;
    }

    if (dataCounter >= pi->allocCount) {
      pi->allocCount += 10;
      pi->strdata = realloc (pi->strdata,
          sizeof (char *) * (size_t) pi->allocCount);
      assert (pi->strdata != NULL);
    }
    if (parsetype == PARSE_KEYVALUE && dataCounter % 2 == 1) {
      /* value data is indented */
      str += 2;
    }
    pi->strdata [dataCounter] = str;
    ++dataCounter;
    str = strtok_r (NULL, "\r\n", &tokptr);
  }
  pi->count = dataCounter;
  logProcEnd (LOG_PROC, "parse", "");
  return dataCounter;
}

/* internal datafile routines */

static void
datafileFreeInternal (datafile_t *df)
{
  logProcBegin (LOG_PROC, "datafileFreeInternal");
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
        case DFTYPE_INDIRECT: {
          nlistFree (df->data);
          break;
        }
        case DFTYPE_KEY_VAL:
        {
          list_t    *tlist;

          tlist = df->data;
          if (tlist->keytype == LIST_KEY_STR) {
            listFree (df->data);
          } else {
            nlistFree (df->data);
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
  logProcEnd (LOG_PROC, "datafileFreeInternal", "");
}

static bool
datafileCheckDfkeys (char *name, datafilekey_t *dfkeys, ssize_t dfkeycount)
{
  char  *last = "";
  bool  ok = true;

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    if (strcmp (dfkeys [i].name, last) <= 0) {
      fprintf (stderr, "datafile: %s dfkey out of order: %s\n", name, dfkeys [i].name);
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: datafile: %s dfkey out of order: %s", name, dfkeys [i].name);
      ok = false;
    }
    last = dfkeys [i].name;
  }
  return ok;
}

static FILE *
datafileSavePrep (char *fn, char *tag)
{
  FILE    *fh;
  char    tbuff [100];


  datafileBackup (fn, 1);
  fh = fileopOpen (fn, "w");

  if (fh == NULL) {
    return NULL;
  }

  fprintf (fh, "# %s\n", tag);
  fprintf (fh, "# %s ", tmutilDstamp (tbuff, sizeof (tbuff)));
  fprintf (fh, "%s\n", tmutilTstamp (tbuff, sizeof (tbuff)));
  return fh;
}

static void
datafileSaveItem (FILE *fh, char *name, dfConvFunc_t convFunc,
    datafileconv_t *conv)
{
  valuetype_t     vt;

  vt = conv->valuetype;
  if (convFunc != NULL) {
    convFunc (conv);
    vt = conv->valuetype;
  }

  fprintf (fh, "%s\n", name);
  if (vt == VALUE_NUM) {
    if (conv->u.num != LIST_VALUE_INVALID) {
      fprintf (fh, "..%zd\n", conv->u.num);
    } else {
      fprintf (fh, "..\n");
    }
  }
  if (vt == VALUE_DOUBLE) {
    if (conv->u.dval != LIST_DOUBLE_INVALID) {
      fprintf (fh, "..%.2f\n", conv->u.dval);
    } else {
      fprintf (fh, "..\n");
    }
  }
  if (vt == VALUE_STR) {
    if (conv->u.str != NULL) {
      fprintf (fh, "..%s\n", conv->u.str);
    } else {
      fprintf (fh, "..\n");
    }
    if (conv->allocated) {
      free (conv->u.str);
    }
  }
}

static void
datafileDumpItem (char *tag, char *name, dfConvFunc_t convFunc,
    datafileconv_t *conv)
{
  valuetype_t     vt;

  vt = conv->valuetype;
  if (convFunc != NULL) {
    convFunc (conv);
    vt = conv->valuetype;
  }

  fprintf (stdout, "%2s: %-20s ", tag, name);
  if (vt == VALUE_NUM) {
    if (conv->u.num != LIST_VALUE_INVALID) {
      fprintf (stdout, "%zd", conv->u.num);
    }
    fprintf (stdout, "\n");
  }
  if (vt == VALUE_DOUBLE) {
    if (conv->u.dval != LIST_DOUBLE_INVALID) {
      fprintf (stdout, "%.2f", conv->u.dval);
    }
    fprintf (stdout, "\n");
  }
  if (vt == VALUE_STR) {
    if (conv->u.str != NULL) {
      fprintf (stdout, "%s", conv->u.str);
    }
    fprintf (stdout, "\n");
    if (conv->allocated) {
      free (conv->u.str);
    }
  }
}

