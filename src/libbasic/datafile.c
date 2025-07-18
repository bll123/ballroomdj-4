/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "datafile.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "tmutil.h"

#include "tagdef.h" // ### REMOVE

typedef enum {
  PARSE_SIMPLE,
  PARSE_KEYVALUE
} parsetype_t;

typedef struct parseinfo {
  char        **strdata;
  ssize_t     allocCount;
  ssize_t     count;
} parseinfo_t;

typedef struct datafile {
  char            *tag;
  char            *fname;
  datafiletype_t  dftype;
  datafilekey_t   *dfkeys;
  int             distvers;
  int             dfkeycount;
  list_t          *data;
} datafile_t;

static const char * const DF_VERSION_STR      = "version";
static const char * const DF_VERSION_DIST_STR = "# version ";
static const char * const DF_VERSION_FMT      = "# version %d";

static ssize_t  parse (parseinfo_t *pi, char *data, parsetype_t parsetype, int *vers);
static void     datafileFreeData (datafile_t *df);
static list_t   *datafileParseMerge (list_t *nlist, char *data, const char *name, datafiletype_t dftype, datafilekey_t *dfkeys, int dfkeycount, int offset, int *distvers);
static bool     datafileCheckDfkeys (const char *tag, datafilekey_t *dfkeys, int dfkeycount);
static void     datafileSaveKeyVal (datafile_t *df, const char *fn, nlist_t *list, int offset, int distvers);
static void     datafileSaveIndirect (datafile_t *df, const char *fn, nlist_t *list, int distvers);
static void     datafileSaveList (datafile_t *df, const char *fn, nlist_t *list, int distvers);
static FILE *   datafileSavePrep (const char *fn, const char *tag, int distvers);
static char     *datafileSaveItem (char *buff, size_t sz, char *currp, const char *name, dfConvFunc_t convFunc, datafileconv_t *conv, int flags);
static void     datafileLoadConv (datafilekey_t *dfkey, nlist_t *list, datafileconv_t *conv, int offset);
static void     datafileConvertValue (char *buff, size_t sz, dfConvFunc_t convFunc, datafileconv_t *conv);
static void     datafileDumpItem (const char *tag, const char *name, dfConvFunc_t convFunc, datafileconv_t *conv);

/* parsing routines */

parseinfo_t *
parseInit (void)
{
  parseinfo_t   *pi;

  logProcBegin ();
  pi = mdmalloc (sizeof (parseinfo_t));
  pi->strdata = NULL;
  pi->allocCount = 0;
  pi->count = 0;
  logProcEnd ("");
  return pi;
}

void
parseFree (parseinfo_t *pi)
{
  logProcBegin ();
  if (pi != NULL) {
    dataFree (pi->strdata);
    mdfree (pi);
  }
  logProcEnd ("");
}

char **
parseGetData (parseinfo_t *pi)
{
  return pi->strdata;
}

ssize_t
parseSimple (parseinfo_t *pi, char *data, int *distvers)
{
  return parse (pi, data, PARSE_SIMPLE, distvers);
}

ssize_t
parseKeyValue (parseinfo_t *pi, char *data, int *distvers)
{
  return parse (pi, data, PARSE_KEYVALUE, distvers);
}

void
convBoolean (datafileconv_t *conv)
{
  ssize_t   num;

  if (conv->invt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    num = 0;
    if (strcmp (conv->str, "on") == 0 ||
        strcmp (conv->str, "yes") == 0 ||
        strcmp (conv->str, "true") == 0 ||
        strcmp (conv->str, "1") == 0) {
      num = 1;
    }
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    conv->outvt = VALUE_STR;

    num = conv->num;
    conv->str = "no";
    if (num) {
      conv->str = "yes";
    }
  }
}

