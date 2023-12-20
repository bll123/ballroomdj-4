#ifndef INC_MPRISI_H
#define INC_MPRISI_H

#include <stdint.h>

typedef struct mpris mpris_t;

mpris_t *mprisInit (void);
void mprisFree (mpris_t *mpris);
int64_t mprisGetPosition (mpris_t *mpris);
void mprisMedia (mpris_t *mpris, const char *uri);
void mprisPause (mpris_t *mpris);
void mprisPlay (mpris_t *mpris);
void mprisStop (mpris_t *mpris);
void mprisNext (mpris_t *mpris);

#endif /* INC_MPRISI_H */
