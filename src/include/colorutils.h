/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_COLORUTILS_H
#define INC_COLORUTILS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char    *createRandomColor (char *tbuff, size_t sz);
void    colorValues (const char *color, double *r, double *g, double *b);
// double  colorLuminance (const char *color); // not in use

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_COLORUTILS_H */
