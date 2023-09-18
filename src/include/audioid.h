#ifndef INC_AUDIOID_H
#define INC_AUDIOID_H

#include "ilist.h"
#include "nlist.h"
#include "song.h"
#include "tagdef.h"

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

/* audioidutil.c */

typedef struct audioidxpath audioidxpath_t;
typedef struct audioidxpath {
  int             flag;
  int             tagidx;
  const char      *xpath;
  const char      *attr;
  audioidxpath_t  *tree;
} audioidxpath_t;

enum {
  AUDIOID_XPATH_TREE,
  AUDIOID_XPATH_DATA,
  AUDIOID_XPATH_END,
};

enum {
  AUDIOID_TYPE_TREE = TAG_KEY_MAX,
  AUDIOID_TYPE_JOINPHRASE = TAG_KEY_MAX + 1,
  AUDIOID_TYPE_YEAR = TAG_KEY_MAX + 2,
  AUDIOID_TYPE_MONTH = TAG_KEY_MAX + 3,
  AUDIOID_TYPE_RESPONSE = TAG_KEY_MAX + 4,
};

void audioidParseInit (void);
void audioidParseCleanup (void);
int audioidParseAll (const char *data, size_t datalen, audioidxpath_t *xpaths, ilist_t *respdata);

/* musicbrainz.c */

audioidmb_t *mbInit (void);
void mbFree (audioidmb_t *mb);
void mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata);

/* acoustid.c */

audioidacoustid_t * acoustidInit (void);
void acoustidFree (audioidacoustid_t *acoustid);
void acoustidLookup (audioidacoustid_t *acoustid, const song_t *song, ilist_t *respdata);

/* acrcloud.c */

audioidacr_t * acrInit (void);
void acrFree (audioidacr_t *acr);
void acrLookup (audioidacr_t *acr, const song_t *song, ilist_t *respdata);

#endif /* INC_AUDIOID_H */
