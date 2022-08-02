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

/* if any itunes names are added, TAG_ITUNES_MAX must be updated in tagdef.h */

tagdef_t tagdefs [TAG_KEY_MAX] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    NULL,                         /* display name         */
    { { "ADJUSTFLAGS", NULL, NULL },
      { "----:BDJ4:ADJUSTFLAGS", "----", "ADJUSTFLAGS" },
      { "TXXX=ADJUSTFLAGS", "TXXX", "ADJUSTFLAGS" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "ALBUM", NULL, NULL },
      { "©alb", NULL, NULL },
      { "TALB", NULL, NULL },
      { "WM/AlbumTitle", NULL, NULL }
    },       /* audio tags */
    "Album",                      /* itunes name          */
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
    { { "ALBUMARTIST", NULL, NULL },
      { "aART", NULL, NULL },
      { "TPE2", NULL, NULL },
      { "WM/AlbumArtist", NULL, NULL }
    },       /* audio tags */
    "Album Artist",               /* itunes name          */
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
    { { "ARTIST", NULL, NULL },
      { "©ART", NULL, NULL },
      { "TPE1", NULL, NULL },
      { "WM/Author", NULL, NULL }
    },       /* audio tags */
    "Artist",                     /* itunes name          */
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
    { { "BPM", NULL, NULL },
      { "tmpo", NULL, NULL },
      { "TBPM", NULL, NULL },
      { "WM/BeatsPerMinute", NULL, NULL }
    },       /* audio tags */
    "BPM",                        /* itunes name          */
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
    { { "COMPOSER", NULL, NULL },
      { "©wrt", NULL, NULL },
      { "TCOM", NULL, NULL },
      { "WM/Composer", NULL, NULL }
    },       /* audio tags */
    "Composer",                   /* itunes name          */
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
    { { "CONDUCTOR", NULL, NULL },
      { "----:com.apple.iTunes:CONDUCTOR", "----", "CONDUCTOR" },
      { "TPE3", NULL, NULL },
      { "WM/Conductor", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "DANCE", NULL, NULL },
      { "----:BDJ4:DANCE", "----", "DANCE" },
      { "TXXX=DANCE", "TXXX", "DANCE" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "DANCELEVEL", NULL, NULL },
      { "----:BDJ4:DANCELEVEL", "----", "DANCELEVEL" },
      { "TXXX=DANCELEVEL", "TXXX", "DANCELEVEL" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "DANCERATING", NULL, NULL },
      { "----:BDJ4:DANCERATING", "----", "DANCERATING" },
      { "TXXX=DANCERATING", "TXXX", "DANCERATING" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    "Rating",                     /* itunes name          */
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
    { { "DATE", NULL, NULL },
      { "©day", NULL, NULL },
      { "TYER", NULL, NULL },
      { "WM/Year", NULL, NULL }
    },       /* audio tags */
    "Year",                       /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Date Added",                 /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
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
  [TAG_DISCNUMBER] =
  { "DISC",                       /* tag */
    NULL,                         /* display name         */
    { { "DISCNUMBER", NULL, NULL },
      { "disk", NULL, NULL },
      { "TPOS", NULL, NULL },
      { "WM/PartOfSet", NULL, NULL }
    },       /* audio tags */
    "Disc Number",                /* itunes name          */
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
    { { "DISCTOTAL", NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Disc Count",                 /* itunes name          */
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
    { { "DURATION", NULL, NULL },
      { "----:BDJ4:DURATION", "----", "DURATION" },
      { "TXXX=DURATION", "TXXX", "DURATION" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    "Total Time",                 /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Location",                   /* itunes name          */
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
    { { "FAVORITE", NULL, NULL },
      { "----:BDJ4:FAVORITE", "----", "FAVORITE" },
      { "TXXX=FAVORITE", "TXXX", "FAVORITE" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    "Favorite",                   /* itunes name          */
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
    { { "GENRE", NULL, NULL },
      { "©gen", NULL, NULL },
      { "TCON", NULL, NULL },
      { "WM/Genre", NULL, NULL }
    },       /* audio tags */
    "Genre",                      /* itunes name          */
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
    { { "KEYWORD", NULL, NULL },
      { "----:BDJ4:KEYWORD", "----", "KEYWORD" },
      { "TXXX=KEYWORD", "TXXX", "KEYWORD" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "MQDISPLAY", NULL, NULL },
      { "----:BDJ4:MQDISPLAY", "----", "MQDISPLAY" },
      { "TXXX=MQDISPLAY", "TXXX", "MQDISPLAY" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "MUSICBRAINZ_TRACKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Track Id", "----", "MusicBrainz Track Id" },
      { "UFID=http://musicbrainz.org", "UFID", "http://musicbrainz.org" },
      { "MusicBrainz/Track Id", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "MUSICBRAINZ_WORKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Work Id", "----", "MusicBrainz Work Id" },
      { "TXXX=MusicBrainz Work Id", "TXXX", "MusicBrainz Work Id" },
      { "MusicBrainz/Work Id", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "MUSICBRAINZ_RELEASETRACKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Release Track Id", "----", "MusicBrainz Release Track Id" },
      { "TXXX=MusicBrainz Release Track Id", "TXXX", "MusicBrainz Release Track Id" },
      { "MusicBrainz/Release Track Id", NULL, NULL }
    },  /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "NOTES", NULL, NULL },
      { "----:BDJ4:NOTES", "----", "NOTES" },
      { "TXXX=NOTES", "TXXX", "NOTES" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "SAMESONG", NULL, NULL },
      { "----:BDJ4:SAMESONG", "----", "SAMESONG" },
      { "TXXX=SAMESONG", "TXXX", "SAMESONG" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "SONGEND", NULL, NULL },
      { "----:BDJ4:SONGEND", "----", "SONGEND" },
      { "TXXX=SONGEND", "TXXX", "SONGEND" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    "Stop Time",                  /* itunes name          */
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
    { { "SONGSTART", NULL, NULL },
      { "----:BDJ4:SONGSTART", "----", "SONGSTART" },
      { "TXXX=SONGSTART", "TXXX", "SONGSTART" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    "Start Time",                 /* itunes name          */
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
    { { "SPEEDADJUSTMENT", NULL, NULL },
      { "----:BDJ4:SPEEDADJUSTMENT", "----", "SPEEDADJUSTMENT" },
      { "TXXX=SPEEDADJUSTMENT", "TXXX", "SPEEDADJUSTMENT" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "STATUS", NULL, NULL },
      { "----:BDJ4:STATUS", "----", "STATUS" },
      { "TXXX=STATUS", "TXXX", "STATUS" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "TAGS", NULL, NULL },
      { "keyw", NULL, NULL },
      { "TXXX=TAGS", "TXXX", "TAGS" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { "TITLE", NULL, NULL },
      { "©nam", NULL, NULL },
      { "TIT2", NULL, NULL },
      { "WM/Title", NULL, NULL }
    },       /* audio tags */
    "Name",                       /* itunes name          */
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
    { { "TRACKNUMBER", NULL, NULL },
      { "trkn", NULL, NULL },
      { "TRCK", NULL, NULL },
      { "WM/TrackNumber", NULL, NULL }
    },       /* audio tags */
    "Track Number",               /* itunes name          */
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
    { { "TRACKTOTAL", NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Track Count",                /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Date Modified",              /* itunes name          */
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
    { { "VOLUMEADJUSTPERC", NULL, NULL },
      { "----:BDJ4:VOLUMEADJUSTPERC", "----", "VOLUMEADJUSTPERC" },
      { "TXXX=VOLUMEADJUSTPERC", "TXXX", "VOLUMEADJUSTPERC" },
      { NULL, NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
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
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
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

  /* listing display is true */

  /* CONTEXT: label: album */
  tagdefs [TAG_ALBUM].displayname = _("Album");
  /* CONTEXT: label: album artist */
  tagdefs [TAG_ALBUMARTIST].displayname = _("Album Artist");
  /* CONTEXT: label: artist */
  tagdefs [TAG_ARTIST].displayname = _("Artist");
  if (bdjoptGetNum (OPT_G_BPM) == BPM_BPM) {
    /* CONTEXT: label: beats per minute */
    tagdefs [TAG_BPM].displayname = _("BPM");
  }
  if (bdjoptGetNum (OPT_G_BPM) == BPM_MPM) {
    /* CONTEXT: label: measures per minute */
    tagdefs [TAG_BPM].displayname = _("MPM");
  }
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
  /* CONTEXT: label: disc count */
  tagdefs [TAG_DISCTOTAL].displayname = _("Disc Count");
  /* CONTEXT: label: duration of the song */
  tagdefs [TAG_DURATION].displayname = _("Duration");
  /* CONTEXT: label: favorite marker */
  tagdefs [TAG_FAVORITE].displayname = _("Favorite");
  /* CONTEXT: label: genre */
  tagdefs [TAG_GENRE].displayname = _("Genre");
  /* CONTEXT: label: keyword (used to filter out songs) */
  tagdefs [TAG_KEYWORD].displayname = _("Keyword");
  /* CONTEXT: label: notes */
  tagdefs [TAG_NOTES].displayname = _("Notes");
  /* CONTEXT: label: status */
  tagdefs [TAG_STATUS].displayname = _("Status");
  /* CONTEXT: label: tags (for use by the user) */
  tagdefs [TAG_TAGS].displayname = _("Tags");
  /* CONTEXT: label: title */
  tagdefs [TAG_TITLE].displayname = _("Title");
  /* CONTEXT: label: track number */
  tagdefs [TAG_TRACKNUMBER].displayname = _("Track");
  /* CONTEXT: label: track count */
  tagdefs [TAG_TRACKTOTAL].displayname = _("Track Count");

  /* editable */

  /* CONTEXT: label: marquee display (alternate display for the marquee, replaces dance) */
  tagdefs [TAG_MQDISPLAY].displayname = _("Marquee Display");
  /* CONTEXT: label: time to end the song */
  tagdefs [TAG_SONGEND].displayname = _("Song End");
  /* CONTEXT: label: time to start the song */
  tagdefs [TAG_SONGSTART].displayname = _("Song Start");
  /* CONTEXT: label: speed adjustment for playback */
  tagdefs [TAG_SPEEDADJUSTMENT].displayname = _("Speed Adjustment");
  /* CONTEXT: label: volume adjustment for playback */
  tagdefs [TAG_VOLUMEADJUSTPERC].displayname = _("Volume Adjustment");

  /* search item */

  /* CONTEXT: label: audio file modification time */
  tagdefs [TAG_AFMODTIME].displayname = _("Audio File Date");
  /* CONTEXT: label: when the database entry was last updated */
  tagdefs [TAG_LAST_UPDATED].displayname = _("Last Updated");

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
tagdefLookup (const char *str)
{
  tagdefInit ();

  return slistGetNum (tagdefinfo.taglookup, str);
}

