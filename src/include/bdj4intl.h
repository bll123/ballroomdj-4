/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <locale.h>
#if __has_include (<libintl.h>)
# include <libintl.h>
#endif

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define GETTEXT_DOMAIN "bdj4"
#define _(str) gettext (str)

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

