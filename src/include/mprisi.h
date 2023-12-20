#ifndef INC_MPRISI_H
#define INC_MPRISI_H

#include <stdint.h>

typedef struct mpris mpris_t;

mpris_t *mprisInit (void);
void mprisFree (mpris_t *mpris);
void mprisMedia (mpris_t *mpris, const char *uri);
int64_t mprisGetPosition (mpris_t *mpris);
const char *mprisPlaybackStatus (mpris_t *mpris);
void mprisPause (mpris_t *mpris);
void mprisPlay (mpris_t *mpris);
void mprisStop (mpris_t *mpris);
void mprisNext (mpris_t *mpris);
bool mprisSetPosition (mpris_t *mpris, int64_t pos);
bool mprisSetVolume (mpris_t *mpris, double vol);
bool mprisSetRate (mpris_t *mpris, double rate);

#endif /* INC_MPRISI_H */
