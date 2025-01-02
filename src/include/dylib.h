/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DYLIB_H
#define INC_DYLIB_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef void dlhandle_t;

dlhandle_t    *dylibLoad (const char *path);
void          dylibClose (dlhandle_t *dlhandle);
void          *dylibLookup (dlhandle_t *dlhandle, const char *funcname);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_DYLIB_H */
