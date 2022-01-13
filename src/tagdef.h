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
  TAG_ADJUSTFLAGS,
  TAG_AFMODTIME,
  TAG_ALBUM,
  TAG_ALBUMARTIST,
  TAG_ARTIST,
  TAG_AUTOORGFLAG,
  TAG_BPM,
  TAG_COMPOSER,
  TAG_CONDUCTOR,
  TAG_DANCE,
  TAG_DANCELEVEL,             //
  TAG_DANCERATING,            //
  TAG_DATE,
  TAG_DBADDDATE,
  TAG_DISCNUMBER,
  TAG_DISCTOTAL,
  TAG_DISPLAYIMG,
  TAG_DURATION,               //
  TAG_FILE,                   //
  TAG_GENRE,
  TAG_KEYWORD,                //
  TAG_MQDISPLAY,
  TAG_MUSICBRAINZ_TRACKID,
  TAG_NOMAXPLAYTIME,
  TAG_NOTES,
  TAG_SAMESONG,
  TAG_SONGEND,
  TAG_SONGSTART,
  TAG_SPEEDADJUSTMENT,
  TAG_STATUS,                 //
  TAG_TAGS,
  TAG_TITLE,
  TAG_TRACKNUMBER,
  TAG_TRACKTOTAL,
  TAG_UPDATEFLAG,
  TAG_UPDATETIME,
  TAG_WRITETIME,
  TAG_VARIOUSARTISTS,
  TAG_VOLUMEADJUSTPERC,
  TAG_RRN,                    //
  MAX_TAG_KEY
} tagdefkey_t;

extern tagdef_t tagdefs[MAX_TAG_KEY];

#endif /* INC_TAGDEF_H */
