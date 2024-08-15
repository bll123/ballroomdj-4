/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJVARSDF_H
#define INC_BDJVARSDF_H

#include "datafile.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  BDJVDF_AUTO_SEL,
  BDJVDF_DANCES,
  BDJVDF_DANCE_TYPES,
  BDJVDF_GENRES,
  BDJVDF_LEVELS,
  BDJVDF_RATINGS,
  BDJVDF_STATUS,
  BDJVDF_FAVORITES,
  BDJVDF_AUDIO_ADJUST,
  BDJVDF_MAX,
} bdjvarkeydf_t;

void  * bdjvarsdfGet (bdjvarkeydf_t idx);
void  bdjvarsdfSet (bdjvarkeydf_t idx, void *data);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_BDJVARSDF_H */
