#ifndef INC_MSGPARSE_H
#define INC_MSGPARSE_H

#include "nlist.h"
#include "musicdb.h"

/* used for music queue update processing */
typedef struct {
  dbidx_t   dbidx;
  int       dispidx;
  int       uniqueidx;
  int       pflag;
} mp_musicqupditem_t;

typedef struct {
  int             mqidx;
  long            tottime;
  nlist_t         *dispList;
} mp_musicqupdate_t;

typedef struct {
  int             mqidx;
  int             loc;
} mp_songselect_t;

mp_musicqupdate_t *msgparseMusicQueueData (char * args);
void  msgparseMusicQueueDataFree (mp_musicqupdate_t *musicqupdate);
mp_songselect_t *msgparseSongSelect (char * args);
void msgparseSongSelectFree (mp_songselect_t *songselect);

#endif /* INC_MSGPARSE_H */
