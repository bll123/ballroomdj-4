/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <locale.h>
#if __has_include (<libintl.h>)
# include <libintl.h>
#endif
/* the gettext macros are in glib (and pgettext!) */
#if __has_include (<glib/gi18n.h>)
# include <glib/gi18n.h>
#endif
