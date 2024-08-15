/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PATHUTIL_H
#define INC_PATHUTIL_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void          pathStripPath (char *buff, size_t len);
void          pathNormalizePath (char *buff, size_t len);
void          pathRealPath (char *to, const char *from, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PATHUTIL_H */