void
convTextList (datafileconv_t *conv)
{
  char  *p;

  logProcBegin ();
  if (conv->invt == VALUE_STR) {
    char    *tokptr;
    char    *str;
    slist_t *tlist;

    str = mdstrdup (conv->str);

    conv->outvt = VALUE_LIST;
    tlist = slistAlloc ("textlist", LIST_UNORDERED, NULL);

    if (conv->str != NULL && *conv->str) {
      p = strtok_r (str, " ,;", &tokptr);
      while (p != NULL) {
        slistSetStr (tlist, p, NULL);
        p = strtok_r (NULL, " ,;", &tokptr);
      }
    }
    conv->list = tlist;
    mdfree (str);
  } else if (conv->invt == VALUE_LIST) {
    slist_t     *list;
    slistidx_t  iteridx;
    char        tbuff [300];
    const char  *key;
    char        *p;
    char        *end = tbuff + sizeof (tbuff);
    bool        first = true;

    conv->outvt = VALUE_STRVAL;
    list = conv->list;

    *tbuff = '\0';
    slistStartIterator (list, &iteridx);
    p = tbuff;
    while ((key = slistIterateKey (list, &iteridx)) != NULL) {
      if (! first) {
        p = stpecpy (p, end, " ");
      }
      p = stpecpy (p, end, key);
      first = false;
    }
    conv->strval = mdstrdup (tbuff);
  }

  logProcEnd ("");
}

/* datafile loading routines */

datafile_t *
datafileAlloc (const char *tag, datafiletype_t dftype, const char *fname, datafilekey_t *dfkeys, int dfkeycount)
{
  datafile_t      *df;

  logProcBegin ();
  df = mdmalloc (sizeof (datafile_t));
  df->tag = mdstrdup (tag);
  df->fname = mdstrdup (fname);
  df->dftype = dftype;
  df->dfkeys = dfkeys;
  df->dfkeycount = dfkeycount;
  df->distvers = 1;
  df->data = NULL;
  logProcEnd ("");
  return df;
}

datafile_t *
datafileAllocParse (const char *tag, datafiletype_t dftype, const char *fname,
    datafilekey_t *dfkeys, int dfkeycount, int offset, datafile_t *mergedf)
{
  datafile_t      *df;
  int             distvers = 1;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_DATAFILE, "datafile alloc/parse %s", fname);
  df = datafileAlloc (tag, dftype, fname, dfkeys, dfkeycount);
  if (df != NULL) {
    char *ddata;

    ddata = datafileLoad (df, dftype, fname);

    if (ddata != NULL) {
      void    *tlist;

      tlist = df->data;
      if (mergedf != NULL) {
        tlist = datafileGetList (mergedf);
      }

      df->data = datafileParseMerge (tlist, ddata, tag, dftype,
          dfkeys, dfkeycount, offset, &distvers);
      df->distvers = distvers;
      if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
        slistSort (df->data);
      } else if (dftype == DFTYPE_KEY_VAL) {
        nlistSort (df->data);
      } else if (dftype == DFTYPE_INDIRECT) {
        ilistSort (df->data);
      }
      mdfree (ddata);
      if (mergedf != NULL) {
        mergedf->data = df->data;
        /* so that the data does not get freed twice */
        df->data = NULL;
      }
    }
  }
  logProcEnd ("");
  return df;
}

void
datafileFree (void *tdf)
{
  datafile_t    *df = (datafile_t *) tdf;

  logProcBegin ();
  if (df != NULL) {
    logMsg (LOG_DBG, LOG_DATAFILE, "datafile free %s", df->fname);
    datafileFreeData (df);
    dataFree (df->tag);
    dataFree (df->fname);
    mdfree (df);
  }
  logProcEnd ("");
}

/* datafileLoad is also used by bdjopt to load the data before */
/* a parse-merge */
char *
datafileLoad (datafile_t *df, datafiletype_t dftype, const char *fname)
{
  char    *data;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_DATAFILE, "datafile load %s", fname);
  df->dftype = dftype;
  if (df->fname == NULL) {
    df->fname = mdstrdup (fname);
  }

  /* load the new filename */
  data = filedataReadAll (fname, NULL);
  logProcEnd ("");
  return data;
}

list_t *
datafileParse (char *data, const char *name, datafiletype_t dftype,
    datafilekey_t *dfkeys, int dfkeycount, int *distvers)
{
  list_t        *datalist = NULL;

  datalist = datafileParseMerge (datalist, data, name, dftype,
      dfkeys, dfkeycount, 0, distvers);
  return datalist;
}

