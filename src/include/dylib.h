/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DYLIB_H
#define INC_DYLIB_H

typedef void dlhandle_t;

dlhandle_t    *dylibLoad (const char *path);
void          dylibClose (dlhandle_t *dlhandle);
void          *dylibLookup (dlhandle_t *dlhandle, const char *funcname);

#endif /* INC_DYLIB_H */
