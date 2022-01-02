#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

typedef enum {
  DFTYPE_LIST,
    /* key_string: use the first item key found after version/count as
       the break.  Otherwise the same as key long. */
  DFTYPE_KEY_STRING,
    /* key_long: has a 'KEY' value that begins a block of key/values. */
  DFTYPE_KEY_LONG,
    /* only key/values */
  DFTYPE_KEY_VAL,
  DFTYPE_MAX,
} datafiletype_t;

typedef struct {
  char    **strdata;
  size_t  allocCount;
  size_t  count;
} parseinfo_t;

typedef long (*dfConvFunc_t)(const char *);

typedef struct {
  char            *name;
  size_t          itemkey;
  valuetype_t     valuetype;
  dfConvFunc_t    convFunc;
} datafilekey_t;

typedef struct {
  char            *fname;
  datafiletype_t  dftype;
  list_t          *data;
  long            version;
} datafile_t;

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
size_t        parseSimple (parseinfo_t *, char *);
size_t        parseKeyValue (parseinfo_t *, char *);
long          parseConvBoolean (const char *);

datafile_t *  datafileAlloc (datafilekey_t *, size_t dfkeycount, char *,
                  datafiletype_t dftype);
void          datafileFree (void *);
void          datafileLoad (datafilekey_t *, size_t dfkeycount,
                  datafile_t *, char *, datafiletype_t dftype);
list_t *      datafileGetData (datafile_t *);
int           datafileSave (datafilekey_t *, size_t dfkeycount, datafile_t *);
void          datafileBackup (char *fname, int count);

#endif /* INC_DATAFILE_H */
