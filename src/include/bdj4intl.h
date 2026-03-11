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
#define C_(ctxt,str) bdj4_pgettext(ctxt "\x04" str, str)

/* taken from vlc/plugins/vlc_common.h */
static inline const char *
bdj4_pgettext (const char *ctx, const char *str)
{
  const char *tr = _(ctx);
  return (tr == ctx) ? str : tr;
}

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

