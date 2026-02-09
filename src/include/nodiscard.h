/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#define BDJ_NODISCARD
#if __STDC_VERSION__ < 202000
# undef BDJ_NODISCARD
# define BDJ_NODISCARD __attribute__ ((warn_unused_result))
#endif
#if __STDC_VERSION__ >= 202000 && defined (__has_cpp_attribute)
# if ! defined (BDJ_NODISCARD) && __has_cpp_attribute( nodiscard )
#  undef BDJ_NODISCARD
#  define BDJ_NODISCARD [[nodiscard]]
# endif
#endif
