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
bool audioidSetResponseData (int level, ilist_t *resp, int respidx, int tagidx, const char *data, const char *joinphrase);

typedef struct audioidparsedata
{
  int         tagidx;
  const char  *name;
} audioidparsedata_t;

typedef struct audioidparse audioidparse_t;
typedef struct audioidparse {
  int                 flag;
  int                 tagidx;
  const char          *name;
  const char          *attr;
  audioidparse_t      *tree;  /* json: object */
  audioidparsedata_t  *jdata;
} audioidparse_t;

enum {
  AUDIOID_PARSE_ARRAY,
  AUDIOID_PARSE_DATA_ARRAY,
  AUDIOID_PARSE_TREE,
  AUDIOID_PARSE_DATA,
  AUDIOID_PARSE_SET,
  AUDIOID_PARSE_END,
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
  AUDIOID_TYPE_IDENT = TAG_KEY_MAX + 4,
  AUDIOID_TYPE_STATUS_CODE = TAG_KEY_MAX + 5,
  AUDIOID_TYPE_STATUS_MSG = TAG_KEY_MAX + 6,
  AUDIOID_TYPE_ARRAY = TAG_KEY_MAX + 7,
  AUDIOID_TYPE_ROLE = TAG_KEY_MAX + 8,
};

/* audioidxml.c */

void audioidParseXMLInit (void);
void audioidParseXMLCleanup (void);
int audioidParseXMLAll (const char *data, size_t datalen, audioidparse_t *xpaths, ilist_t *respdata, audioid_id_t ident);

/* audioidson.c */

int audioidParseJSONAll (const char *data, size_t datalen, audioidparse_t *xpaths, ilist_t *respdata, audioid_id_t ident);

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
