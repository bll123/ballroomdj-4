#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "dance.h"
#include "datafile.h"
#include "genre.h"
#include "level.h"
#include "rating.h"
#include "slist.h"
#include "songfav.h"
#include "status.h"
#include "tagdef.h"

/* as of 2022-6-13 the tag names may no longer match the vorbis tag names */
/* the tag name is used in the internal database and for debugging */

tagdef_t tagdefs [TAG_KEY_MAX] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    NULL,                         /* display name         */
    { "ADJUSTFLAGS",
      NULL,
      "TXXX=ADJUSTFLAGS",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */ // ###
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_AFMODTIME] =
  { "AFMODTIME",                  /* tag */
    NULL,                         /* display name         */
    { "AFMODTIME",
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_ALBUM] =
  { "ALBUM",                      /* tag */
    NULL,                         /* display name         */
    { "ALBUM",
      "©alb",
      "TALB",
      "WM/AlbumTitle"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_ALBUMARTIST] =
  { "ALBUMARTIST",                /* tag */
    NULL,                         /* display name         */
    { "ALBUMARTIST",
      "aART",
      "TPE2",
      "WM/AlbumArtist"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_ARTIST] =
  { "ARTIST",                     /* tag */
    NULL,                         /* display name         */
    { "ARTIST",
      "©ART",
      "TPE1",
      "WM/Author"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    NULL,                         /* display name         */
    { "BPM",
      "tmpo",
      "TBPM",
      "WM/BeatsPerMinute"
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    1,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_COMPOSER] =
  { "COMPOSER",                   /* tag */
    NULL,                         /* display name         */
    { "COMPOSER",
      "©wrt",
      "TCOM",
      "WM/Composer"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_CONDUCTOR] =
  { "CONDUCTOR",                  /* tag */
    NULL,                         /* display name         */
    { "CONDUCTOR",
      "----:com.apple.iTunes:CONDUCTOR",
      "TPE3",
      "WM/Conductor"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_DANCE] =
  { "DANCE",                /* tag */
    NULL,                         /* display name         */
    { "DANCE",
      NULL,
      "TXXX=DANCE",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    danceConvDance,               /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_DANCELEVEL] =
  { "DANCELEVEL",                 /* tag */
    NULL,                         /* display name         */
    { "DANCELEVEL",
      NULL,
      "TXXX=DANCELEVEL",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    levelConv,                    /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DANCERATING] =
  { "DANCERATING",                /* tag */
    NULL,                         /* display name         */
    { "DANCERATING",
      NULL,
      "TXXX=DANCERATING",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    ratingConv,                   /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DATE] =
  { "DATE",                       /* tag */
    NULL,                         /* display name         */
    { "DATE",
      "©day",
      "TYER",
      "WM/Year"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DBADDDATE] =
  { "DBADDDATE",                  /* tag */
    NULL,                         /* display name         */
    { "DBADDDATE",
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DISCNUMBER] =
  { "DISC",                       /* tag */
    NULL,                         /* display name         */
    { "DISCNUMBER",
      "disk",
      "TPOS",
      "WM/PartOfSet"
    },       /* audio tags */
    10,                           /* edit width           */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_DISCTOTAL] =
  { "DISCTOTAL",                  /* tag */
    NULL,                         /* display name         */
    { "DISCTOTAL",
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DURATION] =
  { "DURATION",                   /* tag */
    NULL,                         /* display name         */
    { "DURATION",
      NULL,
      "TXXX=DURATION",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_LABEL,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    convMS,                       /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_FILE] =
  { "FILE",                       /* tag */
    NULL,                         /* display name         */
    { NULL,
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_FAVORITE] =
  { "FAVORITE",                   /* tag */
    NULL,                         /* display name         */
    { "FAVORITE",
      NULL,
      "TXXX=FAVORITE",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    songConvFavorite,             /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_GENRE] =
  { "GENRE",                      /* tag */
    NULL,                         /* display name         */
    { "GENRE",
      "©gen",
      "TCON",
      "WM/Genre"
    },       /* audio tags */
    0,                            /* edit width           */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    genreConv,                    /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_KEYWORD] =
  { "KEYWORD",                /* tag */
    NULL,                         /* display name         */
    { "KEYWORD",
      NULL,
      "TXXX=KEYWORD",
      NULL
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_MQDISPLAY] =
  { "MQDISPLAY",                /* tag */
    NULL,                         /* display name         */
    { "MQDISPLAY",
      NULL,
      "TXXX=MQDISPLAY",
      NULL
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_RECORDING_ID] =
  { "RECORDING_ID",                /* tag */
    NULL,                         /* display name         */
    { "MUSICBRAINZ_TRACKID",
      "----:com.apple.iTunes:MusicBrainz Track Id",
      "UFID=http://musicbrainz.org",
      "MusicBrainz/Track Id"
    },       /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_WORK_ID] =
  { "WORK_ID",
    NULL,                         /* display name         */
    { "MUSICBRAINZ_WORKID",
      "----:com.apple.iTunes:MusicBrainz Work Id",
      "TXXX=MusicBrainz Work Id",
      "MusicBrainz/Work Id"
    },       /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_TRACK_ID] =
  { "TRACK_ID",                    /* tag */
    NULL,                         /* display name         */
    { "MUSICBRAINZ_RELEASETRACKID",
      "----:com.apple.iTunes:MusicBrainz Release Track Id",
      "TXXX=MusicBrainz Release Track Id",
      "MusicBrainz/Release Track Id"
    },  /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_NOTES] =
  { "NOTES",                /* tag */
    NULL,                         /* display name         */
    { "NOTES",
      NULL,
      "TXXX=NOTES",
      NULL
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_SAMESONG] =
  { "SAMESONG",                 /* tag */
    NULL,                         /* display name         */
    { "SAMESONG",
      NULL,
      "TXXX=SAMESONG",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_SONGEND] =
  { "SONGEND",                /* tag */
    NULL,                         /* display name         */
    { "SONGEND",
      NULL,
      "TXXX=SONGEND",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_SONGSTART] =
  { "SONGSTART",                  /* tag */
    NULL,                         /* display name         */
    { "SONGSTART",
      NULL,
      "TXXX=SONGSTART",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_SPEEDADJUSTMENT] =
  { "SPEEDADJUSTMENT",            /* tag */
    NULL,                         /* display name         */
    { "SPEEDADJUSTMENT",
      NULL,
      "TXXX=SPEEDADJUSTMENT",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SCALE,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_STATUS] =
  { "STATUS",                     /* tag */
    NULL,                         /* display name         */
    { "STATUS",
      NULL,
      "TXXX=STATUS",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    statusConv,                   /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_TAGS] =
  { "TAGS",                       /* tag */
    NULL,                         /* display name         */
    { "TAGS",
      "keyw",
      "TXXX=TAGS",
      NULL
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_LIST,                   /* value type           */
    convTextList,                 /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_TITLE] =
  { "TITLE",                      /* tag */
    NULL,                         /* display name         */
    { "TITLE",
      "©nam",
      "TIT2",
      "WM/Title"
    },       /* audio tags */
    20,                           /* edit width           */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_TRACKNUMBER] =
  { "TRACKNUMBER",                /* tag */
    NULL,                         /* display name         */
    { "TRACKNUMBER",
      "trkn",
      "TRCK",
      "WM/TrackNumber"
    },       /* audio tags */
    10,                           /* edit width           */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_TRACKTOTAL] =
  { "TRACKTOTAL",                 /* tag */
    NULL,                         /* display name         */
    { "TRACKTOTAL",
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    1,                            /* align right          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_LAST_UPDATED] =
  { "LASTUPDATED",                /* tag */
    NULL,                         /* display name         */
    { NULL,
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_VOLUMEADJUSTPERC] =
  { "VOLUMEADJUSTPERC",           /* tag */
    NULL,                         /* display name         */
    { "VOLUMEADJUSTPERC",
      NULL,
      "TXXX=VOLUMEADJUSTPERC",
      NULL
    },       /* audio tags */
    0,                            /* edit width           */
    ET_SCALE,                     /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_RRN] =
  { "RRN",                        /* tag */
    NULL,                         /* display name         */
    { NULL,
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DBIDX] =
  { "DBIDX",                      /* tag */
    NULL,                         /* display name         */
    { NULL,
      NULL,
      NULL,
      NULL
    },         /* audio tags */
    0,                            /* edit width           */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* ellipsize            */
    0,                            /* align right          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
};

typedef struct {
  slist_t   *taglookup;
  bool      initialized : 1;
} tagdefinfo_t;

static tagdefinfo_t   tagdefinfo = {
  NULL,
  false,
};

void
tagdefInit (void)
{
  if (tagdefinfo.initialized) {
    return;
  }
  tagdefinfo.initialized = true;

  /* CONTEXT: label: audio file modification time */
  tagdefs [TAG_AFMODTIME].displayname = _("Audio File Date");
  /* CONTEXT: label: album */
  tagdefs [TAG_ALBUM].displayname = _("Album");
  /* CONTEXT: label: album artist */
  tagdefs [TAG_ALBUMARTIST].displayname = _("Album Artist");
  /* CONTEXT: label: artist */
  tagdefs [TAG_ARTIST].displayname = _("Artist");
  /* CONTEXT: label: composer */
  tagdefs [TAG_COMPOSER].displayname = _("Composer");
  /* CONTEXT: label: conductor */
  tagdefs [TAG_CONDUCTOR].displayname = _("Conductor");
  /* CONTEXT: label: dance */
  tagdefs [TAG_DANCE].displayname = _("Dance");
  /* CONTEXT: label: dance level */
  tagdefs [TAG_DANCELEVEL].displayname = _("Dance Level");
  /* CONTEXT: label: dance rating */
  tagdefs [TAG_DANCERATING].displayname = _("Dance Rating");
  /* CONTEXT: label: date */
  tagdefs [TAG_DATE].displayname = _("Date");
  /* CONTEXT: label: date added to the database */
  tagdefs [TAG_DBADDDATE].displayname = _("Date Added");
  /* CONTEXT: label: disc number */
  tagdefs [TAG_DISCNUMBER].displayname = _("Disc");
  /* CONTEXT: label: duration of the song */
  tagdefs [TAG_DURATION].displayname = _("Duration");
  /* CONTEXT: label: favorite marker */
  tagdefs [TAG_FAVORITE].displayname = _("Favorite");
  /* CONTEXT: label: genre */
  tagdefs [TAG_GENRE].displayname = _("Genre");
  /* CONTEXT: label: keyword (used to filter out songs) */
  tagdefs [TAG_KEYWORD].displayname = _("Keyword");
  /* CONTEXT: label: marquee display (alternate display for the marquee, replaces dance) */
  tagdefs [TAG_MQDISPLAY].displayname = _("Marquee Display");
  /* CONTEXT: label: notes */
  tagdefs [TAG_NOTES].displayname = _("Notes");
  /* CONTEXT: label: time to end the song */
  tagdefs [TAG_SONGEND].displayname = _("Song End");
  /* CONTEXT: label: time to start the song */
  tagdefs [TAG_SONGSTART].displayname = _("Song Start");
  /* CONTEXT: label: speed adjustment for playback */
  tagdefs [TAG_SPEEDADJUSTMENT].displayname = _("Speed Adjustment");
  /* CONTEXT: label: status */
  tagdefs [TAG_STATUS].displayname = _("Status");
  /* CONTEXT: label: tags (for use by the user) */
  tagdefs [TAG_TAGS].displayname = _("Tags");
  /* CONTEXT: label: title */
  tagdefs [TAG_TITLE].displayname = _("Title");
  /* CONTEXT: label: track number */
  tagdefs [TAG_TRACKNUMBER].displayname = _("Track");
  /* CONTEXT: when the database entry was last updated */
  tagdefs [TAG_LAST_UPDATED].displayname = _("Last Updated");
  /* CONTEXT: label: volume adjustment for playback */
  tagdefs [TAG_VOLUMEADJUSTPERC].displayname = _("Volume Adjustment");

  if (bdjoptGetNum (OPT_G_BPM) == BPM_BPM) {
    /* CONTEXT: label: beats per minute */
    tagdefs [TAG_BPM].displayname = _("BPM");
  }
  if (bdjoptGetNum (OPT_G_BPM) == BPM_MPM) {
    /* CONTEXT: label: measures per minute */
    tagdefs [TAG_BPM].displayname = _("MPM");
  }

  tagdefinfo.taglookup = slistAlloc ("tagdef", LIST_UNORDERED, NULL);
  slistSetSize (tagdefinfo.taglookup, TAG_KEY_MAX);
  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    slistSetNum (tagdefinfo.taglookup, tagdefs [i].tag, i);
  }
  slistSort (tagdefinfo.taglookup);
}

void
tagdefCleanup (void)
{
  if (! tagdefinfo.initialized) {
    return;
  }

  if (tagdefinfo.taglookup != NULL) {
    slistFree (tagdefinfo.taglookup);
    tagdefinfo.taglookup = NULL;
  }
  tagdefinfo.initialized = false;
}

tagdefkey_t
tagdefLookup (char *str)
{
  tagdefInit ();

  return slistGetNum (tagdefinfo.taglookup, str);
}

