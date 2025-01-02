/*
 * Copyright 2022-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VSENCDEC_H
#define INC_VSENCDEC_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define VSEC_E_PFX "ENC"

void vsencdec (const char *str, char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_VSENCDEC_H */
