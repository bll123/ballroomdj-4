#ifndef INC_TAGDEF_H
#define INC_TAGDEF_H

#include "bdjstring.h"

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
  unsigned int        isBdjTag : 1;
  unsigned int        isNormTag : 1;
  unsigned int        albumEdit : 1;
  unsigned int        allEdit : 1;
  unsigned int        isEditable : 1;
  unsigned int        listingDisplay : 1;
  unsigned int        songListDisplay : 1;
  unsigned int        textSearchable : 1;
} tagdef_t;

#define TAG_KEY_ADJUSTFLAGS   0
#define TAG_ADJUSTFLAGS       "ADJUSTFLAGS"
#define TAG_KEY_AFMODTIME     1
#define TAG_AFMODTIME         "AFMODTIME"
#define TAG_KEY_ALBUM         2
#define TAG_ALBUM             "ALBUM"
#define TAG_KEY_ALBUMARTIST   3
#define TAG_ALBUMARTIST       "ALBUMARTIST"
#define TAG_KEY_ARTIST        4
#define TAG_ARTIST            "ARTIST"
#define TAG_KEY_BDJSYNCID     5
#define TAG_BDJSYNCID         "BDJSYNCID"
#define TAG_KEY_BPM           6
#define TAG_BPM               "BPM"
#define TAG_KEY_COMPOSER      7
#define TAG_COMPOSER          "COMPOSER"
#define TAG_KEY_CONDUCTOR     8
#define TAG_CONDUCTOR         "CONDUCTOR"
#define TAG_KEY_DANCE         9
#define TAG_DANCE             "DANCE"
#define TAG_KEY_DANCELEVEL    10
#define TAG_DANCELEVEL        "DANCELEVEL"
#define TAG_KEY_DANCERATING   11
#define TAG_DANCERATING       "DANCERATING"
#define TAG_KEY_DATE          12
#define TAG_DATE              "DATE"
#define TAG_KEY_DBADDDATE     13
#define TAG_DBADDDATE         "DBADDDATE"
#define TAG_KEY_DISCNUMBER    14
#define TAG_DISCNUMBER        "DISCNUMBER"
#define TAG_KEY_DISCTOTAL     15
#define TAG_DISCTOTAL         "DISCTOTAL"
#define TAG_KEY_DISPLAYIMG    16
#define TAG_DISPLAYIMG        "DISPLAYIMG"
#define TAG_KEY_DURATION      17
#define TAG_DURATION          "DURATION"
#define TAG_KEY_DURATION_STR  18
#define TAG_DURATION_STR      "DURATION_STR"
#define TAG_KEY_FILE          19
#define TAG_FILE              "FILE"
#define TAG_KEY_GENRE         20
#define TAG_GENRE             "GENRE"
#define TAG_KEY_KEYWORD       21
#define TAG_KEYWORD           "KEYWORD"
#define TAG_KEY_MQDISPLAY     22
#define TAG_MQDISPLAY         "MQDISPLAY"
#define TAG_KEY_MUSICBRAINZ_TRACKID 23
#define TAG_MUSICBRAINZ_TRACKID "MUSICBRAINZ_TRACKID"
#define TAG_KEY_NOMAXPLAYTIME 24
#define TAG_NOMAXPLAYTIME     "NOMAXPLAYTIME"
#define TAG_KEY_NOTES         25
#define TAG_NOTES             "NOTES"
#define TAG_KEY_SAMESONG      26
#define TAG_SAMESONG          "SAMESONG"
#define TAG_KEY_SONGEND       27
#define TAG_SONGEND           "SONGEND"
#define TAG_KEY_SONGSTART     28
#define TAG_SONGSTART         "SONGSTART"
#define TAG_KEY_SPEEDADJUSTMENT 29
#define TAG_SPEEDADJUSTMENT   "SPEEDADJUSTMENT"
#define TAG_KEY_STATUS        30
#define TAG_STATUS            "STATUS"
#define TAG_KEY_TAGS          31
#define TAG_TAGS              "TAGS"
#define TAG_KEY_TITLE         32
#define TAG_TITLE             "TITLE"
#define TAG_KEY_TRACKNUMBER   33
#define TAG_TRACKNUMBER       "TRACKNUMBER"
#define TAG_KEY_TRACKTOTAL    34
#define TAG_TRACKTOTAL        "TRACKTOTAL"
#define TAG_KEY_VARIOUSARTISTS 35
#define TAG_VARIOUSARTISTS    "VARIOUSARTISTS"
#define TAG_KEY_VOLUMEADJUSTPERC 36
#define TAG_VOLUMEADJUSTPERC  "VOLUMEADJUSTPERC"

extern tagdef_t tagdefs[];

void tagdefInit (void);

#endif /* INC_TAGDEF_H */
