/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
    { [TAG_TYPE_VORBIS] = { "ADJUSTFLAGS", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:ADJUSTFLAGS", "BDJ4", "ADJUSTFLAGS" },
      [TAG_TYPE_ID3] = { "TXXX=ADJUSTFLAGS", "TXXX", "ADJUSTFLAGS" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    songutilConvAdjustFlags,      /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUM] =
  { "ALBUM",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUM", NULL, NULL },
      [TAG_TYPE_MP4] = { "©alb", NULL, NULL },
      [TAG_TYPE_ID3] = { "TALB", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/AlbumTitle", NULL, NULL },
      [TAG_TYPE_WAV] = { "IPRD", NULL, NULL },
    },       /* audio tags */
    "Album",                      /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUMARTIST] =
  { "ALBUMARTIST",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUMARTIST", NULL, NULL },
      [TAG_TYPE_MP4] = { "aART", NULL, NULL },
      [TAG_TYPE_ID3] = { "TPE2", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/AlbumArtist", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Album Artist",               /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_ARTIST] =
  { "ARTIST",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ARTIST", NULL, NULL },
      [TAG_TYPE_MP4] = { "©ART", NULL, NULL },
      [TAG_TYPE_ID3] = { "TPE1", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Author", NULL, NULL },
      [TAG_TYPE_WAV] = { "IART", NULL, NULL },
    },       /* audio tags */
    "Artist",                     /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_AUDIOID_IDENT] =
  { "AUDIOID_IDENT",              /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_AUDIOID_SCORE] =
  { "SCORE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "BPM", NULL, NULL },
      [TAG_TYPE_MP4] = { "tmpo", NULL, NULL },
      [TAG_TYPE_ID3] = { "TBPM", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/BeatsPerMinute", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "BPM",                        /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_BPM_DISPLAY] =
  { "BPMDISPLAY",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    true,                         /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_COMPOSER] =
  { "COMPOSER",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "COMPOSER", NULL, NULL },
      [TAG_TYPE_MP4] = { "©wrt", NULL, NULL },
      [TAG_TYPE_ID3] = { "TCOM", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Composer", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Composer",                   /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_CONDUCTOR] =
  { "CONDUCTOR",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "CONDUCTOR", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:CONDUCTOR", "BDJ4", "CONDUCTOR" },
      [TAG_TYPE_ID3] = { "TPE3", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Conductor", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_DANCE] =
  { "DANCE",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCE", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCE", "BDJ4", "DANCE" },
      [TAG_TYPE_ID3] = { "TXXX=DANCE", "TXXX", "DANCE" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    danceConvDance,               /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    true,                         /* player-ui-disp       */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCELEVEL] =
  { "DANCELEVEL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCELEVEL", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCELEVEL", "BDJ4", "DANCELEVEL" },
      [TAG_TYPE_ID3] = { "TXXX=DANCELEVEL", "TXXX", "DANCELEVEL" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    levelConv,                    /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCERATING] =
  { "DANCERATING",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCERATING", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCERATING", "BDJ4", "DANCERATING" },
      [TAG_TYPE_ID3] = { "TXXX=DANCERATING", "TXXX", "DANCERATING" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Rating",                     /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    ratingConv,                   /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DATE] =
  { "DATE",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DATE", NULL, NULL },
      [TAG_TYPE_MP4] = { "©day", NULL, NULL },
      [TAG_TYPE_ID3] = { "TDRC", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Year", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Year",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DBADDDATE] =
  { "DBADDDATE",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    "Date Added",                 /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCNUMBER] =
  { "DISC",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DISCNUMBER", NULL, NULL },
      [TAG_TYPE_MP4] = { "disk", NULL, NULL },
      [TAG_TYPE_ID3] = { "TPOS", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/PartOfSet", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Disc Number",                /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCTOTAL] =
  { "DISCTOTAL",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DISCTOTAL", NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    "Disc Count",                 /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DURATION] =
  { "DURATION",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DURATION", NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  /* leave this as 'FILE' for now, will require a big db upgrade to fix */
  [TAG_URI] =
  { "URI",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    "Location",                   /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_FAVORITE] =
  { "FAVORITE",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "FAVORITE", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:FAVORITE", "BDJ4", "FAVORITE" },
      [TAG_TYPE_ID3] = { "TXXX=FAVORITE", "TXXX", "FAVORITE" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Favorite",                   /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    songFavoriteConv,             /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_GENRE] =
  { "GENRE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "GENRE", NULL, NULL },
      [TAG_TYPE_MP4] = { "©gen", NULL, NULL },
      [TAG_TYPE_ID3] = { "TCON", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Genre", NULL, NULL },
      [TAG_TYPE_WAV] = { "IGNR", NULL, NULL },
    },       /* audio tags */
    "Genre",                      /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    genreConv,                    /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    true,                         /* is org tag           */
    true,                         /* vorbis multi         */
  },
  [TAG_KEYWORD] =
  { "KEYWORD",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "KEYWORD", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:KEYWORD", "BDJ4", "KEYWORD" },
      [TAG_TYPE_ID3] = { "TXXX=KEYWORD", "TXXX", "KEYWORD" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_MQDISPLAY] =
  { "MQDISPLAY",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MQDISPLAY", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:MQDISPLAY", "BDJ4", "MQDISPLAY" },
      [TAG_TYPE_ID3] = { "TXXX=MQDISPLAY", "TXXX", "MQDISPLAY" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_RECORDING_ID] =
  { "RECORDING_ID",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_TRACKID", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Track Id", "BDJ4", "MusicBrainz Track Id" },
      [TAG_TYPE_ID3] = { "UFID=http://musicbrainz.org", "UFID", "http://musicbrainz.org" },
      [TAG_TYPE_WMA] = { "MusicBrainz/Track Id", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_WORK_ID] =
  { "WORK_ID",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_WORKID", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Work Id", "BDJ4", "MusicBrainz Work Id" },
      [TAG_TYPE_ID3] = { "TXXX=MusicBrainz Work Id", "TXXX", "MusicBrainz Work Id" },
      [TAG_TYPE_WMA] = { "MusicBrainz/Work Id", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACK_ID] =
  { "TRACK_ID",                    /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_RELEASETRACKID", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Release Track Id", "BDJ4", "MusicBrainz Release Track Id" },
      [TAG_TYPE_ID3] = { "TXXX=MusicBrainz Release Track Id", "TXXX", "MusicBrainz Release Track Id" },
      [TAG_TYPE_WMA] = { "MusicBrainz/Release Track Id", NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },  /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_NOTES] =
  { "NOTES",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "NOTES", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:NOTES", "BDJ4", "NOTES" },
      [TAG_TYPE_ID3] = { "TXXX=NOTES", "TXXX", "NOTES" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SAMESONG] =
  { "SAMESONG",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SAMESONG", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SAMESONG", "BDJ4", "SAMESONG" },
      [TAG_TYPE_ID3] = { "TXXX=SAMESONG", "TXXX", "SAMESONG" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGEND] =
  { "SONGEND",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SONGEND", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SONGEND", "BDJ4", "SONGEND" },
      [TAG_TYPE_ID3] = { "TXXX=SONGEND", "TXXX", "SONGEND" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Stop Time",                  /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGSTART] =
  { "SONGSTART",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SONGSTART", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SONGSTART", "BDJ4", "SONGSTART" },
      [TAG_TYPE_ID3] = { "TXXX=SONGSTART", "TXXX", "SONGSTART" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    "Start Time",                 /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_SPEEDADJUSTMENT] =
  { "SPEEDADJUSTMENT",            /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SPEEDADJUSTMENT", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SPEEDADJUSTMENT", "BDJ4", "SPEEDADJUSTMENT" },
      [TAG_TYPE_ID3] = { "TXXX=SPEEDADJUSTMENT", "TXXX", "SPEEDADJUSTMENT" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_STATUS] =
  { "STATUS",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "STATUS", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:STATUS", "BDJ4", "STATUS" },
      [TAG_TYPE_ID3] = { "TXXX=STATUS", "TXXX", "STATUS" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    statusConv,                   /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TAGS] =
  { "TAGS",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TAGS", NULL, NULL },
      [TAG_TYPE_MP4] = { "keyw", NULL, NULL },
      [TAG_TYPE_ID3] = { "TXXX=TAGS", "TXXX", "TAGS" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_LIST,                   /* value type           */
    convTextList,                 /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TITLE] =
  { "TITLE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TITLE", NULL, NULL },
      [TAG_TYPE_MP4] = { "©nam", NULL, NULL },
      [TAG_TYPE_ID3] = { "TIT2", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/Title", NULL, NULL },
      [TAG_TYPE_WAV] = { "INAM", NULL, NULL },
    },       /* audio tags */
    "Name",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKNUMBER] =
  { "TRACKNUMBER",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TRACKNUMBER", NULL, NULL },
      [TAG_TYPE_MP4] = { "trkn", NULL, NULL },
      [TAG_TYPE_ID3] = { "TRCK", NULL, NULL },
      [TAG_TYPE_WMA] = { "WM/TrackNumber", NULL, NULL },
      [TAG_TYPE_WAV] = { "ITRK", NULL, NULL },
    },       /* audio tags */
    "Track Number",               /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    true,                         /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKTOTAL] =
  { "TRACKTOTAL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TRACKTOTAL", NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    "Track Count",                /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align right          */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_LAST_UPDATED] =
  { "LASTUPDATED",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_VOLUMEADJUSTPERC] =
  { "VOLUMEADJUSTPERC",           /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "VOLUMEADJUSTPERC", NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:VOLUMEADJUSTPERC", "BDJ4", "VOLUMEADJUSTPERC" },
      [TAG_TYPE_ID3] = { "TXXX=VOLUMEADJUSTPERC", "TXXX", "VOLUMEADJUSTPERC" },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_RRN] =
  { "RRN",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DBIDX] =
  { "DBIDX",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DB_LOC_LOCK] =
  { "DB_LOC_LOCK",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_SWITCH,                    /* edit type            */
    VALUE_NUM,                    /* value type           */
    convBoolean,                  /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_PREFIX_LEN] =
  { "PFXLEN",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },        /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* is org tag           */
    false,                        /* vorbis multi         */
  },
  [TAG_DB_FLAGS] =
  { "DB_FLAGS",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL },
      [TAG_TYPE_WMA] = { NULL, NULL, NULL },
      [TAG_TYPE_WAV] = { NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align right          */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
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
  /* CONTEXT: audio identification: the score or rating of each match */
  tagdefs [TAG_AUDIOID_SCORE].displayname = _("Score");
  /* CONTEXT: audio identification: the audio identification source */
  tagdefs [TAG_AUDIOID_IDENT].displayname = _("Source");

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
  /* CONTEXT: label: lock audio file location */
  tagdefs [TAG_DB_LOC_LOCK].displayname = _("Prevent Rename");
  /* CONTEXT: label: disc number */
  tagdefs [TAG_DISCNUMBER].displayname = _("Disc");
  /* CONTEXT: label: total disc count */
  tagdefs [TAG_DISCTOTAL].displayname = _("Total Discs");
  /* CONTEXT: label: duration of the song */
  tagdefs [TAG_DURATION].displayname = _("Duration");
  /* CONTEXT: label: favorite marker */
  tagdefs [TAG_FAVORITE].displayname = _("Favourite");
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
  /* CONTEXT: label: total track count */
  tagdefs [TAG_TRACKTOTAL].displayname = _("Total Tracks");

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

