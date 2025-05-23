/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJSTRING_H
#define INC_BDJSTRING_H

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char *    stringAsciiToLower (char *s);
char *    stringAsciiToUpper (char *s);
void      stringTrim (char *s);
void      stringTrimChar (char *s, unsigned char c);
int       versionCompare (const char *v1, const char *v2);

char * stpecpy (char *dst, char *end, const char *restrict src);

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

#endif /* INC_BDJSTRING_H */
