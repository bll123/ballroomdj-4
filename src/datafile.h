#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

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
  char    **strdata;
  size_t  allocCount;
  size_t  count;
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
  union {
    long        l;
    list_t      *list;
    char        *str;
  } u;
} datafileret_t;

typedef void (*dfConvFunc_t)(char *, datafileret_t *);

typedef struct {
  char            *name;
  size_t          itemkey;
  valuetype_t     valuetype;
  dfConvFunc_t    convFunc;
} datafilekey_t;

#define DATAFILE_NO_LOOKUP -1

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
size_t        parseSimple (parseinfo_t *, char *);
size_t        parseKeyValue (parseinfo_t *, char *);
void          parseConvBoolean (char *, datafileret_t *);
void          parseConvTextList (char *, datafileret_t *);

datafile_t *  datafileAlloc (char *name);
datafile_t *  datafileAllocParse (char *name, datafiletype_t dftype,
                  char *fname, datafilekey_t *dfkeys, size_t dfkeycount,
                  long lookupKey);
void          datafileFree (void *);
char *        datafileLoad (datafile_t *df, datafiletype_t dftype, char *fname);
list_t        *datafileParse (char *data, char *name, datafiletype_t dftype,
                  datafilekey_t *dfkeys, size_t dfkeycount,
                  long lookupKey, list_t **lookup);
list_t        *datafileParseMerge (list_t *nlist, char *data, char *name,
                  datafiletype_t dftype,
                  datafilekey_t *dfkeys, size_t dfkeycount,
                  long lookupKey, list_t **lookup);
long          dfkeyBinarySearch (const datafilekey_t *dfkeys,
                  size_t count, char *key);
list_t *      datafileGetList (datafile_t *);
list_t *      datafileGetLookup (datafile_t *);
void          datafileSetData (datafile_t *df, void *data);
int           datafileSave (datafilekey_t *, size_t dfkeycount, datafile_t *);
void          datafileBackup (char *fname, int count);

#endif /* INC_DATAFILE_H */
