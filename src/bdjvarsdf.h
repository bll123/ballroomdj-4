#ifndef INC_BDJVARSDF_H
#define INC_BDJVARSDF_H

#include "datafile.h"

typedef enum {
  BDJVDF_AUTO_SEL,
  BDJVDF_DANCES,
  BDJVDF_DANCE_SPEEDS,
  BDJVDF_DANCE_TYPES,
  BDJVDF_GENRES,
  BDJVDF_LEVELS,
  BDJVDF_RATINGS,
  BDJVDF_SORT_OPT,
  BDJVDF_MAX,
} bdjvarkeydf_t;

extern void *bdjvarsdf [BDJVDF_MAX];

#endif /* INC_BDJVARSDF_H */
