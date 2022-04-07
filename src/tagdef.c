#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "datafile.h"
#include "slist.h"
#include "tagdef.h"

tagdef_t tagdefs [TAG_KEY_MAX] = {
  [TAG_ADJUSTFLAGS] =
  { "ADJUSTFLAGS",                /* tag */
    NULL,                         /* display name         */
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "©alb", "TALB", "WM/AlbumTitle" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "aART", "TPE2", "WM/AlbumArtist" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "©ART", "TPE1", "WM/Author" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    1,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    1,                            /* is org tag           */
  },
  [TAG_AUTOORGFLAG] =
  { "AUTOORGFLAG",                /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_BPM] =
  { "BPM",                        /* tag */
    NULL,                         /* display name         */
    { "tmpo", "TBPM", "WM/BeatsPerMinute" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_END,                    /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "©wrt", "TCOM", "WM/Composer" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_OPT,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TPE3", "WM/Conductor" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_OPT,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_COMBOBOX,                  /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_COMBOBOX,                  /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_COMBOBOX,                  /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "©day", "TYER", "WM/Year" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_OPT,                     /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_DBADDDATE] =
  { "DBADDDATE",                  /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "disk", "TPOS", "WM/PartOfSet" },       /* audio tags */
    5,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_END,                    /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_YES,                     /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    15,                           /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_END,                    /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "gnre", "TCON", "WM/Genre" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_COMBOBOX,                  /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    20,                           /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    20,                           /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    1,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_MUSICBRAINZ_TRACKID] =
  { "MUSICBRAINZ_TRACKID",                /* tag */
    NULL,                         /* display name         */
    { NULL, "UFID", "MusicBrainz/Track Id" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    10,                           /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    10,                           /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_SCALE,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_COMBOBOX,                  /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "keyw", "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "©nam", "TIT2", "WM/Title" },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_CENTER,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { "trkn", "TRCK", "WM/TrackNumber" },       /* audio tags */
    5,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_END,                    /* song list anchor     */
    ET_ENTRY,                     /* edit type            */
    DISP_YES,                     /* audio id disp        */
    1,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_START,                  /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_YES,                     /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_UPDATEFLAG] =
  { "UPDATEFLAG",                 /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_UPDATETIME] =
  { "UPDATETIME",                 /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, "TXXX", NULL },       /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_SCALE,                     /* edit type            */
    DISP_NO,                      /* audio id disp        */
    1,                            /* listing display      */
    1,                            /* song edit only       */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_WRITETIME] =
  { "WRITETIME",                  /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* text search          */
    0,                            /* is org tag           */
  },
  [TAG_RRN] =
  { "RRN",                        /* tag */
    NULL,                         /* display name         */
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
    { NULL, NULL, NULL },         /* audio tags */
    0,                            /* edit width           */
    ALIGN_END,                    /* listing anchor       */
    ALIGN_START,                  /* song list anchor     */
    ET_NA,                        /* edit type            */
    DISP_NO,                      /* audio id disp        */
    0,                            /* listing display      */
    0,                            /* song edit only       */
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
  datafileconv_t    conv;

  if (tagdefinfo.initialized) {
    return;
  }
  tagdefinfo.initialized = true;

  tagdefs [TAG_AFMODTIME].displayname = _("Audio File Date");
  tagdefs [TAG_ALBUM].displayname = _("Album");
  tagdefs [TAG_ALBUMARTIST].displayname = _("Album Artist");
  tagdefs [TAG_ARTIST].displayname = _("Artist");
  tagdefs [TAG_COMPOSER].displayname = _("Composer");
  tagdefs [TAG_CONDUCTOR].displayname = _("Conductor");
  tagdefs [TAG_DANCE].displayname = _("Dance");
  tagdefs [TAG_DANCELEVEL].displayname = _("Dance Level");
  tagdefs [TAG_DANCERATING].displayname = _("Dance Rating");
  tagdefs [TAG_DBADDDATE].displayname = _("Date Added");
  tagdefs [TAG_DISCNUMBER].displayname = _("Disc");
  tagdefs [TAG_DURATION].displayname = _("Duration");
  tagdefs [TAG_FAVORITE].displayname = _("Favorite");
  tagdefs [TAG_GENRE].displayname = _("Genre");
  tagdefs [TAG_KEYWORD].displayname = _("Keyword");
  tagdefs [TAG_MQDISPLAY].displayname = _("Marquee Display");
  tagdefs [TAG_NOTES].displayname = _("Notes");
  tagdefs [TAG_SONGEND].displayname = _("Song End");
  tagdefs [TAG_SONGSTART].displayname = _("Song Start");
  tagdefs [TAG_SPEEDADJUSTMENT].displayname = _("Speed Adjustment");
  tagdefs [TAG_STATUS].displayname = _("Status");
  tagdefs [TAG_TAGS].displayname = _("Tags");
  tagdefs [TAG_TITLE].displayname = _("Title");
  tagdefs [TAG_TRACKNUMBER].displayname = _("Track Number");
  tagdefs [TAG_UPDATETIME].displayname = _("Last Updated");
  tagdefs [TAG_VOLUMEADJUSTPERC].displayname = _("Volume Adjustment");

  conv.valuetype = VALUE_NUM;
  conv.u.num = bdjoptGetNum (OPT_G_BPM);
  bdjoptConvBPM (&conv);
  tagdefs [TAG_BPM].displayname = _(conv.u.str);

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

