/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_LOCALEUTIL_H
#define INC_LOCALEUTIL_H

#include "slist.h"

char *_gettext (const char *str);

enum {
  LOCALE_KEY_LONG,
  LOCALE_KEY_SHORT,
  LOCALE_KEY_ISO639_3,
  LOCALE_KEY_DISPLAY,
  LOCALE_KEY_AUTO,
  LOCALE_KEY_STDROUNDS,
  LOCALE_KEY_QDANCE,
  LOCALE_KEY_MAX,
};

void  localeInit (void);
void  localeSetup (void);
void  localeCleanup (void);
const char *localeGetStr (int key);
slist_t *localeGetDisplayList (void);
void  localeDebug (void);

#endif /* INC_LOCALEUTIL_H */

