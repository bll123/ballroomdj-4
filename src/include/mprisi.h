/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_MPRISI_H
#define INC_MPRISI_H

#include <stdint.h>

#include "pli.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct mpris mpris_t;

int mprisGetPlayerList (mpris_t *mpris, char **ret, int max);
void mprisCleanup (void);
mpris_t *mprisInit (const char *plinm);
void mprisFree (mpris_t *mpris);
bool mprisCanSeek (mpris_t *mpris);
bool mprisHasSpeed (mpris_t *mpris);
void mprisMedia (mpris_t *mpris, const char *fulluri, int sourceType);
int64_t mprisGetDuration (mpris_t *mpris);
int64_t mprisGetPosition (mpris_t *mpris);
plistate_t mprisState (mpris_t *mpris);
void mprisPause (mpris_t *mpris);
void mprisPlay (mpris_t *mpris);
void mprisStop (mpris_t *mpris);
bool mprisSetPosition (mpris_t *mpris, int64_t pos);
bool mprisSetVolume (mpris_t *mpris, double vol);
bool mprisSetRate (mpris_t *mpris, double rate);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_MPRISI_H */
