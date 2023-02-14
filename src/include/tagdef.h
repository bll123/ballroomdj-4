/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  TAG_TYPE_VORBIS,    // .ogg, .flac, et. al.
  TAG_TYPE_MP4,     // .m4a
  TAG_TYPE_MP3,
  TAG_TYPE_WMA,
  TAG_TYPE_MAX,
};

typedef struct {
  const char  *tag;
  const char  *base;
  const char  *desc;
} tagaudiotag_t;

typedef struct {
  const char          *tag;
  const char          *displayname;
  const char          *shortdisplayname;
  tagaudiotag_t       audiotags [TAG_TYPE_MAX];
  const char          *itunesName;
  tagedittype_t       editType;
  valuetype_t         valueType;
  dfConvFunc_t        convfunc;
  tagdispflag_t       audioiddispflag;
  bool                listingDisplay : 1;
  bool                secondaryDisplay : 1;
  bool                ellipsize : 1;
  bool                alignRight : 1;
  bool                isBDJTag : 1;
  bool                isNormTag : 1;
  bool                allEdit : 1;
  bool                isEditable : 1;
  bool                textSearchable : 1;
  bool                isOrgTag : 1;
} tagdef_t;

typedef enum {
  TAG_ADJUSTFLAGS,            //
  TAG_ALBUM,                  //
  TAG_ALBUMARTIST,            //
  TAG_ARTIST,                 //
  TAG_BPM,                    //
  TAG_BPM_DISPLAY,            //
  TAG_COMPOSER,               //
  TAG_CONDUCTOR,              //
  TAG_DANCE,                  //
  TAG_DANCELEVEL,             //
  TAG_DANCERATING,            //
  TAG_DATE,                   //
  TAG_DBADDDATE,              //
  TAG_DBIDX,                  // not saved
  TAG_DISCNUMBER,             //
  TAG_DISCTOTAL,              //
  TAG_DURATION,               // not saved to af
  TAG_FAVORITE,               //
  TAG_FILE,                   //
  TAG_GENRE,                  //
  TAG_KEYWORD,                //
  TAG_MQDISPLAY,              //
  TAG_NOTES,                  //
  TAG_RECORDING_ID,           // musicbrainz_trackid
  TAG_RRN,                    // not save to af
  TAG_SAMESONG,               //
  TAG_SONGEND,                //
  TAG_SONGSTART,              //
  TAG_SPEEDADJUSTMENT,        //
  TAG_STATUS,                 //
  TAG_TAGS,                   //
  TAG_TEMPORARY,              //
  TAG_TITLE,                  //
  TAG_TRACK_ID,               // musicbrainz_releasetrackid
  TAG_TRACKNUMBER,            //
  TAG_TRACKTOTAL,             //
  TAG_LAST_UPDATED,           //
  TAG_VOLUMEADJUSTPERC,       //
  TAG_WORK_ID,                // musicbrainz_workid
  TAG_KEY_MAX,
} tagdefkey_t;

enum {
  /* this can be approximate.  it controls how many itunes fields are */
  /* placed in the first column in the configuration ui */
  TAG_ITUNES_MAX = 18,
};

extern tagdef_t tagdefs [TAG_KEY_MAX];

void        tagdefInit (void);
void        tagdefCleanup (void);
tagdefkey_t tagdefLookup (const char *str);

#endif /* INC_TAGDEF_H */
