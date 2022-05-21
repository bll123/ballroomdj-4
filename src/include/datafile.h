#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"
#include "nlist.h"
#include "slist.h"
#include "ilist.h"

typedef enum {
  DFTYPE_NONE,
  DFTYPE_LIST,
  DFTYPE_INDIRECT,
  DFTYPE_KEY_VAL,
  DFTYPE_MAX,
} datafiletype_t;

typedef struct parseinfo parseinfo_t;

typedef struct datafile datafile_t;

typedef struct {
  valuetype_t   valuetype;
  bool          allocated;
  union {
    ssize_t     num;
    list_t      *list;
    char        *str;
    double      dval;
  };
} datafileconv_t;

typedef void (*dfConvFunc_t)(datafileconv_t *);

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
typedef struct {
  char            *name;
  ssize_t         itemkey;
  valuetype_t     valuetype;
  dfConvFunc_t    convFunc;
  ssize_t         backupKey;
} datafilekey_t;

#define DATAFILE_NO_LOOKUP -1
#define DATAFILE_NO_BACKUPKEY -1
#define DATAFILE_NO_WRITE -2

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
ssize_t       parseSimple (parseinfo_t *, char *);
ssize_t       parseKeyValue (parseinfo_t *, char *);
void          convBoolean (datafileconv_t *conv);
void          convTextList (datafileconv_t *conv);
void          convMS (datafileconv_t *conv);

datafile_t *  datafileAlloc (char *name);
datafile_t *  datafileAllocParse (char *name, datafiletype_t dftype,
                  const char *fname, datafilekey_t *dfkeys, ssize_t dfkeycount,
                  listidx_t lookupKey);
void          datafileFree (void *);
char *        datafileLoad (datafile_t *df, datafiletype_t dftype, const char *fname);
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
void          datafileSaveKeyVal (char *tag, char *fn, datafilekey_t *dfkeys, ssize_t count, nlist_t *list);
void          datafileSaveIndirect (char *tag, char *fn, datafilekey_t *dfkeys, ssize_t count, nlist_t *list);
void          datafileSaveList (char *tag, char *fn, slist_t *list);
void          datafileDumpKeyVal (char *tag, datafilekey_t *dfkeys, ssize_t dfkeycount, nlist_t *list);

/* for debugging only */
datafiletype_t datafileGetType (datafile_t *df);
char *datafileGetFname (datafile_t *df);
list_t *datafileGetData (datafile_t *df);
listidx_t parseGetAllocCount (parseinfo_t *pi);


#endif /* INC_DATAFILE_H */
