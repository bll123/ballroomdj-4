/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DYLIB_H
#define INC_DYLIB_H

typedef void dlhandle_t;

dlhandle_t    *dylibLoad (char *path);
void          dylibClose (dlhandle_t *dlhandle);
void          *dylibLookup (dlhandle_t *dlhandle, char *funcname);

#endif /* INC_DYLIB_H */
