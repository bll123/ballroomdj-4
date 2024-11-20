/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ROMAN_H
#define INC_ROMAN_H

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int convertToRoman (int value, char * buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ROMAN_H */
