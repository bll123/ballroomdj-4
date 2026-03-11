/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "ilist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char *_gettext (const char *str);

enum {
  LOCALE_KEY_LONG,
  LOCALE_KEY_SHORT,
  LOCALE_KEY_DISPLAY,
  LOCALE_KEY_STDROUNDS,
  LOCALE_KEY_QDANCE,
  LOCALE_KEY_MAX,
};

enum {
  LOCALE_USE_SYS,
  LOCALE_USE_DATA,
};

void  localeInit (void);
void  localeSetup (void);
void  localeCleanup (void);
const char *localeGetStr (int key);
slist_t *localeGetDisplayList (void);
void  localeDebug (const char *tag);
ilist_t * localeCreateDropDownList (int *idx, bool uselocale);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