/* save the key-value data to a list */
slist_t *
datafileSaveKeyValList (const char *tag,
    datafilekey_t *dfkeys, int dfkeycount, nlist_t *list)
{
  datafileconv_t  conv;
  slist_t         *slist;
  char            tbuff [1024];

  slist = slistAlloc (tag, LIST_ORDERED, NULL);

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    if (dfkeys [i].writeFlag == DF_NO_WRITE) {
      continue;
    }
    datafileLoadConv (&dfkeys [i], list, &conv, 0);
    datafileConvertValue (tbuff, sizeof (tbuff), dfkeys [i].convFunc, &conv);
    slistSetStr (slist, dfkeys [i].name, tbuff);
  }

  return slist;
}

/* save the key-value data to a buffer */
size_t
datafileSaveKeyValBuffer (char *buff, size_t sz, const char *tag,
    datafilekey_t *dfkeys, int dfkeycount, nlist_t *list, int offset, int flags)
{
  datafileconv_t  conv;
  char            *currp;

  *buff = '\0';
  currp = buff;

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    if (dfkeys [i].writeFlag == DF_NO_WRITE) {
      continue;
    }
    datafileLoadConv (&dfkeys [i], list, &conv, offset);
    currp = datafileSaveItem (buff, sz, currp, dfkeys [i].name, dfkeys [i].convFunc, &conv, flags);
  }

  return currp - buff;
}

void
datafileSave (datafile_t *df, const char *fname, nlist_t *list,
    int offset, int distvers)
{
  if (df == NULL) {
    return;
  }

  if (fname == NULL) {
    fname = df->fname;
  }
  logMsg (LOG_DBG, LOG_DATAFILE, "save type %d to %s", df->dftype, fname);

  if (df->dftype == DFTYPE_LIST) {
    datafileSaveList (df, fname, list, distvers);
  }
  if (df->dftype == DFTYPE_KEY_VAL) {
    datafileSaveKeyVal (df, fname, list, offset, distvers);
  }
  if (df->dftype == DFTYPE_INDIRECT) {
    datafileSaveIndirect (df, fname, list, distvers);
  }
}

