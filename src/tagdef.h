#ifndef INC_TAGDEF_H
#define INC_TAGDEF_H

#include "bdjstring.h"
#include "list.h"

typedef enum {
  AIDD_YES,
  AIDD_OPT,
  AIDD_NO
} audioiddispflag_t;

typedef enum {
  ET_SCALE,
  ET_CHECKBUTTON,
  ET_COMBOBOX,
  ET_ENTRY,
  ET_NA,
  ET_DISABLED_ENTRY
} edittype_t;

typedef enum {
  ANCHOR_WEST,
  ANCHOR_EAST,
  ANCHOR_BOTH
} anchor_t;

typedef struct {
  char                *name;
  char                *label;
  char                *shortLabel;
  unsigned int        defaultEditOrder;
  unsigned int        editIndex;
  unsigned int        editWidth;
  anchor_t            listingAnchor;
  unsigned int        listingWeight;
  anchor_t            songlistAnchor;
  unsigned int        songlistWeight;
  audioiddispflag_t   audioiddispflag;
  edittype_t          editType;
  valuetype_t         valuetype;
  long                initialized;
  unsigned int        isBdjTag : 1;
  unsigned int        isNormTag : 1;
  unsigned int        albumEdit : 1;
  unsigned int        allEdit : 1;
  unsigned int        isEditable : 1;
  unsigned int        listingDisplay : 1;
  unsigned int        songListDisplay : 1;
  unsigned int        textSearchable : 1;
} tagdef_t;

typedef enum {
  TAG_KEY_ADJUSTFLAGS,
  TAG_KEY_AFMODTIME,
  TAG_KEY_ALBUM,
  TAG_KEY_ALBUMARTIST,
  TAG_KEY_ARTIST,
  TAG_KEY_AUTOORGFLAG,
  TAG_KEY_BPM,
  TAG_KEY_COMPOSER,
  TAG_KEY_CONDUCTOR,
  TAG_KEY_DANCE,
  TAG_KEY_DANCELEVEL,
  TAG_KEY_DANCERATING,
  TAG_KEY_DATE,
  TAG_KEY_DBADDDATE,
  TAG_KEY_DISCNUMBER,
  TAG_KEY_DISCTOTAL,
  TAG_KEY_DISPLAYIMG,
  TAG_KEY_DURATION,
  TAG_KEY_DURATION_HMS,
  TAG_KEY_DURATION_STR,
  TAG_KEY_FILE,
  TAG_KEY_GENRE,
  TAG_KEY_KEYWORD,
  TAG_KEY_MQDISPLAY,
  TAG_KEY_MUSICBRAINZ_TRACKID,
  TAG_KEY_NOMAXPLAYTIME,
  TAG_KEY_NOTES,
  TAG_KEY_SAMESONG,
  TAG_KEY_SONGEND,
  TAG_KEY_SONGSTART,
  TAG_KEY_SPEEDADJUSTMENT,
  TAG_KEY_STATUS,
  TAG_KEY_TAGS,
  TAG_KEY_TITLE,
  TAG_KEY_TRACKNUMBER,
  TAG_KEY_TRACKTOTAL,
  TAG_KEY_UPDATEFLAG,
  TAG_KEY_UPDATETIME,
  TAG_KEY_WRITETIME,
  TAG_KEY_VARIOUSARTISTS,
  TAG_KEY_VOLUMEADJUSTPERC,
  TAG_KEY_rrn,
  TAG_KEY_dur,
  MAX_TAG_KEY
} tagdefkey_t;

#define TAGDEF_INITIALIZED 0x11332244

extern tagdef_t tagdefs[MAX_TAG_KEY];

void          tagdefInit (void);
tagdefkey_t   tagdefGetIdx (char *);
void          tagdefCleanup (void);

#endif /* INC_TAGDEF_H */
