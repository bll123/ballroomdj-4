#ifndef INC_AUDIOID_H
#define INC_AUDIOID_H

#include "ilist.h"
#include "nlist.h"
#include "song.h"

typedef struct audioid audioid_t;
typedef struct audioidmb audioidmb_t;
typedef struct audioidacoustid audioidacoustid_t;
typedef struct audioidacr audioidacr_t;

/* audioid.c */

audioid_t *audioidInit (void);
void audioidFree (audioid_t *audioid);
bool audioidLookup (audioid_t *audioid, const song_t *song);
void audioidStartIterator (audioid_t *audioid);
ilistidx_t audioidIterate (audioid_t *audioid);
nlist_t * audioidGetList (audioid_t *audioid, int key);

/* musicbrainz.c */

audioidmb_t *mbInit (void);
void mbFree (audioidmb_t *mb);
void mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata);

/* acoustid.c */

audioidacoustid_t * acoustidInit (void);
void acoustidFree (audioidacoustid_t *acoustid);
void acoustidLookup (audioidacoustid_t *acoustid, song_t *song, ilist_t *respdata);

/* acrcloud.c */

audioidacr_t * acrInit (void);
void acrFree (audioidacr_t *acr);
void acrLookup (audioidacr_t *acr, song_t *song, ilist_t *respdata);

#endif /* INC_AUDIOID_H */
