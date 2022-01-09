#ifndef INC_TAGDEF_H
#define INC_TAGDEF_H

#include <stdbool.h>

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
  unsigned int        defaultEditOrder;
  unsigned int        editIndex;
  unsigned int        editWidth;
  anchor_t            listingAnchor;
  unsigned int        listingWeight;
  anchor_t            songlistAnchor;
  unsigned int        songlistWeight;
  audioiddispflag_t   audioiddispflag;
  edittype_t          editType;
  long                initialized;
  bool                isBdjTag : 1;
  bool                isNormTag : 1;
  bool                albumEdit : 1;
  bool                allEdit : 1;
  bool                isEditable : 1;
  bool                listingDisplay : 1;
  bool                songListDisplay : 1;
  bool                textSearchable : 1;
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
  TAG_KEY_RRN,
  MAX_TAG_KEY
} tagdefkey_t;

#define TAGDEF_INITIALIZED 0x11332244

extern tagdef_t tagdefs[MAX_TAG_KEY];

#endif /* INC_TAGDEF_H */
