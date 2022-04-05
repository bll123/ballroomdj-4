#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"
#include "nlist.h"
#include "slist.h"
#include "ilist.h"

/*
 * The datafilekey_t table is used to convert string keys to long keys.
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
typedef enum {
  DFTYPE_NONE,
  DFTYPE_LIST,
  DFTYPE_INDIRECT,
  DFTYPE_KEY_VAL,
  DFTYPE_MAX,
} datafiletype_t;

typedef struct {
  char        **strdata;
  ssize_t     allocCount;
  ssize_t     count;
} parseinfo_t;

typedef struct {
  char            *name;
  char            *fname;
  datafiletype_t  dftype;
  list_t          *data;
  list_t          *lookup;
  long            version;
} datafile_t;

typedef struct {
  valuetype_t   valuetype;
  bool          allocated;
  union {
    ssize_t     num;
    list_t      *list;
    char        *str;
    double      dval;
  } u;
} datafileconv_t;

typedef void (*dfConvFunc_t)(datafileconv_t *);

typedef struct {
  char            *name;
  ssize_t         itemkey;
  valuetype_t     valuetype;
  dfConvFunc_t    convFunc;
  ssize_t         backupKey;
} datafilekey_t;

#define DATAFILE_NO_LOOKUP -1

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
ssize_t       parseSimple (parseinfo_t *, char *);
ssize_t       parseKeyValue (parseinfo_t *, char *);
void          convBoolean (datafileconv_t *);
void          convTextList (datafileconv_t *);

datafile_t *  datafileAlloc (char *name);
datafile_t *  datafileAllocParse (char *name, datafiletype_t dftype,
                  char *fname, datafilekey_t *dfkeys, ssize_t dfkeycount,
                  listidx_t lookupKey);
void          datafileFree (void *);
char *        datafileLoad (datafile_t *df, datafiletype_t dftype, char *fname);
list_t        *datafileParse (char *data, char *name, datafiletype_t dftype,
                  datafilekey_t *dfkeys, ssize_t dfkeycount,
                  listidx_t lookupKey, list_t **lookup);
list_t        *datafileParseMerge (list_t *nlist, char *data, char *name,
                  datafiletype_t dftype,
                  datafilekey_t *dfkeys, ssize_t dfkeycount,
                  listidx_t lookupKey, list_t **lookup);
listidx_t     dfkeyBinarySearch (const datafilekey_t *dfkeys,
                  ssize_t count, char *key);
list_t *      datafileGetList (datafile_t *);
list_t *      datafileGetLookup (datafile_t *);
void          datafileSetData (datafile_t *df, void *data);
void          datafileBackup (char *fname, int count);
void          datafileSaveKeyVal (char *tag, char *fn, datafilekey_t *dfkeys, ssize_t count, nlist_t *list);
void          datafileSaveIndirect (char *tag, char *fn, datafilekey_t *dfkeys, ssize_t count, nlist_t *list);

#endif /* INC_DATAFILE_H */
