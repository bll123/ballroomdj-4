/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char *    stringAsciiToLower (char *s);
char *    stringAsciiToUpper (char *s);
void      stringTrim (char *s);
void      stringTrimChar (char *s, unsigned char c);
int       versionCompare (const char *v1, const char *v2);

#if defined (__cplusplus) || defined (c_plusplus)
char * stpecpy (char *dst, char *end, const char *src);
#else
char * stpecpy (char *dst, char *end, const char *restrict src);
#endif

/* windows snprintf does not support positional parameters. */
/* use _sprintf_p instead */
#if _lib__sprintf_p
# ifdef snprintf
#  undef snprintf   // undo mongoose snprintf define
# endif
# define snprintf(s,ssz,fmt,...) _sprintf_p (s, ssz, fmt __VA_OPT__(,) __VA_ARGS__)
#endif

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

