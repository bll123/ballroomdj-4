/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if __has_cpp_attribute( nodiscard )
# define NODISCARD [[nodiscard]]
#else
# define NODISCARD
#endif

