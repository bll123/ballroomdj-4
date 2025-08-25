/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  DYLIB_OPT_NONE        = 0x00,
  DYLIB_OPT_VERSION     = 0x01,
  DYLIB_OPT_MAC_PREFIX  = 0x02,
  DYLIB_OPT_ICU         = 0x04,
  DYLIB_OPT_AV          = 0x08,
} dlopt_t;

enum {
  DYLIB_ICU_BEG_VERS    = 65,
  DYLIB_ICU_END_VERS    = 120,
  DYLIB_AV_BEG_VERS     = 56,   // include both libavformat and libavutil
  DYLIB_AV_END_VERS     = 90,
};

typedef void dlhandle_t;

dlhandle_t    *dylibLoad (const char *path, dlopt_t opt);
void          dylibClose (dlhandle_t *dlhandle);
void          *dylibLookup (dlhandle_t *dlhandle, const char *funcname);
int           dylibCheckVersion (dlhandle_t *dlhandle, const char *funcname, dlopt_t opt);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

