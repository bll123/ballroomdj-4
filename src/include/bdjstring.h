/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJSTRING
#define INC_BDJSTRING

#include "config.h"

char *    stringAsciiToLower (char *s);
char *    stringAsciiToUpper (char *s);
void      stringTrim (char *s);
void      stringTrimChar (char *s, unsigned char c);
int       versionCompare (const char *v1, const char *v2);
size_t    stringAppend (char *str, size_t maxsz, size_t currsz, const char *data);
void      dataFree (void *data);

#if ! _lib_strlcat
size_t strlcat(char *dst, const char *src, size_t siz);
#endif
#if ! _lib_strlcpy
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#endif /* INC_BDJSTRING */
