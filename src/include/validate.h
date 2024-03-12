/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VALIDATION_H
#define INC_VALIDATION_H

enum {
  VAL_NONE          = 0x0000,
  VAL_NOT_EMPTY     = 0x0001,
  VAL_NO_SPACES     = 0x0002,
  VAL_NO_SLASHES    = 0x0004,
  VAL_NUMERIC       = 0x0008,
  VAL_FLOAT         = 0x0010,
  VAL_HOUR_MIN      = 0x0020,
  VAL_MIN_SEC       = 0x0040,
  VAL_HOUR_MIN_SEC  = 0x0080,
};

const char * validate (const char *str, int flags);

#endif /* INC_VALIDATION_H */
