/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_GSTI_H
#define INC_GSTI_H

#include "pli.h"

typedef struct gsti gsti_t;

gsti_t *gstiInit (const char *plinm);
void gstiFree (gsti_t *gsti);
void gstiCleanup (void);
void gstiMedia (gsti_t *gsti, const char *fulluri, int sourceType);
int64_t gstiGetDuration (gsti_t *gsti);
int64_t gstiGetPosition (gsti_t *gsti);
plistate_t gstiState (gsti_t *gsti);
void gstiPause (gsti_t *gsti);
void gstiPlay (gsti_t *gsti);
void gstiStop (gsti_t *gsti);
bool gstiSetPosition (gsti_t *gsti, int64_t pos);
bool gstiSetVolume (gsti_t *gsti, double vol);
bool gstiSetRate (gsti_t *gsti, double rate);

#endif /* INC_GSTI_H */
