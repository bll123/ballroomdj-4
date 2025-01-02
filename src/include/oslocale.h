/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSLOCALE_H
#define INC_OSLOCALE_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  /* locale text direction */
  TEXT_DIR_DEFAULT,
  TEXT_DIR_LTR,
  TEXT_DIR_RTL,
};

char  *osGetLocale (char *buff, size_t sz);
int   osLocaleDirection (const char *);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_OSLOCALE_H */
