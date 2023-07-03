/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "dance.h"
#include "datafile.h"
#include "genre.h"
#include "level.h"
#include "rating.h"
#include "slist.h"
#include "songfav.h"
#include "songutil.h"
#include "status.h"
#include "tagdef.h"

/* as of 2022-6-13 the tag names may no longer match the vorbis tag names */
/* the tag name is used in the internal database and for debugging */

/* if any itunes names are added, TAG_ITUNES_MAX must be updated in tagdef.h */

tagdef_t tagdefs [TAG_KEY_MAX] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "ADJUSTFLAGS", NULL, NULL },
      { "----:BDJ4:ADJUSTFLAGS", "BDJ4", "ADJUSTFLAGS" },
      { "TXXX=ADJUSTFLAGS", "TXXX", "ADJUSTFLAGS" },
      { NULL, NULL, NULL },
      { "ADJUSTFLAGS", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    songutilConvAdjustFlags,      /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUM] =
  { "ALBUM",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "ALBUM", NULL, NULL },
      { "©alb", NULL, NULL },
      { "TALB", NULL, NULL },
      { "WM/AlbumTitle", NULL, NULL },
      { "album", NULL, NULL }
    },       /* audio tags */
    "Album",                      /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUMARTIST] =
  { "ALBUMARTIST",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "ALBUMARTIST", NULL, NULL },
      { "aART", NULL, NULL },
      { "TPE2", NULL, NULL },
      { "WM/AlbumArtist", NULL, NULL },
      { "album_artist", NULL, NULL }
    },       /* audio tags */
    "Album Artist",               /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_ARTIST] =
  { "ARTIST",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "ARTIST", NULL, NULL },
      { "©ART", NULL, NULL },
      { "TPE1", NULL, NULL },
      { "Author", NULL, NULL },
      { "artist", NULL, NULL }
    },       /* audio tags */
    "Artist",                     /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "BPM", NULL, NULL },
      { "tmpo", NULL, NULL },
      { "TBPM", NULL, NULL },
      { "WM/BeatsPerMinute", NULL, NULL },
      { "bpm", NULL, NULL }
    },       /* audio tags */
    "BPM",                        /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_BPM_DISPLAY] =
  { "BPMDISPLAY",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { "BPMDISPLAY", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    true,                         /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_COMPOSER] =
  { "COMPOSER",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "COMPOSER", NULL, NULL },
      { "©wrt", NULL, NULL },
      { "TCOM", NULL, NULL },
      { "WM/Composer", NULL, NULL },
      { "composer", NULL, NULL }
    },       /* audio tags */
    "Composer",                   /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_CONDUCTOR] =
  { "CONDUCTOR",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "CONDUCTOR", NULL, NULL },
      { "----:com.apple.iTunes:CONDUCTOR", "BDJ4", "CONDUCTOR" },
      { "TPE3", NULL, NULL },
      { "WM/Conductor", NULL, NULL },
      { "performer", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_DANCE] =
  { "DANCE",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DANCE", NULL, NULL },
      { "----:BDJ4:DANCE", "BDJ4", "DANCE" },
      { "TXXX=DANCE", "TXXX", "DANCE" },
      { NULL, NULL, NULL },
      { "DANCE", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    danceConvDance,               /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCELEVEL] =
  { "DANCELEVEL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DANCELEVEL", NULL, NULL },
      { "----:BDJ4:DANCELEVEL", "BDJ4", "DANCELEVEL" },
      { "TXXX=DANCELEVEL", "TXXX", "DANCELEVEL" },
      { NULL, NULL, NULL },
      { "DANCELEVEL", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    levelConv,                    /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCERATING] =
  { "DANCERATING",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DANCERATING", NULL, NULL },
      { "----:BDJ4:DANCERATING", "BDJ4", "DANCERATING" },
      { "TXXX=DANCERATING", "TXXX", "DANCERATING" },
      { NULL, NULL, NULL },
      { "DANCERATING", NULL, NULL }
    },       /* audio tags */
    "Rating",                     /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    ratingConv,                   /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DATE] =
  { "DATE",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DATE", NULL, NULL },
      { "©day", NULL, NULL },
      { "TDRC", NULL, NULL },
      { "WM/Year", NULL, NULL },
      { "date", NULL, NULL }
    },       /* audio tags */
    "Year",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_OPT,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DBADDDATE] =
  { "DBADDDATE",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { "DBADDDATE", NULL, NULL }
    },         /* audio tags */
    "Date Added",                 /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCNUMBER] =
  { "DISC",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DISCNUMBER", NULL, NULL },
      { "disk", NULL, NULL },
      { "TPOS", NULL, NULL },
      { "WM/PartOfSet", NULL, NULL },
      { "disc", NULL, NULL }
    },       /* audio tags */
    "Disc Number",                /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCTOTAL] =
  { "DISCTOTAL",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DISCTOTAL", NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Disc Count",                 /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DURATION] =
  { "DURATION",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "DURATION", NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { "DURATION", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    convMS,                       /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_FILE] =
  { "FILE",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { "FILE", NULL, NULL }
    },         /* audio tags */
    "Location",                   /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_FAVORITE] =
  { "FAVORITE",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "FAVORITE", NULL, NULL },
      { "----:BDJ4:FAVORITE", "BDJ4", "FAVORITE" },
      { "TXXX=FAVORITE", "TXXX", "FAVORITE" },
      { NULL, NULL, NULL },
      { "FAVORITE", NULL, NULL }
    },       /* audio tags */
    "Favorite",                   /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    songFavoriteConv,             /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_GENRE] =
  { "GENRE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "GENRE", NULL, NULL },
      { "©gen", NULL, NULL },
      { "TCON", NULL, NULL },
      { "WM/Genre", NULL, NULL },
      { "genre", NULL, NULL }
    },       /* audio tags */
    "Genre",                      /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    genreConv,                    /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_KEYWORD] =
  { "KEYWORD",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "KEYWORD", NULL, NULL },
      { "----:BDJ4:KEYWORD", "BDJ4", "KEYWORD" },
      { "TXXX=KEYWORD", "TXXX", "KEYWORD" },
      { NULL, NULL, NULL },
      { "KEYWORD", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_MQDISPLAY] =
  { "MQDISPLAY",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "MQDISPLAY", NULL, NULL },
      { "----:BDJ4:MQDISPLAY", "BDJ4", "MQDISPLAY" },
      { "TXXX=MQDISPLAY", "TXXX", "MQDISPLAY" },
      { NULL, NULL, NULL },
      { "MQDISPLAY", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_RECORDING_ID] =
  { "RECORDING_ID",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "MUSICBRAINZ_TRACKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Track Id", "BDJ4", "MusicBrainz Track Id" },
      { "UFID=http://musicbrainz.org", "UFID", "http://musicbrainz.org" },
      { "MusicBrainz/Track Id", NULL, NULL },
      { "MusicBrainz Track Id", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_WORK_ID] =
  { "WORK_ID",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "MUSICBRAINZ_WORKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Work Id", "BDJ4", "MusicBrainz Work Id" },
      { "TXXX=MusicBrainz Work Id", "TXXX", "MusicBrainz Work Id" },
      { "MusicBrainz/Work Id", NULL, NULL },
      { "MusicBrainz Work Id", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACK_ID] =
  { "TRACK_ID",                    /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "MUSICBRAINZ_RELEASETRACKID", NULL, NULL },
      { "----:com.apple.iTunes:MusicBrainz Release Track Id", "BDJ4", "MusicBrainz Release Track Id" },
      { "TXXX=MusicBrainz Release Track Id", "TXXX", "MusicBrainz Release Track Id" },
      { "MusicBrainz/Release Track Id", NULL, NULL },
      { "MusicBrainz Release Track Id", NULL, NULL }
    },  /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_NOTES] =
  { "NOTES",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "NOTES", NULL, NULL },
      { "----:BDJ4:NOTES", "BDJ4", "NOTES" },
      { "TXXX=NOTES", "TXXX", "NOTES" },
      { NULL, NULL, NULL },
      { "NOTES", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SAMESONG] =
  { "SAMESONG",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "SAMESONG", NULL, NULL },
      { "----:BDJ4:SAMESONG", "BDJ4", "SAMESONG" },
      { "TXXX=SAMESONG", "TXXX", "SAMESONG" },
      { NULL, NULL, NULL },
      { "SAMESONG", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGEND] =
  { "SONGEND",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "SONGEND", NULL, NULL },
      { "----:BDJ4:SONGEND", "BDJ4", "SONGEND" },
      { "TXXX=SONGEND", "TXXX", "SONGEND" },
      { NULL, NULL, NULL },
      { "SONGEND", NULL, NULL }
    },       /* audio tags */
    "Stop Time",                  /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGSTART] =
  { "SONGSTART",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "SONGSTART", NULL, NULL },
      { "----:BDJ4:SONGSTART", "BDJ4", "SONGSTART" },
      { "TXXX=SONGSTART", "TXXX", "SONGSTART" },
      { NULL, NULL, NULL },
      { "SONGSTART", NULL, NULL }
    },       /* audio tags */
    "Start Time",                 /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SPEEDADJUSTMENT] =
  { "SPEEDADJUSTMENT",            /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "SPEEDADJUSTMENT", NULL, NULL },
      { "----:BDJ4:SPEEDADJUSTMENT", "BDJ4", "SPEEDADJUSTMENT" },
      { "TXXX=SPEEDADJUSTMENT", "TXXX", "SPEEDADJUSTMENT" },
      { NULL, NULL, NULL },
      { "SPEEDADJUSTMENT", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_STATUS] =
  { "STATUS",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "STATUS", NULL, NULL },
      { "----:BDJ4:STATUS", "BDJ4", "STATUS" },
      { "TXXX=STATUS", "TXXX", "STATUS" },
      { NULL, NULL, NULL },
      { "STATUS", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    statusConv,                   /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TAGS] =
  { "TAGS",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "TAGS", NULL, NULL },
      { "keyw", NULL, NULL },
      { "TXXX=TAGS", "TXXX", "TAGS" },
      { NULL, NULL, NULL },
      { "keywords", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_LIST,                   /* value type           */
    convTextList,                 /* conv func            */
    DISP_NO,                      /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TITLE] =
  { "TITLE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "TITLE", NULL, NULL },
      { "©nam", NULL, NULL },
      { "TIT2", NULL, NULL },
      { "Title", NULL, NULL },
      { "title", NULL, NULL }
    },       /* audio tags */
    "Name",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKNUMBER] =
  { "TRACKNUMBER",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "TRACKNUMBER", NULL, NULL },
      { "trkn", NULL, NULL },
      { "TRCK", NULL, NULL },
      { "WM/TrackNumber", NULL, NULL },
      { "track", NULL, NULL }
    },       /* audio tags */
    "Track Number",               /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKTOTAL] =
  { "TRACKTOTAL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "TRACKTOTAL", NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    "Track Count",                /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_YES,                     /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_LAST_UPDATED] =
  { "LASTUPDATED",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { "LASTUPDATED", NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_VOLUMEADJUSTPERC] =
  { "VOLUMEADJUSTPERC",           /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { "VOLUMEADJUSTPERC", NULL, NULL },
      { "----:BDJ4:VOLUMEADJUSTPERC", "BDJ4", "VOLUMEADJUSTPERC" },
      { "TXXX=VOLUMEADJUSTPERC", "TXXX", "VOLUMEADJUSTPERC" },
      { NULL, NULL, NULL },
      { "VOLUMEADJUSTPERC", NULL, NULL }
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_RRN] =
  { "RRN",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DBIDX] =
  { "DBIDX",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_PREFIX_LEN] =
  { "PFXLEN",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL, },
      { NULL, NULL, NULL, },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },        /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TEMPORARY] =
  { "TEMPORARY",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL },
      { NULL, NULL, NULL }
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    DISP_NO,                      /* audio id disp        */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
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
  /* CONTEXT: label: dance level (short name for listing) */
  tagdefs [TAG_DANCELEVEL].shortdisplayname = _("Level");
  /* CONTEXT: label: dance rating */
  tagdefs [TAG_DANCERATING].displayname = _("Dance Rating");
  /* CONTEXT: label: dance rating (short name for listing) */
  tagdefs [TAG_DANCERATING].shortdisplayname = _("Rating");
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

  /* CONTEXT: label: when the database entry was last updated */
  tagdefs [TAG_LAST_UPDATED].displayname = _("Last Updated");

  tagdefinfo.taglookup = slistAlloc ("tagdef", LIST_UNORDERED, NULL);
  slistSetSize (tagdefinfo.taglookup, TAG_KEY_MAX);
  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].isBDJTag && tagdefs [i].isNormTag) {
      fprintf (stderr, "FATAL: invalid tagdef: %s both bdj-tag and norm-tag are set\n", tagdefs [i].tag);
      exit (1);
    }
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

  slistFree (tagdefinfo.taglookup);
  tagdefinfo.taglookup = NULL;
  tagdefinfo.initialized = false;
}

tagdefkey_t
tagdefLookup (const char *str)
{
  tagdefInit ();

  return slistGetNum (tagdefinfo.taglookup, str);
}

