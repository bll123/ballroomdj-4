#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

typedef enum {
  DFTYPE_NONE,
    /* list: simple list */
  DFTYPE_LIST,
    /* key_string: use the first item key found after version/count as
       the break.  Otherwise the same as key long. */
  DFTYPE_KEY_STRING,
    /* key_long: has a 'KEY' value that begins a block of key/values. */
  DFTYPE_KEY_LONG,
    /* key_val: only key/values; generally not used. */
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

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
size_t        parseSimple (parseinfo_t *, char *);
size_t        parseKeyValue (parseinfo_t *, char *);
void          parseConvBoolean (char *, datafileret_t *);
void          parseConvTextList (char *, datafileret_t *);

datafile_t *  datafileAlloc (char *name);
datafile_t *  datafileAllocParse (char *name, datafiletype_t dftype,
                  char *fname, datafilekey_t *dfkeys, size_t dfkeycount);
void          datafileFree (void *);
char *        datafileLoad (datafile_t *df, datafiletype_t dftype, char *fname);
list_t *      datafileParse (char *data, char *name, datafiletype_t dftype,
                  datafilekey_t *dfkeys, size_t dfkeycount);
list_t *      datafileGetData (datafile_t *);
int           datafileSave (datafilekey_t *, size_t dfkeycount, datafile_t *);
void          datafileBackup (char *fname, int count);

#endif /* INC_DATAFILE_H */
