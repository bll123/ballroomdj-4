/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_LIST_H
#define INC_LIST_H

#include <stdint.h>
#include <stdbool.h>

typedef int32_t listidx_t;
typedef int64_t listnum_t;

typedef enum {
  LIST_UNORDERED,
  LIST_ORDERED,
} listorder_t;

typedef enum {
  VALUE_NONE,
  VALUE_DOUBLE,
  VALUE_DATA,
  VALUE_STR,
  VALUE_NUM,
  VALUE_LIST,
  VALUE_ALIAS,
  VALUE_STRVAL,     // only used by the conversion functions (out valuetype)
} valuetype_t;

enum {
  LIST_LOC_INVALID    = -1,
  LIST_VALUE_INVALID  = -65534,
  LIST_END_LIST       = -1,
  LIST_NO_VERSION     = -1,
};
#define LIST_DOUBLE_INVALID -665534.0

typedef struct list list_t;
typedef void (*listFree_t)(void *);

#endif /* INC_LIST_H */
