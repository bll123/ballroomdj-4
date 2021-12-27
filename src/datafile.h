#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

typedef struct {
  char    **strdata;
  size_t  allocCount;
  size_t  count;
} parseinfo_t;

typedef struct {
  size_t      itemkey;
  char        *name;
  valuetype_t valuetype;
} datafilekey_t;

typedef struct {
  char        *fname;
  list_t      *data;
} datafile_t;

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
size_t        parseSimple (parseinfo_t *, char *);
size_t        parseKeyValue (parseinfo_t *, char *);

datafile_t *  datafileAlloc (datafilekey_t *, size_t dfkeycount, char *);
void          datafileFree (void *);
void          datafileLoad (datafilekey_t *, size_t dfkeycount,
                  datafile_t *, char *);
int           datafileSave (datafilekey_t *, size_t dfkeycount, datafile_t *);

#endif /* INC_DATAFILE_H */