listidx_t
dfkeyBinarySearch (const datafilekey_t *dfkeys, int count, const char *key)
{
  listidx_t     l = 0;
  listidx_t     r = count - 1;
  listidx_t     m = 0;
  int           rc;

  while (l <= r) {
    m = l + (r - l) / 2;

    rc = strcmp (dfkeys [m].name, key);
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

void
datafileDumpKeyVal (const char *tag, datafilekey_t *dfkeys,
    int dfkeycount, nlist_t *list, int offset)
{
  datafileconv_t  conv;

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    datafileLoadConv (&dfkeys [i], list, &conv, offset);
    datafileDumpItem (tag, dfkeys [i].name, dfkeys [i].convFunc, &conv);
  }
}

int
datafileDistVersion (datafile_t *df)
{
  if (df == NULL) {
    return 0;
  }

  return df->distvers;
}

int
datafileReadDistVersion (const char *fname)
{
  FILE    *fh;
  char    line [MAXPATHLEN];
  int     distvers = 1;
  bool    done;

  fh = fileopOpen (fname, "r");
  if (fh == NULL) {
    return distvers;
  }

  done = false;
  while (! done) {
    (void) ! fgets (line, sizeof (line), fh);
    if (strncmp (line, DF_VERSION_DIST_STR, strlen (DF_VERSION_DIST_STR)) == 0) {
      sscanf (line, DF_VERSION_FMT, &distvers);
      done = true;
    }
    if (*line != '#') {
      /* assume, that after then first block of comments, that the */
      /* dist version does not exist */
      done = true;
    }
  }

  mdextfclose (fh);
  fclose (fh);
  return distvers;
}

/* debug / informational */

/* for testing */
datafiletype_t
datafileGetType (datafile_t *df) /* TESTING */
{
  return df->dftype;
}

/* for testing */
char *
datafileGetFname (datafile_t *df) /* TESTING */
{
  return df->fname;
}

/* for testing */
list_t *
datafileGetData (datafile_t *df) /* TESTING */
{
  return df->data;
}

listidx_t
parseGetAllocCount (parseinfo_t *pi)  /* TESTING */
{
  return pi->allocCount;
}

/* internal parse routines */

static ssize_t
parse (parseinfo_t *pi, char *data, parsetype_t parsetype, int *distvers)
{
  char        *tokptr;
  char        *str;
  ssize_t     dataCounter;

  logProcBegin ();
  tokptr = NULL;
  if (data == NULL) {
    logProcEnd ("null-data");
    return 0;
  }

  if (pi->allocCount < 60) {
    pi->allocCount = 60;
    pi->strdata = mdrealloc (pi->strdata,
        sizeof (char *) * (size_t) pi->allocCount);
  }

  dataCounter = 0;
  str = strtok_r (data, "\r\n", &tokptr);
  while (str != NULL) {
    if (*str == '#') {
      if (distvers != NULL &&
          strncmp (str, DF_VERSION_DIST_STR, strlen (DF_VERSION_DIST_STR)) == 0) {
        sscanf (str, DF_VERSION_FMT, distvers);
      }
      str = strtok_r (NULL, "\r\n", &tokptr);
      continue;
    }

    if (dataCounter >= pi->allocCount) {
      pi->allocCount += 10;
      pi->strdata = mdrealloc (pi->strdata,
          sizeof (char *) * (size_t) pi->allocCount);
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
  logProcEnd ("");
  return dataCounter;
}

/* internal datafile routines */

static void
datafileFreeData (datafile_t *df)
{
  logProcBegin ();
  if (df != NULL) {
    if (df->data != NULL) {
      switch (df->dftype) {
        case DFTYPE_LIST: {
          slistFree (df->data);
          break;
        }
        case DFTYPE_INDIRECT: {
          ilistFree (df->data);
          break;
        }
        case DFTYPE_KEY_VAL:
        {
          if (df->dfkeys == NULL) {
            /* simple list */
            slistFree (df->data);
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
  logProcEnd ("");
}


static list_t *
datafileParseMerge (list_t *datalist, char *data, const char *name,
    datafiletype_t dftype, datafilekey_t *dfkeys,
    int dfkeycount, int offset, int *distvers)
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
  nlistidx_t    isz = 0;
  nlistidx_t    ikey = 0;
  listnum_t     lval = 0;
  double        dval = 0.0;
  char          *tkeystr;
  char          *tvalstr = NULL;
  datafileconv_t conv;


  logProcBegin ();
  logMsg (LOG_DBG, LOG_DATAFILE, "begin parse %s", name);

  if (dfkeys != NULL) {
    if (datafileCheckDfkeys (name, dfkeys, dfkeycount) != true) {
      fprintf (stderr, "ERR: df-keys are not valid\n");
      return NULL;
    }
  }

  pi = parseInit ();
  if (dftype == DFTYPE_LIST) {
    dataCount = parseSimple (pi, data, distvers);
  } else {
    dataCount = parseKeyValue (pi, data, distvers);
  }
  strdata = parseGetData (pi);

  logMsg (LOG_DBG, LOG_DATAFILE, "dftype: %d", dftype);
  switch (dftype) {
    case DFTYPE_LIST: {
      inc = 1;
      if (datalist == NULL) {
        datalist = slistAlloc (name, LIST_UNORDERED, NULL);
        slistSetSize (datalist, dataCount);
      } else {
        slistSetSize (datalist, dataCount + slistGetCount (datalist));
      }
      /* for simple datafiles, the distvers and the version are the same */
      if (distvers != NULL) {
        slistSetVersion (datalist, *distvers);
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
          datalist = slistAlloc (name, LIST_UNORDERED, NULL);
          slistSetSize (datalist, dataCount / 2);
        } else {
          slistSetSize (datalist, dataCount / 2 + slistGetCount (datalist));
        }
        logMsg (LOG_DBG, LOG_DATAFILE, "key_val: list");
      } else {
        if (datalist == NULL) {
          datalist = nlistAlloc (name, LIST_UNORDERED, NULL);
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

  if (dfkeys != NULL) {
    logMsg (LOG_DBG, LOG_DATAFILE, "use dfkeys");
  }

  nikey = 0;
  for (ssize_t i = 0; i < dataCount; i += inc) {
    tkeystr = strdata [i];
    if (inc > 1) {
      tvalstr = strdata [i + 1];
    }

    if (inc == 2 && strcmp (tkeystr, DF_VERSION_STR) == 0) {
      /* ignore set version and replace with correct version */
      int     version = atoi (tvalstr);

      if (dftype == DFTYPE_INDIRECT) {
        ilistSetVersion (datalist, version);
      } else {
        nlistSetVersion (datalist, version);
      }
      continue;
    }
    if (strcmp (tkeystr, "count") == 0) {
      if (dftype == DFTYPE_INDIRECT) {
        ilistSetSize (datalist, atol (tvalstr) + 2);
      }
      continue;
    }

    if (dftype == DFTYPE_INDIRECT &&
        strcmp (tkeystr, "KEY") == 0) {
      char      temp [80];

      /* rather than using the indirect key in the file, renumber the data */
      /* the key value acts as a marker rather than an actual value */
      if (key >= 0) {
        nlistidx_t    tsz;

        ilistSetDatalist (datalist, nikey, itemList);
        tsz = nlistGetCount (itemList);
        if (tsz > isz) {
          isz = tsz;
        }
        key = -1L;
        nikey++;
      }
      key = atol (tvalstr);
      snprintf (temp, sizeof (temp), "%s-item-%" PRId32, name, nikey);
      itemList = nlistAlloc (temp, LIST_ORDERED, NULL);
      if (isz > 0) {
        nlistSetSize (itemList, isz);
      }
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
        logMsg (LOG_DBG, LOG_DATAFILE, "found %s idx: %" PRId32, tkeystr, idx);
        ikey = dfkeys [idx].itemkey;
        vt = dfkeys [idx].valuetype;
        logMsg (LOG_DBG, LOG_DATAFILE, "ikey:%" PRId32 "(%" PRId32 ") vt:%d tvalstr:%s", ikey, ikey + offset, vt, tvalstr);

        conv.invt = VALUE_NONE;
        if (dfkeys [idx].convFunc != NULL) {
          conv.invt = VALUE_STR;
          conv.str = tvalstr;
          dfkeys [idx].convFunc (&conv);

          vt = conv.outvt;
          if (vt == VALUE_NUM) {
            lval = conv.num;
            logMsg (LOG_DBG, LOG_DATAFILE, "converted value: %s to %" PRId64, tvalstr, lval);
          }
        } else {
          if (vt == VALUE_NUM) {
            if (strcmp (tvalstr, "") == 0) {
              lval = LIST_VALUE_INVALID;
            } else {
              lval = atoll (tvalstr);
            }
            logMsg (LOG_DBG, LOG_DATAFILE, "value: %" PRId64, lval);
          }
          if (vt == VALUE_DOUBLE) {
            if (strcmp (tvalstr, "") == 0) {
              dval = LIST_DOUBLE_INVALID;
            } else {
              dval = atof (tvalstr) / DF_DOUBLE_MULT;
            }
            logMsg (LOG_DBG, LOG_DATAFILE, "value: %.2f", dval);
          }
        }
      } else {
        logMsg (LOG_ERR, LOG_DATAFILE, "ERR: Unable to locate key: %s", tkeystr);
        continue;
      }

      logMsg (LOG_DBG, LOG_DATAFILE, "set: dftype:%d vt:%d", dftype, vt);

      /* use the 'setlist' temporary variable to hold the correct list var */
      if (dftype == DFTYPE_INDIRECT) {
        setlist = itemList;
      }
      if (dftype == DFTYPE_KEY_VAL) {
        setlist = datalist;
      }

      if (vt == VALUE_STR) {
        nlistSetStr (setlist, ikey + offset, tvalstr);
      }
      if (vt == VALUE_DATA) {
        fprintf (stderr, "invalid data type in datafile\n");
        exit (1);
      }
      if (vt == VALUE_NUM) {
        nlistSetNum (setlist, ikey + offset, lval);
      }
      if (vt == VALUE_DOUBLE) {
        nlistSetDouble (setlist, ikey + offset, dval);
      }
      if (vt == VALUE_LIST) {
        nlistSetList (setlist, ikey + offset, conv.list);
      }
    }

    if (dftype == DFTYPE_KEY_VAL && dfkeys == NULL) {
      logMsg (LOG_DBG, LOG_DATAFILE, "set: key_val");
      slistSetStr (datalist, tkeystr, tvalstr);
      key = -1L;
    }
  }

  if (dftype == DFTYPE_INDIRECT && nikey >= 0) {
    ilistSetDatalist (datalist, nikey, itemList);
  }

  parseFree (pi);
  logProcEnd ("");
  return datalist;
}

static bool
datafileCheckDfkeys (const char *name, datafilekey_t *dfkeys, int dfkeycount)
{
  char  *last = "";
  bool  ok = true;

  for (ssize_t i = 0; i < dfkeycount; ++i) {
    if (strcmp (dfkeys [i].name, last) <= 0) {
      fprintf (stderr, "datafile: %s dfkey out of order: %s\n", name, dfkeys [i].name);
      logMsg (LOG_ERR, LOG_IMPORTANT | LOG_DATAFILE, "ERR: datafile: %s dfkey out of order: %s", name, dfkeys [i].name);
      ok = false;
    }
    last = dfkeys [i].name;
  }
  return ok;
}

static void
datafileSaveKeyVal (datafile_t *df, const char *fn,
    nlist_t *list, int offset, int distvers)
{
  FILE    *fh;
  char    *buff;

  buff = mdmalloc (DATAFILE_MAX_SIZE);
  *buff = '\0';
  fh = datafileSavePrep (fn, df->tag, distvers);
  if (fh == NULL) {
    return;
  }

  fprintf (fh, "%s\n..%d\n", DF_VERSION_STR, nlistGetVersion (list));
  datafileSaveKeyValBuffer (buff, DATAFILE_MAX_SIZE, df->tag,
      df->dfkeys, df->dfkeycount, list, offset, DF_NONE);
  fprintf (fh, "%s", buff);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
}

static void
datafileSaveIndirect (datafile_t *df, const char *fn,
    ilist_t *list, int distvers)
{
  FILE            *fh;
  datafileconv_t  conv;
  valuetype_t     vt;
  ilistidx_t      count;
  ilistidx_t      iteridx;
  ilistidx_t      key;
  char            *buff;
  char            *currp;

  buff = mdmalloc (DATAFILE_MAX_SIZE);
  *buff = '\0';
  fh = datafileSavePrep (fn, df->tag, distvers);
  if (fh == NULL) {
    return;
  }

  count = ilistGetCount (list);
  ilistStartIterator (list, &iteridx);

  fprintf (fh, "%s\n..%d\n", DF_VERSION_STR, ilistGetVersion (list));
  fprintf (fh, "count\n..%" PRId32 "\n", count);

  count = 0;
  currp = buff;

  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    conv.invt = VALUE_NUM;
    /* on save, re-order the keys */
    conv.num = count++;
    currp = datafileSaveItem (buff, DATAFILE_MAX_SIZE, currp,
        "KEY", NULL, &conv, DF_NONE);

    for (ssize_t i = 0; i < df->dfkeycount; ++i) {
      if (df->dfkeys [i].writeFlag == DF_NO_WRITE) {
        continue;
      }

      vt = df->dfkeys [i].valuetype;
      conv.invt = vt;

      /* load the data value into the conv structure so that retrieval is */
      /* the same for both non-converted and converted values */
      if (vt == VALUE_NUM) {
        conv.num = ilistGetNum (list, key, df->dfkeys [i].itemkey);
      }
      if (vt == VALUE_STR) {
        conv.str = ilistGetStr (list, key, df->dfkeys [i].itemkey);
      }
      if (vt == VALUE_LIST) {
        conv.list = ilistGetList (list, key, df->dfkeys [i].itemkey);
      }
      if (vt == VALUE_DOUBLE) {
        conv.dval = ilistGetDouble (list, key, df->dfkeys [i].itemkey);
      }

      currp = datafileSaveItem (buff, DATAFILE_MAX_SIZE, currp,
          df->dfkeys [i].name, df->dfkeys [i].convFunc, &conv, DF_NONE);
    }
  }
  fwrite (buff, currp - buff, 1, fh);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
}

static void
datafileSaveList (datafile_t *df, const char *fn, slist_t *list, int distvers)
{
  FILE        *fh;
  slistidx_t  iteridx;
  const char  *str;
  char        *buff;
  char        *p;
  char        *end;

  logProcBegin ();
  buff = mdmalloc (DATAFILE_MAX_SIZE);
  end = buff + DATAFILE_MAX_SIZE;
  *buff = '\0';
  fh = datafileSavePrep (fn, df->tag, distvers);
  if (fh == NULL) {
    logProcEnd ("no-file");
    return;
  }

  fprintf (fh, DF_VERSION_FMT, slistGetVersion (list));
  fprintf (fh, "\n");

  slistStartIterator (list, &iteridx);
  p = buff;
  while ((str = slistIterateKey (list, &iteridx)) != NULL) {
    logMsg (LOG_DBG, LOG_DATAFILE, "save-list: %s", str);
    p = stpecpy (p, end, str);
    p = stpecpy (p, end, "\n");
  }
  fprintf (fh, "%s", buff);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
  logProcEnd ("");
}

static FILE *
datafileSavePrep (const char *fn, const char *tag, int distvers)
{
  FILE    *fh;
  char    tbuff [100];


  filemanipBackup (fn, 1);
  fh = fileopOpen (fn, "w");

  if (fh == NULL) {
    return NULL;
  }

  fprintf (fh, "# %s\n", tag);
  fprintf (fh, "# %s ", tmutilDstamp (tbuff, sizeof (tbuff)));
  fprintf (fh, "%s\n", tmutilTstamp (tbuff, sizeof (tbuff)));
  fprintf (fh, "# version %d\n", distvers);
  return fh;
}

static char *
datafileSaveItem (char *buff, size_t sz, char *currp, const char *name,
    dfConvFunc_t convFunc, datafileconv_t *conv, int flags)
{
  char    valbuff [2048];
  char    *end = buff + sz;

  datafileConvertValue (valbuff, sizeof (valbuff), convFunc, conv);
  if ((flags & DF_SKIP_EMPTY) == DF_SKIP_EMPTY) {
    if (! *valbuff) {
      return currp;
    }
  }
  currp = stpecpy (currp, end, name);
  currp = stpecpy (currp, end, "\n");
  currp = stpecpy (currp, end, "..");
  currp = stpecpy (currp, end, valbuff);
  currp = stpecpy (currp, end, "\n");
  return currp;
}

static void
datafileLoadConv (datafilekey_t *dfkey, nlist_t *list,
    datafileconv_t *conv, int offset)
{
  valuetype_t     vt;

  vt = dfkey->valuetype;
  conv->invt = vt;
  conv->outvt = vt;

  /* load the data value into the conv structure so that retrieval is */
  /* the same for both non-converted and converted values */
  if (vt == VALUE_NUM) {
    conv->outvt = VALUE_STR;
    conv->num = nlistGetNum (list, dfkey->itemkey + offset);
  }
  if (vt == VALUE_STR) {
    conv->outvt = VALUE_NUM;
    conv->str = nlistGetStr (list, dfkey->itemkey + offset);
  }
  if (vt == VALUE_LIST) {
    conv->outvt = VALUE_STR;
    conv->list = nlistGetList (list, dfkey->itemkey + offset);
  }
  if (vt == VALUE_DOUBLE) {
    conv->outvt = VALUE_STR;
    conv->dval = nlistGetDouble (list, dfkey->itemkey + offset);
  }
}

static void
datafileConvertValue (char *buff, size_t sz, dfConvFunc_t convFunc,
    datafileconv_t *conv)
{
  valuetype_t     vt;

  vt = conv->invt;
  if (convFunc != NULL) {
    convFunc (conv);
    vt = conv->outvt;
  }

  *buff = '\0';
  if (vt == VALUE_NUM) {
    if (conv->num != LIST_VALUE_INVALID) {
      snprintf (buff, sz, "%" PRId64, conv->num);
    }
  }
  if (vt == VALUE_DOUBLE) {
    if (conv->dval != LIST_DOUBLE_INVALID) {
      snprintf (buff, sz, "%.0f", conv->dval * DF_DOUBLE_MULT);
    }
  }
  if (vt == VALUE_STR) {
    if (conv->str != NULL) {
      snprintf (buff, sz, "%s", conv->str);
    }
  }
  if (vt == VALUE_STRVAL) {
    if (conv->strval != NULL) {
      snprintf (buff, sz, "%s", conv->strval);
      mdfree (conv->strval);
    }
  }
}

static void
datafileDumpItem (const char *tag, const char *name, dfConvFunc_t convFunc,
    datafileconv_t *conv)
{
  char    tbuff [1024];

  datafileConvertValue (tbuff, sizeof (tbuff), convFunc, conv);
  fprintf (stdout, "%3s: %-20s ", tag, name);
  fprintf (stdout, "%s", tbuff);
  fprintf (stdout, "\n");
}
