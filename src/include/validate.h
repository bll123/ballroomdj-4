#ifndef INC_VALIDATION_H
#define INC_VALIDATION_H

enum {
  VAL_NONE      = 0x0000,
  VAL_NOT_EMPTY = 0x0001,
  VAL_NO_SPACES = 0x0002,
  VAL_NUMERIC   = 0x0004,
  VAL_FLOAT     = 0x0008,
};

const char * validate (const char *str, int flags);

#endif /* INC_VALIDATION_H */
