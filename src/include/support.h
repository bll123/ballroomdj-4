/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  SUPPORT_NO_COMPRESSION,
  SUPPORT_COMPRESSED,
};

typedef struct support support_t;

support_t * supportAlloc (void);
void supportFree (support_t *support);
void supportGetLatestVersion (support_t *support, char *buff, size_t sz);
void supportSendFile (support_t *support, const char *ident, const char *origfn, int compflag);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

