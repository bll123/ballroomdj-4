#ifndef INC_BDJVARSDF_H
#define INC_BDJVARSDF_H

#include "datafile.h"

typedef enum {
  BDJVDF_DANCE_SPEEDS,
  BDJVDF_DANCE_TYPES,
  BDJVDF_DANCES,
  BDJVDF_RATINGS,
  BDJVDF_GENRES,
  BDJVDF_LEVELS,
  BDJVDF_SORT_OPT,
  BDJVDF_AUTO_SEL,
  BDJVDF_MAX,
} bdjvarkeydf_t;

extern datafile_t *bdjvarsdf [BDJVDF_MAX];

void    bdjvarsdfInit (void);
void    bdjvarsdfCleanup (void);

#endif /* INC_BDJVARSDF_H */
