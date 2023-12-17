#ifndef INC_AESENCDEC_H
#define INC_AESENCDEC_H

#include <stdbool.h>

bool aesencrypt (const char *str, char *buff, size_t sz);
bool aesdecrypt (const char *str, char *buff, size_t sz);

#endif /* INC_AESENCDEC_H */
