/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

bool aesencrypt (const char *str, char *buff, size_t sz);
bool aesdecrypt (const char *str, char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

