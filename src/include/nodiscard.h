/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_NODISCARD_H
#define INC_NODISCARD_H

#if __has_cpp_attribute( nodiscard )
# define NODISCARD [[nodiscard]]
#else
# define NODISCARD
#endif

#endif /* INC_NODISCARD_H */
