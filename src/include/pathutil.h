/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void          pathStripPath (char *buff, size_t len);
void          pathRealPath (char *path, size_t sz);
/* these are used for windows */
void          pathShortPath (char *path, size_t sz);
void          pathLongPath (char *path, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

