/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_XMLPARSE_H
#define INC_XMLPARSE_H

#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  XMLPARSE_VAL,
  XMLPARSE_NM,
};

enum {
  XMLPARSE_USENS,
  XMLPARSE_NONS,
};

typedef struct {
  const char  *name;
  int         idx;
  const char  *attr;
} xmlparseattr_t;

typedef struct xmlparse xmlparse_t;

xmlparse_t * xmlParseInitFile (const char *fname, int nsflag);
xmlparse_t * xmlParseInitData (const char *data, size_t datalen, int nsflag);
void xmlParseFree (xmlparse_t * xmlparse);
void xmlParseGetItem (xmlparse_t * xmlparse, const char *xpath, char *buff, size_t sz);
ilist_t * xmlParseGetList (xmlparse_t * xmlparse, const char *xpath, const xmlparseattr_t attr []);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_XMLPARSE_H */
