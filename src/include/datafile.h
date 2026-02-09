/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "list.h"
#include "nlist.h"
#include "slist.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  DFTYPE_NONE,
  DFTYPE_LIST,
  DFTYPE_INDIRECT,
  DFTYPE_KEY_VAL,
  DFTYPE_MAX,
} datafiletype_t;

enum {
  DF_NONE         = 0,
  DF_SKIP_EMPTY   = (1 << 0),
};

typedef struct parseinfo parseinfo_t;

typedef struct datafile datafile_t;

typedef struct {
  valuetype_t   invt;
  valuetype_t   outvt;
  union {
    listnum_t   num;
    list_t      *list;
    const char  *str;
    double      dval;
    char        *strval;
  };
} datafileconv_t;

typedef void (*dfConvFunc_t)(datafileconv_t *);

/*
 * The datafilekey_t table is used to parse and process the datafile.
 *
 * list : simple list
 *    key only or key/value.
 *    ( sequence ( discards and rebuilds ), sortopt )
 * indirect : list (key : long) -> list (key : long/string)
 *    These datafiles have an ordering key, with data associated with
 *    each key. e.g.
 *    0 : dance Waltz rating Good; 1 : dance Tango rating Great
 *    ( dance, genre, rating, level, playlist dances, songlist )
 * key/val : key : long -> value
 *    These always have a datafilekey_t table.
 *    ( autosel, bdjopt/bdjconfig, playlist info, music db (manually built) )
 *
 */
typedef struct {
  char            *name;
  int             itemkey;
  valuetype_t     valuetype;
  dfConvFunc_t    convFunc;
  int             writeFlag;
} datafilekey_t;

enum {
  DF_NORM = -1,
  DF_NO_WRITE = -2,
  DF_NO_OFFSET = 0,
  /* 2025-6-16 update size to 4 megs */
  DATAFILE_MAX_SIZE = 4 * 1024 * 1024,
};
#define DF_DOUBLE_MULT 1000.0

BDJ_NODISCARD parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
ssize_t       parseSimple (parseinfo_t *, char *, int *distvers);
ssize_t       parseKeyValue (parseinfo_t *, char *, int *distvers);
void          convBoolean (datafileconv_t *conv);
void          convTextList (datafileconv_t *conv);

BDJ_NODISCARD datafile_t *  datafileAlloc (const char *tag, datafiletype_t dftype, const char *fname, datafilekey_t *dfkeys, int dfkeycount);
BDJ_NODISCARD datafile_t *  datafileAllocParse (const char *tag, datafiletype_t dftype, const char *fname, datafilekey_t *dfkeys, int dfkeycount, int offset, datafile_t *mergedf);
void          datafileFree (void *);
char *        datafileLoad (datafile_t *df, datafiletype_t dftype, const char *fname);
BDJ_NODISCARD list_t        *datafileParse (char *data, const char *name, datafiletype_t dftype, datafilekey_t *dfkeys, int dfkeycount, int *distvers);
listidx_t     dfkeyBinarySearch (const datafilekey_t *dfkeys, int count, const char *key);
list_t *      datafileGetList (datafile_t *);
BDJ_NODISCARD slist_t *     datafileSaveKeyValList (const char *tag, datafilekey_t *dfkeys, int dfkeycount, nlist_t *list);
size_t        datafileSaveKeyValBuffer (char *buff, size_t sz, const char *tag, datafilekey_t *dfkeys, int dfkeycount, nlist_t *list, int offset, int flags);
void          datafileSave (datafile_t *df, const char *fn, nlist_t *list, int offset, int distvers);
void          datafileDumpKeyVal (const char *tag, datafilekey_t *dfkeys, int dfkeycount, nlist_t *list, int offset);
int           datafileDistVersion (datafile_t *df);
int           datafileReadDistVersion (const char *fname);

/* for debugging only */
datafiletype_t datafileGetType (datafile_t *df);
char *datafileGetFname (datafile_t *df);
list_t *datafileGetData (datafile_t *df);
listidx_t parseGetAllocCount (parseinfo_t *pi);


#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

