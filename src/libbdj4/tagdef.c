/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
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
#include "songutil.h"
#include "status.h"
#include "tagdef.h"

/* as of 2022-6-13 the tag names may no longer match the vorbis tag names */
/* the tag name is used in the internal database and for debugging */

/* if any itunes names are added, TAG_ITUNES_MAX must be updated in tagdef.h */

/* as of 2025-9-26, the tag-type-mk is not used and any data present */
/* is probably incorrect */

tagdef_t tagdefs [TAG_KEY_MAX] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ADJUSTFLAGS", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:ADJUSTFLAGS", "BDJ4", "ADJUSTFLAGS", NULL },
      [TAG_TYPE_ID3] = { "TXXX=ADJUSTFLAGS", "TXXX", "ADJUSTFLAGS", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    songutilConvAdjustFlags,      /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUM] =
  { "ALBUM",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUM", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©alb", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TALB", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/AlbumTitle", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "IPRD", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/TITLE", "ALBUM", "TITLE", NULL },
    },       /* audio tags */
    "Album",                      /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_ALBUMARTIST] =
  { "ALBUMARTIST",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUMARTIST", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "aART", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TPE2", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/AlbumArtist", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/ARTIST", "ALBUM", "ARTIST", NULL },
    },       /* audio tags */
    "Album Artist",               /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_ARTIST] =
  { "ARTIST",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ARTIST", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©ART", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TPE1", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Author", NULL, NULL, NULL },   // or author?
      [TAG_TYPE_RIFF] = { "IART", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/ARTIST", "TRACK", "ARTIST", NULL },
    },       /* audio tags */
    "Artist",                     /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  /* used internally */
  [TAG_AUDIOID_IDENT] =
  { "AUDIOID_IDENT",              /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* used internally */
  [TAG_AUDIOID_SCORE] =
  { "AUDIOID_SCORE",              /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "BPM", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "tmpo", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TBPM", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/BeatsPerMinute", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/BPM", "TRACK", "BPM", NULL },
    },       /* audio tags */
    "BPM",                        /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_BPM_DISPLAY] =
  { "BPMDISPLAY",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    true,                         /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_COMMENT] =
  { "COMMENT" ,                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "COMMENT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©cmt", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "COMM", NULL, NULL, NULL },    // handled in ati
      [TAG_TYPE_ASF] = { "Description", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "ICMT", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/COMMENT", NULL, NULL, NULL },
    },       /* audio tags */
    "Comment",                    /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_COMPOSER] =
  { "COMPOSER",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "COMPOSER", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©wrt", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TCOM", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Composer", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/COMPOSER", "ALBUM", "COMPOSER", NULL },
    },       /* audio tags */
    "Composer",                   /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_CONDUCTOR] =
  { "CONDUCTOR",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "CONDUCTOR", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:CONDUCTOR", "BDJ4", "CONDUCTOR", NULL },
      [TAG_TYPE_ID3] = { "TPE3", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Conductor", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/CONDUCTOR", "ALBUM", "CONDUCTOR", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_DANCE] =
  { "DANCE",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCE", "BDJ4", "DANCE", NULL },
      [TAG_TYPE_ID3] = { "TXXX=DANCE", "TXXX", "DANCE", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_DANCE", "TRACK", "_DANCE", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    danceConvDance,               /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    true,                         /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCELEVEL] =
  { "DANCELEVEL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCELEVEL", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCELEVEL", "BDJ4", "DANCELEVEL", NULL },
      [TAG_TYPE_ID3] = { "TXXX=DANCELEVEL", "TXXX", "DANCELEVEL", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_DANCELEVEL", "TRACK", "_DANCELEVEL", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    levelConv,                    /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DANCERATING] =
  { "DANCERATING",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DANCERATING", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:DANCERATING", "BDJ4", "DANCERATING", NULL },
      [TAG_TYPE_ID3] = { "TXXX=DANCERATING", "TXXX", "DANCERATING", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_DANCERATING", "TRACK", "_DANCERATING", NULL },
    },       /* audio tags */
    "Rating",                     /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    ratingConv,                   /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DATE] =
  { "DATE",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DATE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©day", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TDRC", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Year", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "ICRD", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/DATE_RECORDED", "ALBUM", "DATE_RECORDED", NULL },
    },       /* audio tags */
    "Year",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DBADDDATE] =
  { "DB_ADD_DATE",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    "Date Added",                 /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCNUMBER] =
  { "DISC",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DISCNUMBER", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "disk", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TPOS", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/PartOfSet", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "VOLUME/PART_NUMBER", "VOLUME", "PART_NUMBER", NULL },
    },       /* audio tags */
    "Disc Number",                /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DISCTOTAL] =
  { "DISCTOTAL",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DISCTOTAL", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "VOLUME/TOTAL_PARTS", "VOLUME", "TOTAL_PARTS", NULL },
    },         /* audio tags */
    "Disc Count",                 /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_DURATION] =
  { "DURATION",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "DURATION", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_LABEL,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_URI] =
  { "URI",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    "Location",                   /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_FAVORITE] =
  { "FAVORITE",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "FAVORITE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:FAVORITE", "BDJ4", "FAVORITE", NULL },
      [TAG_TYPE_ID3] = { "TXXX=FAVORITE", "TXXX", "FAVORITE", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_FAVORITE", "TRACK", "_FAVORITE", NULL },
    },       /* audio tags */
    "Favorite",                   /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    songFavoriteConv,             /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_GENRE] =
  { "GENRE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "GENRE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©gen", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TCON", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Genre", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "IGNR", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/GENRE", "ALBUM", "GENRE", NULL },
    },       /* audio tags */
    "Genre",                      /* itunes name          */
    ET_COMBOBOX,                  /* edit type            */
    VALUE_NUM,                    /* value type           */
    genreConv,                    /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_GROUPING] =
  { "GROUPING",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "GROUPING", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©grp", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "GRP1", NULL, NULL, NULL },      // itunes compat
      [TAG_TYPE_ASF] = { "WM/ContentGroupDescription", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_GROUPING", "TRACK", "_GROUPING", NULL },
    },       /* audio tags */
    "Grouping",                   /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  /* used internally : at this time, not stored in the database */
  /* i do not know that any container formats use a uri for the cover image */
  /* podcasts use a uri */
  [TAG_IMAGE_URI] =
  { "IMAGE_URI",                    /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },   // or author?
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_KEYWORD] =
  { "KEYWORD",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "KEYWORD", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:KEYWORD", "BDJ4", "KEYWORD", NULL },
      [TAG_TYPE_ID3] = { "TXXX=KEYWORD", "TXXX", "KEYWORD", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_KEYWORD", "TRACK", "_KEYWORD", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_MOVEMENTNAME] =
  { "MOVEMENTNAME",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MOVEMENTNAME", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©mvn", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "MVNM", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "MOVEMENT/TITLE", "MOVEMENT", "TITLE", NULL },
    },       /* audio tags */
    "Movement Name",              /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_MOVEMENTNUM] =
  { "MOVEMENTNUM",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MOVEMENT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©mvi", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "MVIN", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "MOVEMENT/PART_NUMBER", "MOVEMENT", "PART_NUMBER", NULL },
    },       /* audio tags */
    "Movement Number",            /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_MOVEMENTCOUNT] =
  { "MOVEMENTCOUNT",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MOVEMENTTOTAL", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©mvc", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "MOVEMENT/TOTAL_PARTS", "MOVEMENT", "TOTAL_PARTS", NULL },
    },       /* audio tags */
    "Movement Count",             /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_MQDISPLAY] =
  { "MQDISPLAY",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MQDISPLAY", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:MQDISPLAY", "BDJ4", "MQDISPLAY", NULL },
      [TAG_TYPE_ID3] = { "TXXX=MQDISPLAY", "TXXX", "MQDISPLAY", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_MQDISPLAY", "TRACK", "_MQDISPLAY", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_RECORDING_ID] =
  { "RECORDING_ID",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_TRACKID", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Track Id", "BDJ4", "MusicBrainz Track Id", NULL },
      [TAG_TYPE_ID3] = { "UFID=http://musicbrainz.org", "UFID", "http://musicbrainz.org", NULL },
      [TAG_TYPE_ASF] = { "MusicBrainz/Track Id", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_MUSICBRAINZ_TRACKID", "TRACK", "_MUSICBRAINZ_TRACKID", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_WORK] =
  { "WORK",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "WORK", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©wrk", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TIT1", NULL, NULL, NULL },      // itunes compat
      [TAG_TYPE_ASF] = { "WM/Work", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },       /* audio tags */
    "Work",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_WORK_ID] =
  { "WORK_ID",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_WORKID", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Work Id", "BDJ4", "MusicBrainz Work Id", NULL },
      [TAG_TYPE_ID3] = { "TXXX=MusicBrainz Work Id", "TXXX", "MusicBrainz Work Id", NULL },
      [TAG_TYPE_ASF] = { "MusicBrainz/Work Id", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_MUSICBRAINZ_WORKID", "TRACK", "_MUSICBRAINZ_WORKID", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACK_ID] =
  { "TRACK_ID",                    /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "MUSICBRAINZ_RELEASETRACKID", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:com.apple.iTunes:MusicBrainz Release Track Id", "BDJ4", "MusicBrainz Release Track Id", NULL },
      [TAG_TYPE_ID3] = { "TXXX=MusicBrainz Release Track Id", "TXXX", "MusicBrainz Release Track Id", NULL },
      [TAG_TYPE_ASF] = { "MusicBrainz/Release Track Id", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_MUSICBRAINZ_RELEASETRACKID", "TRACK", "_MUSICBRAINZ_RELEASETRACKID", NULL },
    },  /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_NO_PLAY_TM_LIMIT] =
  { "NOPLAYTMLIMIT",              /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "NOPLAYTMLIMIT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:NOPLAYTMLIMIT", "BDJ4", "NOPLAYTMLIMIT", NULL },
      [TAG_TYPE_ID3] = { "TXXX=NOPLAYTMLIMIT", "TXXX", "NOPLAYTMLIMIT", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_NOPLAYTMLIMIT", "TRACK", "_NOPLAYTMLIMIT", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SWITCH,                    /* edit type            */
    VALUE_NUM,                    /* value type           */
    convBoolean,                  /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_NOTES] =
  { "NOTES",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "NOTES", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:NOTES", "BDJ4", "NOTES", NULL },
      [TAG_TYPE_ID3] = { "TXXX=NOTES", "TXXX", "NOTES", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_NOTES", "TRACK", "_NOTES", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SAMESONG] =
  { "SAMESONG",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SAMESONG", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SAMESONG", "BDJ4", "SAMESONG", NULL },
      [TAG_TYPE_ID3] = { "TXXX=SAMESONG", "TXXX", "SAMESONG", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_SAMESONG", "TRACK", "_SAMESONG", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SHOWMOVEMENT] =
  { "SHOWMOVEMENT",
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SHOWMOVEMENT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "shwm", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TXXX=SHOWMOVEMENT", "TXXX", "SHOWMOVEMENT", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_SHOWMOVEMENT", "TRACK", "_SHOWMOVEMENT", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGEND] =
  { "SONGEND",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SONGEND", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SONGEND", "BDJ4", "SONGEND", NULL },
      [TAG_TYPE_ID3] = { "TXXX=SONGEND", "TXXX", "SONGEND", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_SONGEND", "TRACK", "_SONGEND", NULL },
    },       /* audio tags */
    "Stop Time",                  /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SONGSTART] =
  { "SONGSTART",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SONGSTART", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SONGSTART", "BDJ4", "SONGSTART", NULL },
      [TAG_TYPE_ID3] = { "TXXX=SONGSTART", "TXXX", "SONGSTART", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_SONGSTART", "TRACK", "_SONGSTART", NULL },
    },       /* audio tags */
    "Start Time",                 /* itunes name          */
    ET_SPINBOX_TIME,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SONG_TYPE] =
  { "SONGTYPE",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SONGTYPE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SONGTYPE", "BDJ4", "SONGTYPE", NULL },
      [TAG_TYPE_ID3] = { "TXXX=SONGTYPE", "TXXX", "SONGTYPE", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_SONGTYPE", "TRACK", "_SONGTYPE", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    songutilConvSongType,         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SORT_ALBUM] =
  { "ALBUMSORT",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUMSORT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "soal", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TSOA", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/AlbumSortOrder", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/SORT_WITH", "ALBUM", "SORT_WITH", NULL },
    },       /* audio tags */
    "Sort Album",                 /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SORT_ALBUMARTIST] =
  { "ALBUMARTISTSORT",            /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ALBUMARTISTSORT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "soaa", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TSO2", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/AlbumArtistSortOrder", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/ARTIST/SORT_WITH", "ALBUM", "ARTIST", "SORT_WITH" },
    },       /* audio tags */
    "Sort Album Artist",          /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_SORT_ARTIST] =
  { "ARTISTSORT",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "ARTISTSORT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "soar", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TSOP", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/ArtistSortOrder", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/ARTIST/SORT_WITH", "TRACK", "ARTIST", "SORT_WITH" },
    },       /* audio tags */
    "Sort Artist",                /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_SORT_COMPOSER] =
  { "COMPOSERSORT",               /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "COMPOSERSORT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "soco", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TSOC", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/ComposerSortOrder", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "COMPOSER/SORT_WITH", "COMPOSER", "SORT_WITH", NULL },
    },       /* audio tags */
    "Sort Composer",              /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    true,                         /* vorbis multi         */
  },
  [TAG_SORT_TITLE] =
  { "TITLESORT",                  /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TITLESORT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "sonm", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TSOT", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/TitleSortOrder", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/TITLE/SORT_WITH", "TRACK", "TITLE", "SORT_WITH" },
    },       /* audio tags */
    "Sort Name",                  /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_SPEEDADJUSTMENT] =
  { "SPEEDADJUSTMENT",            /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "SPEEDADJUSTMENT", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:SPEEDADJUSTMENT", "BDJ4", "SPEEDADJUSTMENT", NULL },
      [TAG_TYPE_ID3] = { "TXXX=SPEEDADJUSTMENT", "TXXX", "SPEEDADJUSTMENT", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK_SPEEDADJUSTMENT", "TRACK", "_SPEEDADJUSTMENT", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_STATUS] =
  { "STATUS",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "STATUS", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:STATUS", "BDJ4", "STATUS", NULL },
      [TAG_TYPE_ID3] = { "TXXX=STATUS", "TXXX", "STATUS", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_STATUS", "TRACK", "_STATUS", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SPINBOX_TEXT,              /* edit type            */
    VALUE_NUM,                    /* value type           */
    statusConv,                   /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_TAGS] =
  { "TAGS",                       /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TAGS", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "keyw", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TXXX=TAGS", "TXXX", "TAGS", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_TAGS", "TRACK", "_TAGS", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_LIST,                   /* value type           */
    convTextList,                 /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_TITLE] =
  { "TITLE",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TITLE", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "©nam", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TIT2", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/Title", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "INAM", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/TITLE", "TRACK", "TITLE", NULL },
    },       /* audio tags */
    "Name",                       /* itunes name          */
    ET_ENTRY,                     /* edit type            */
    VALUE_STR,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    true,                         /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    true,                         /* marquee-disp         */
    true,                         /* player-ui-disp       */
    true,                         /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKNUMBER] =
  { "TRACKNUMBER",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TRACKNUMBER", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "trkn", NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { "TRCK", NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { "WM/TrackNumber", NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { "ITRK", NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/PART_NUMBER", "TRACK", "PART_NUMBER", NULL },
    },       /* audio tags */
    "Track Number",               /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_TRACKTOTAL] =
  { "TRACKTOTAL",                 /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "TRACKTOTAL", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "ALBUM/TOTAL_PARTS", "ALBUM", "TOTAL_PARTS", NULL },
    },         /* audio tags */
    "Track Count",                /* itunes name          */
    ET_SPINBOX,                   /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    true,                         /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    true,                         /* align end            */
    false,                        /* is bdj tag           */
    true,                         /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    true,                         /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_LAST_UPDATED] =
  { "LASTUPDATED",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  [TAG_VOLUMEADJUSTPERC] =
  { "VOLUMEADJUSTPERC",           /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "VOLUMEADJUSTPERC", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:VOLUMEADJUSTPERC", "BDJ4", "VOLUMEADJUSTPERC", NULL },
      [TAG_TYPE_ID3] = { "TXXX=VOLUMEADJUSTPERC", "TXXX", "VOLUMEADJUSTPERC", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { "TRACK/_VOLUMEADJUSTPERC", "TRACK", "_VOLUMEADJUSTPERC", NULL },
    },       /* audio tags */
    NULL,                         /* itunes name          */
    ET_SCALE,                     /* edit type            */
    VALUE_DOUBLE,                 /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* internal : relative record number */
  [TAG_RRN] =
  { "RRN",                        /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* internal : temporary to save the dbidx */
  [TAG_DBIDX] =
  { "DBIDX",                      /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* prevent-rename */
  [TAG_DB_LOC_LOCK] =
  { "DB_LOC_LOCK",                /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { "PREVENTRENAME", NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { "----:BDJ4:PREVENTRENAME", "BDJ4", "PREVENTRENAME", NULL },
      [TAG_TYPE_ID3] = { "TXXX=PREVENTRENAME", "TXXX", "PREVENTRENAME", NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_SWITCH,                    /* edit type            */
    VALUE_NUM,                    /* value type           */
    convBoolean,                  /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    true,                         /* is bdj tag           */
    false,                        /* is norm tag          */
    true,                         /* edit-all             */
    true,                         /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* internal : the length of any directory prefixing the song uri */
  [TAG_PREFIX_LEN] =
  { "PFXLEN",                     /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },        /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
  /* internal : various database flags */
  [TAG_DB_FLAGS] =
  { "DB_FLAGS",                   /* tag */
    NULL,                         /* display name         */
    NULL,                         /* short display name   */
    { [TAG_TYPE_VORBIS] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MP4] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ID3] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_ASF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_RIFF] = { NULL, NULL, NULL, NULL },
      [TAG_TYPE_MK] = { NULL, NULL, NULL, NULL },
    },         /* audio tags */
    NULL,                         /* itunes name          */
    ET_NA,                        /* edit type            */
    VALUE_NUM,                    /* value type           */
    NULL,                         /* conv func            */
    false,                        /* listing display      */
    false,                        /* secondary display    */
    false,                        /* ellipsize            */
    false,                        /* align end            */
    false,                        /* is bdj tag           */
    false,                        /* is norm tag          */
    false,                        /* edit-all             */
    false,                        /* editable             */
    false,                        /* audio-id             */
    false,                        /* marquee-disp         */
    false,                        /* player-ui-disp       */
    false,                        /* text search          */
    false,                        /* vorbis multi         */
  },
};

static_assert (sizeof (tagdefs) / sizeof (tagdef_t) == TAG_KEY_MAX,
    "missing tag entry");

typedef struct {
  slist_t               *taglookup;
  volatile atomic_flag  initialized;
} tagdefinfo_t;

static tagdefinfo_t   tagdefinfo = {
  NULL,
  ATOMIC_FLAG_INIT,
};

void
tagdefInit (void)
{
  if (atomic_flag_test_and_set (&tagdefinfo.initialized)) {
    return;
  }

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
  /* CONTEXT: label: comment */
  tagdefs [TAG_COMMENT].displayname = _("Comment");
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
  /* CONTEXT: label: total disc count (short name for listing) */
  tagdefs [TAG_DISCTOTAL].shortdisplayname = _("Tot. Discs");
  /* CONTEXT: label: duration of the song */
  tagdefs [TAG_DURATION].displayname = _("Duration");
  /* CONTEXT: label: favorite marker */
  tagdefs [TAG_FAVORITE].displayname = _("Favourite");
  /* CONTEXT: label: genre */
  tagdefs [TAG_GENRE].displayname = _("Genre");
  /* CONTEXT: label: grouping */
  tagdefs [TAG_GROUPING].displayname = _("Grouping");
  /* CONTEXT: label: keyword (used to filter out songs) */
  tagdefs [TAG_KEYWORD].displayname = _("Keyword");
  /* CONTEXT: label: no play time limit */
  tagdefs [TAG_NO_PLAY_TM_LIMIT].displayname = _("No Play Time Limit");
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
  /* CONTEXT: label: total track count (short name for listing) */
  tagdefs [TAG_TRACKTOTAL].shortdisplayname = _("Tot. Tracks");
  /* CONTEXT: label: work (a musical work) */
  tagdefs [TAG_WORK].displayname = _("Work");

  /* CONTEXT: label: title sort order */
  tagdefs [TAG_SORT_TITLE].displayname = _("Title Sort Order");
  /* CONTEXT: label: album sort order */
  tagdefs [TAG_SORT_ALBUM].displayname = _("Album Sort Order");
  /* CONTEXT: label: album artist sort order */
  tagdefs [TAG_SORT_ALBUMARTIST].displayname = _("Album Artist Sort Order");
  /* CONTEXT: label: artist sort order */
  tagdefs [TAG_SORT_ARTIST].displayname = _("Artist Sort Order");
  /* CONTEXT: label: composer sort order */
  tagdefs [TAG_SORT_COMPOSER].displayname = _("Composer Sort Order");

  /* CONTEXT: label: movement (classical music movement number) */
  tagdefs [TAG_MOVEMENTNUM].displayname = _("Movement Number");
  /* CONTEXT: label: movement (classical music movement number) (short name for listing) */
  tagdefs [TAG_MOVEMENTNUM].shortdisplayname = _("Mvt No.");
  /* CONTEXT: label: movement count (classical music movement count) */
  tagdefs [TAG_MOVEMENTCOUNT].displayname = _("Movement Count");
  /* CONTEXT: label: movement (classical music movement count) (short name for listing) */
  tagdefs [TAG_MOVEMENTCOUNT].shortdisplayname = _("Mvt Count");
  /* CONTEXT: label: movement name (classical music movement name) */
  tagdefs [TAG_MOVEMENTNAME].displayname = _("Movement");

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
  if (! atomic_flag_test_and_set (&tagdefinfo.initialized)) {
    atomic_flag_clear (&tagdefinfo.initialized);
    return;
  }

  slistFree (tagdefinfo.taglookup);
  tagdefinfo.taglookup = NULL;
  atomic_flag_clear (&tagdefinfo.initialized);
}

/* can return LIST_VALUE_INVALID if not found */
int
tagdefLookup (const char *str)
{
  tagdefInit ();

  return slistGetNum (tagdefinfo.taglookup, str);
}

