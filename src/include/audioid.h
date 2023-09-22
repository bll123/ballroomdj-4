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

typedef enum {
  AUDIOID_ID_ACOUSTID,
  AUDIOID_ID_MB_LOOKUP,
  AUDIOID_ID_ACRCLOUD,
  AUDIOID_ID_MAX,
} audioid_id_t;

enum {
  AUDIOID_TYPE_TREE = TAG_KEY_MAX,
  AUDIOID_TYPE_JOINPHRASE = TAG_KEY_MAX + 1,
  AUDIOID_TYPE_MONTH = TAG_KEY_MAX + 2,
  AUDIOID_TYPE_RESPIDX = TAG_KEY_MAX + 3,
  AUDIOID_TYPE_JOINED = TAG_KEY_MAX + 4,
  AUDIOID_TYPE_IDENT = TAG_KEY_MAX + 5,
};

void audioidParseInit (void);
void audioidParseCleanup (void);
int audioidParseAll (const char *data, size_t datalen, audioidxpath_t *xpaths, ilist_t *respdata, audioid_id_t ident);

/* musicbrainz.c */

audioidmb_t *mbInit (void);
void mbFree (audioidmb_t *mb);
int mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata);

/* acoustid.c */

audioidacoustid_t * acoustidInit (void);
void acoustidFree (audioidacoustid_t *acoustid);
int acoustidLookup (audioidacoustid_t *acoustid, const song_t *song, ilist_t *respdata);

/* acrcloud.c */

audioidacr_t * acrInit (void);
void acrFree (audioidacr_t *acr);
int acrLookup (audioidacr_t *acr, const song_t *song, ilist_t *respdata);

#endif /* INC_AUDIOID_H */
