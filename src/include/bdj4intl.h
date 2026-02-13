/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <locale.h>
#if __has_include (<libintl.h>)
# include <libintl.h>
#endif

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define _(str) gettext (str)

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

