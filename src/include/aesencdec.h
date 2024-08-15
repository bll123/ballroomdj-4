/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AESENCDEC_H
#define INC_AESENCDEC_H

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

bool aesencrypt (const char *str, char *buff, size_t sz);
bool aesdecrypt (const char *str, char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_AESENCDEC_H */
