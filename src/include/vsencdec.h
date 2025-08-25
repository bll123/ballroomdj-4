/*
 * Copyright 2022-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define VSEC_E_PFX "ENC"

void vsencdec (const char *str, char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

