/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void      istringInit (const char *locale);
bool      istringCheck (void);
void      istringCleanup (void);
int       istringCompare (const char *, const char *);
size_t    istrlen (const char *);
void      istringToLower (char *str);
const char * istring639_2 (const char *locale);
char *    istring16ToUTF8 (const unsigned char *instr);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

