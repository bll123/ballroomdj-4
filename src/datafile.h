#ifndef INC_DATAFILE_H
#define INC_DATAFILE_H

#include "list.h"

typedef struct {
  char    **strdata;
  size_t  allocCount;
  size_t  count;
} parseinfo_t;

parseinfo_t * parseInit (void);
void          parseFree (parseinfo_t *);
char **       parseGetData (parseinfo_t *);
size_t        parseSimple (parseinfo_t *, char *);
size_t        parseKeyValue (parseinfo_t *, char *);
int           saveDataFile (list_t *, char *);
void          makeBackups (char *, int);
int           fileExists (char *);
int           fileCopy (char *, char *);
int           fileMove (char *, char *);

#endif /* INC_DATAFILE_H */
