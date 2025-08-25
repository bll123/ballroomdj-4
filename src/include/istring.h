/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void      istringInit (const char *locale);
void      istringCleanup (void);
int       istringCompare (const char *, const char *);
size_t    istrlen (const char *);
void      istringToLower (char *str);
char *    istring16ToUTF8 (wchar_t *instr);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

