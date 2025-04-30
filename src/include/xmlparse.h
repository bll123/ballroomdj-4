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

typedef struct xmlparse xmlparse_t;

xmlparse_t * xmlParseInit (const char *fname);
void xmlParseFree (xmlparse_t * xmlparse);
void xmlParseGetItem (xmlparse_t * xmlparse, const char *xpath, char *buff, size_t sz);
ilist_t * xmlParseGetList (xmlparse_t * xmlparse, const char *xpath);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_XMLPARSE_H */
