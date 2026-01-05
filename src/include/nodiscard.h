/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#define NODISCARD
#if __STDC_VERSION__ < 202000
# undef NODISCARD
# define NODISCARD __attribute__ ((warn_unused_result))
#endif
#if __has_cpp_attribute( nodiscard ) || __STDC_VERSION__ >= 202000
# undef NODISCARD
# define NODISCARD [[nodiscard]]
#endif
