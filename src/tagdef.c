#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "tagdef.h"

tagdef_t tagdefs[MAX_TAG_KEY] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_AFMODTIME] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_ALBUM] =
  { "ALBUM",                      /* tag */
    { "©alb", "TALB", "WM/AlbumTitle" },       /* audio tags */
    0,                            /* default edit order   */
    7,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_ALBUMARTIST] =
  { "ALBUMARTIST",                /* tag */
    { "aART", "TPE2", "WM/AlbumArtist" },       /* audio tags */
    1,                            /* default edit order   */
    4,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_ARTIST] =
  { "ARTIST",                     /* tag */
    { "©ART", "TPE1", "WM/Author" },       /* audio tags */
    3,                            /* default edit order   */
    5,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_AUTOORGFLAG] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    { "tmpo", "TBPM", "WM/BeatsPerMinute" },       /* audio tags */
    0,                            /* default edit order   */
    15,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_COMPOSER] =
  { "COMPOSER",                   /* tag */
    { "©wrt", "TCOM", "WM/Composer" },       /* audio tags */
    0,                            /* default edit order   */
    2,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_CONDUCTOR] =
  { "CONDUCTOR",                  /* tag */
    { NULL, "TPE3", "WM/Conductor" },       /* audio tags */
    0,                            /* default edit order   */
    3,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_DANCE] =
  { "DANCE",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    5,                            /* default edit order   */
    9,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    1,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DANCELEVEL] =
  { "DANCELEVEL",                 /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    7,                            /* default edit order   */
    11,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DANCERATING] =
  { "DANCERATING",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    6,                            /* default edit order   */
    10,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DATE] =
  { "DATE",                       /* tag */
    { "©day", "TYER", "WM/Year" },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DBADDDATE] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DISCNUMBER] =
  { "DISCNUMBER",                 /* tag */
    { "disk", "TPOS", "WM/PartOfSet" },       /* audio tags */
    13,                           /* default edit order   */
    13,                           /* edit index           */
    5,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DISCTOTAL] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DURATION] =
  { "DURATION",                   /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    9,                            /* default edit order   */
    16,                           /* edit index           */
    15,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_FILE] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_FAVORITE] =
  { "FAVORITE",                   /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_GENRE] =
  { "GENRE",                      /* tag */
    { "gnre", "TCON", "WM/Genre" },       /* audio tags */
    1,                            /* default edit order   */
    1,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_KEYWORD] =
  { "KEYWORD",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    20,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_MQDISPLAY] =
  { "MQDISPLAY",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    20,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_MUSICBRAINZ_TRACKID] =
  { "MUSICBRAINZ_TRACKID",                /* tag */
    { NULL, "UFID", "MusicBrainz/Track Id" },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_NOMAXPLAYTIME] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_CHECKBUTTON,               /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_NOTES] =
  { "NOTES",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    8,                            /* default edit order   */
    12,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_SAMESONG] =
  { "SAMESONG",                 /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_SONGEND] =
  { "SONGEND",                /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    10,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_SONGSTART] =
  { "SONGSTART",                  /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    10,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_SPEEDADJUSTMENT] =
  { "SPEEDADJUSTMENT",            /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_SCALE,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_STATUS] =
  { "STATUS",                     /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    17,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_TAGS] =
  { "TAGS",                       /* tag */
    { "keyw", "TXXX", NULL },       /* audio tags */
    8,                            /* default edit order   */
    12,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_TITLE] =
  { "TITLE",                      /* tag */
    { "©nam", "TIT2", "WM/Title" },       /* audio tags */
    4,                            /* default edit order   */
    8,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    10,                           /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_TRACKNUMBER] =
  { "TRACKNUMBER",                /* tag */
    { "trkn", "TRCK", "WM/TrackNumber" },       /* audio tags */
    0,                            /* default edit order   */
    14,                           /* edit index           */
    5,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    1,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_TRACKTOTAL] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_VARIOUSARTISTS] =
  { "VARIOUSARTISTS",                 /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    6,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_CHECKBUTTON,               /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_UPDATEFLAG] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_UPDATETIME] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_VOLUMEADJUSTPERC] =
  { "VOLUMEADJUSTPERC",           /* tag */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_SCALE,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_WRITETIME] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_RRN] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DBIDX] =
  { NULL,                         /* tag */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
};

