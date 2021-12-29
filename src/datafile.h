#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

typedef enum {
  DFTYPE_LIST,
    /* key string: use the first item key found after version/count as
     * the break.  Otherwise the same as key long.
     */
  DFTYPE_KEY_STRING,
  DFTYPE_KEY_LONG,
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
int           datafileSave (datafilekey_t *, size_t dfkeycount, datafile_t *);

#endif /* INC_DATAFILE_H */
