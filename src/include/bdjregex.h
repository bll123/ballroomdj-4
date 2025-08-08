/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJREGEX_H
#define INC_BDJREGEX_H

#include "nodiscard.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct bdjregex bdjregex_t;

NODISCARD bdjregex_t * regexInit (const char *pattern);
void regexFree (bdjregex_t *rx);
char * regexEscape (const char *str);
bool regexMatch (bdjregex_t *rx, const char *str);
char ** regexGet (bdjregex_t *rx, const char *str);
void regexGetFree (char **val);
char *regexReplace (bdjregex_t *rx, const char *str, const char *repl);
char *regexReplaceLiteral (const char *str, const char *tgt, const char *repl);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_BDJREGEX_H */
