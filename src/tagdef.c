#include <stdlib.h>
#include <libintl.h>

#include "tagdef.h"
#include "bdjstring.h"

tagdef_t tagdefs = {
  { "ADJUSTFLAGS",                /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
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
  { "AFMODTIME",                  /* name                 */
    _("Audio File Date"),         /* label                */
    "",                           /* short-label          */
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
  { "ALBUM",                      /* name                 */
    _("Album"),                   /* label                */
    "",                           /* short-label          */
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
  { "ALBUMARTIST",                /* name                 */
    _("Album Artist"),            /* label                */
    "",                           /* short-label          */
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
  { "ARTIST",                     /* name                 */
    _("Artist"),                  /* label                */
    "",                           /* short-label          */
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
  { "BDJSYNCID",                  /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
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
    0                             /* text search          */
  },
  { "BPM",                        /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
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
  { "COMPOSER",                   /* name                 */
    _("Composer"),                /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    2,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    $v(audioid.disp.opt),         /* audio id disp        */
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
  { "CONDUCTOR",                  /* name                 */
    _("Conductor"),               /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    3,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    $v(audioid.disp.opt),         /* audio id disp        */
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
  { "DANCE",                      /* name                 */
    _("Dance"),                   /* label                */
    "",                           /* short-label          */
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
  { "DANCELEVEL",                 /* name                 */
    _("Level"),                   /* label                */
    "",                           /* short-label          */
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
  { "DANCERATING",                /* name                 */
    _("Dance Rating"),            /* label                */
    _("Rating"),                  /* short-label          */
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
  { "DATE",                       /* name                 */
    _("Date"),                    /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    $v(audioid.disp.opt),         /* audio id disp        */
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
  { "DBADDDATE",                  /* name                 */
    _("Date Added"),              /* label                */
    "",                           /* short-label          */
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
  { "DISCNUMBER",                 /* name                 */
    _("Disc"),                    /* label                */
    "",                           /* short-label          */
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
  { "DISCTOTAL",                  /* name                 */
    _("Total Discs"),             /* label                */
    "",                           /* short-label          */
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
  { "DISPLAYIMG",                 /* name                 */
    _("Image"),                   /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    1                             /* text search          */
  },
  { "DURATION",                   /* name                 */
    _("Duration"),                /* label                */
    "",                           /* short-label          */
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
  { "DURATION_STR",               /* name                 */
    _("Duration"),                /* label                */
    "",                           /* short-label          */
    9,                            /* default edit order   */
    16,                           /* edit index           */
    15,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_N,                       /* audio id disp        */
    ET_DISABLED_ENTRY,            /* edit type            */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  { "FILE",                       /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    AIDD_NO,                      /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    ANCHOR_WEST,                  /* audio id disp        */
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
  { "GENRE",                      /* name                 */
    _("Genre"),                   /* label                */
    "",                           /* short-label          */
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
  { "KEYWORD",                    /* name                 */
    _("Keyword"),                 /* label                */
    "",                           /* short-label          */
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
  { "MQDISPLAY",                  /* name                 */
    _("Marquee Display"),         /* label                */
    "",                           /* short-label          */
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
  { "MUSICBRAINZ_TRACKID",        /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_N,                       /* audio id disp        */
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
  { "NOMAXPLAYTIME",              /* name                 */
    _("No Maximum Play Time"),    /* label                */
    _("No Max"),                  /* short-label          */
    0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_N,                       /* audio id disp        */
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
  { "NOTES",                      /* name                 */
    _("Notes"),                   /* label                */
    "",                           /* short-label          */
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
  { "SAMESONG",                   /* name                 */
    "",                           /* label                */
    "",                           /* short-label          */
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
  { "SONGEND",                    /* name                 */
    _("Song End"),                /* label                */
    "",                           /* short-label          */
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
  { "SONGSTART",                  /* name                 */
    _("Song Start"),              /* label                */
    "",                           /* short-label          */
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
  { "SPEEDADJUSTMENT",            /* name                 */
    _("Speed Adjustment"),        /* label                */
    _("Spd. Adj."),               /* short-label          */
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
  { "STATUS",                     /* name                 */
    _("Status"),                  /* label                */
    "",                           /* short-label          */
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
  { "TAGS",                       /* name                 */
    _("Tags"),                    /* label                */
    "",                           /* short-label          */
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
  { "TITLE",                      /* name                 */
    _("Title"),                   /* label                */
    "",                           /* short-label          */
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
  { "TRACKNUMBER",                /* name                 */
    _("Track Number"),            /* label                */
    _("Track"),                   /* short-label          */
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
  { "TRACKTOTAL",                 /* name                 */
    _("Total Tracks"),            /* label                */
    "",                           /* short-label          */
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
  { "VARIOUSARTISTS",             /* name                 */
    _("Various Artists"),         /* label                */
    "",                           /* short-label          */
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
  { "VOLUMEADJUSTPERC",           /* name                 */
    _("Volume Adjustment"),       /* label                */
    _("Vol. Adj."),               /* short-label          */
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
  }
};
