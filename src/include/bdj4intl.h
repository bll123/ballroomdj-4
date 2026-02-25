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

#define _(str) gettext(str)
#define C_(ctxt,str) gettext(ctxt "\x04" str)

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

