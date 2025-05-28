/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_TAGDEF_H
#define INC_TAGDEF_H

#include <stdbool.h>

#include "bdjstring.h"
#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  ET_COMBOBOX,
  ET_ENTRY,
  ET_LABEL,
  ET_NA,
  ET_SCALE,
  ET_SPINBOX,
  ET_SPINBOX_TIME,
  ET_SPINBOX_TEXT,
  ET_SWITCH,
} tagedittype_t;

enum {
  TAG_TYPE_VORBIS,    // .ogg, .flac, .opus
  TAG_TYPE_MP4,       // MP4 .m4a, et.al.
  TAG_TYPE_ID3,       // .mp3
  TAG_TYPE_ASF,       // ASF .wma
  TAG_TYPE_RIFF,      // RIFF .wav
  TAG_TYPE_MAX,
};

typedef struct {
  const char  *tag;
  const char  *base;
  const char  *desc;
  const char  *alternate;
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
  bool                listingDisplay : 1;
  /* a secondary display is a special tag to display additional information */
  /* in the song editor */
  bool                secondaryDisplay : 1;
  bool                ellipsize : 1;
  bool                alignend : 1;
  bool                isBDJTag : 1;
  bool                isNormTag : 1;
  bool                allEdit : 1;
  bool                isEditable : 1;
  bool                isAudioID : 1;
  bool                marqueeDisp : 1;
  bool                pluiDisp : 1;
  bool                textSearchable : 1;
  bool                vorbisMulti : 1;
} tagdef_t;

typedef enum {
  TAG_ADJUSTFLAGS,            //  bdj4
  TAG_ALBUM,                  //
  TAG_ALBUMARTIST,            //
  TAG_ARTIST,                 //
  TAG_AUDIOID_IDENT,          //  audio-id
  TAG_AUDIOID_SCORE,          //  audio-id
  TAG_BPM,                    //
  TAG_BPM_DISPLAY,            //
  TAG_COMMENT,                //
  TAG_COMPOSER,               //
  TAG_CONDUCTOR,              //
  TAG_DANCE,                  //  bdj4
  TAG_DANCELEVEL,             //  bdj4
  TAG_DANCERATING,            //  bdj4
  TAG_DATE,                   //
  TAG_DBADDDATE,              // only in the database, treated as special case
  TAG_DBIDX,                  // internal: not saved to db or af
  TAG_DB_FLAGS,               // internal: not saved to db or af
  TAG_DB_LOC_LOCK,            // only in the database
  TAG_DISCNUMBER,             //
  TAG_DISCTOTAL,              //
  TAG_DURATION,               // not saved to af
  TAG_FAVORITE,               //
  TAG_GENRE,                  //
  TAG_GROUPING,               //
  TAG_KEYWORD,                //
  TAG_LAST_UPDATED,           //  internal
  TAG_MOVEMENTCOUNT,          //
  TAG_MOVEMENTNAME,           //
  TAG_MOVEMENTNUM,            //
  TAG_MQDISPLAY,              //  bdj4
  TAG_NOTES,                  //  bdj4
  TAG_NO_PLAY_TM_LIMIT,         //  bdj4
  TAG_PREFIX_LEN,             // used for secondary directories
  TAG_RECORDING_ID,           // musicbrainz_trackid
  TAG_RRN,                    // internal: not saved to db or af
  TAG_SAMESONG,               //  bdj4  internal: only in the db
  TAG_SHOWMOVEMENT,           //
  TAG_SONGEND,                //  bdj4
  TAG_SONGSTART,              //  bdj4
  TAG_SONG_TYPE,              //  bdj4 : norm, podcast, remote
  TAG_SORT_ALBUM,
  TAG_SORT_ALBUMARTIST,
  TAG_SORT_ARTIST,
  TAG_SORT_COMPOSER,
  TAG_SORT_TITLE,
  TAG_SPEEDADJUSTMENT,        //  bdj4
  TAG_STATUS,                 //  bdj4
  TAG_TAGS,                   //  bdj4
  TAG_TITLE,                  //
  TAG_TRACKNUMBER,            //
  TAG_TRACKTOTAL,             //
  TAG_TRACK_ID,               // musicbrainz_releasetrackid
  TAG_URI,                    //
  TAG_VOLUMEADJUSTPERC,       //  bdj4
  TAG_WORK,                   //
  TAG_WORK_ID,                // musicbrainz_workid
  TAG_KEY_MAX,
} tagdefkey_t;

extern tagdef_t tagdefs [TAG_KEY_MAX];

void        tagdefInit (void);
void        tagdefCleanup (void);
int         tagdefLookup (const char *str);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_TAGDEF_H */
