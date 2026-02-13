/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

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
void  osGetPreferredLocales (char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

