#ifndef INC_TAGDEF_H
#define INC_TAGDEF_H

#include <stdbool.h>

#include "bdjstring.h"
#include "datafile.h"
#include "slist.h"

typedef enum {
  DISP_YES,
  DISP_OPT,
  DISP_NO,
} tagdispflag_t;

typedef enum {
  ET_COMBOBOX,
  ET_ENTRY,
  ET_LABEL,
  ET_NA,
  ET_SCALE,
  ET_SPINBOX,
  ET_SPINBOX_TIME,
  ET_SPINBOX_TEXT,
} tagedittype_t;

enum {
  TAG_TYPE_M4A,
  TAG_TYPE_MP3,
  TAG_TYPE_WMA,
  TAG_TYPE_MAX,
};

typedef struct {
  char                *tag;
  char                *displayname;
  char                *audiotags [TAG_TYPE_MAX];
  unsigned int        editWidth;
  tagedittype_t       editType;
  valuetype_t         valueType;
  dfConvFunc_t        convfunc;
  tagdispflag_t       audioiddispflag;
  bool                listingDisplay : 1;
  bool                ellipsize : 1;
  bool                alignRight : 1;
  bool                isBdjTag : 1;
  bool                isNormTag : 1;
  bool                albumEdit : 1;
  bool                allEdit : 1;
  bool                isEditable : 1;
  bool                textSearchable : 1;
  bool                isOrgTag : 1;
} tagdef_t;

typedef enum {
  TAG_ADJUSTFLAGS,
  TAG_AFMODTIME,
  TAG_ALBUM,                  //
  TAG_ALBUMARTIST,            //
  TAG_ARTIST,                 //
  TAG_AUTOORGFLAG,
  TAG_BPM,                    //
  TAG_COMPOSER,
  TAG_CONDUCTOR,
  TAG_DANCE,                  //
  TAG_DANCELEVEL,             //
  TAG_DANCERATING,            //
  TAG_DATE,
  TAG_DBADDDATE,              //
  TAG_DBIDX,                  // not saved
  TAG_DISCNUMBER,
  TAG_DISCTOTAL,
  TAG_DURATION,               //
  TAG_FAVORITE,
  TAG_FILE,                   //
  TAG_GENRE,
  TAG_KEYWORD,                //
  TAG_MQDISPLAY,              //
  TAG_MUSICBRAINZ_TRACKID,
  TAG_NOTES,
  TAG_SAMESONG,
  TAG_SONGEND,                //
  TAG_SONGSTART,              //
  TAG_SPEEDADJUSTMENT,        //
  TAG_STATUS,                 //
  TAG_TAGS,
  TAG_TITLE,
  TAG_TRACKNUMBER,
  TAG_TRACKTOTAL,
  TAG_UPDATEFLAG,
  TAG_UPDATETIME,
  TAG_VOLUMEADJUSTPERC,       //
  TAG_WRITETIME,
  TAG_RRN,                    //
  TAG_KEY_MAX,
} tagdefkey_t;

extern tagdef_t tagdefs [TAG_KEY_MAX];

void        tagdefInit (void);
void        tagdefCleanup (void);
tagdefkey_t tagdefLookup (char *str);

#endif /* INC_TAGDEF_H */
