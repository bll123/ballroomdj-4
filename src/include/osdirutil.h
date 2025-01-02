/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSDIRUTIL_H
#define INC_OSDIRUTIL_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int     osChangeDir (const char *path);
void    osGetCurrentDir (char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_OSDIRUTIL_H */
