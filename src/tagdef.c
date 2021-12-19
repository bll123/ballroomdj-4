#include <stdlib.h>
#include <libintl.h>

#include "tagdef.h"
#include "bdjstring.h"

tagdef_t tagdefs[] = {
    { TAG_ADJUSTFLAGS,              /* name                 */
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
    { TAG_AFMODTIME,                /* name                 */
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
      1,                            /* listing display      */
      1,                            /* song list display    */
      0                             /* text search          */
    },
    { TAG_ALBUM,                    /* name                 */
      "",                           /* label                */
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
    { TAG_ALBUMARTIST,              /* name                 */
      "",                           /* label                */
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
    { TAG_ARTIST,                   /* name                 */
      "",                           /* label                */
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
    { TAG_BDJSYNCID,                /* name                 */
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
    { TAG_BPM,                      /* name                 */
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
    { TAG_COMPOSER,                 /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_CONDUCTOR,                /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_DANCE,                    /* name                 */
      "",                           /* label                */
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
    { TAG_DANCELEVEL,               /* name                 */
      "",                           /* label                */
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
    { TAG_DANCERATING,              /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_DATE,                     /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_DBADDDATE,                /* name                 */
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
      1,                            /* listing display      */
      1,                            /* song list display    */
      0                             /* text search          */
    },
    { TAG_DISCNUMBER,               /* name                 */
      "",                           /* label                */
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
    { TAG_DISCTOTAL,                /* name                 */
      "",                           /* label                */
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
    { TAG_DISPLAYIMG,               /* name                 */
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
    { TAG_DURATION,                 /* name                 */
      "",                           /* label                */
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
    { TAG_DURATION_STR,             /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
      9,                            /* default edit order   */
      16,                           /* edit index           */
      15,                           /* edit width           */
      ANCHOR_WEST,                  /* listing anchor       */
      0,                            /* listing weight       */
      ANCHOR_WEST,                  /* song list anchor     */
      0,                            /* song list weight     */
      AIDD_NO,                      /* audio id disp        */
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
    { TAG_FILE,                     /* name                 */
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
      1                             /* text search          */
    },
    { TAG_GENRE,                    /* name                 */
      "",                           /* label                */
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
    { TAG_KEYWORD,                  /* name                 */
      "",                           /* label                */
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
    { TAG_MQDISPLAY,                /* name                 */
      "",                           /* label                */
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
    { TAG_MUSICBRAINZ_TRACKID,      /* name                 */
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
      1,                            /* is norm tag          */
      0,                            /* album edit           */
      0,                            /* all edit             */
      0,                            /* editable             */
      0,                            /* listing display      */
      0,                            /* song list display    */
      0                             /* text search          */
    },
    { TAG_NOMAXPLAYTIME,            /* name                 */
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
    { TAG_NOTES,                    /* name                 */
      "",                           /* label                */
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
    { TAG_SAMESONG,                 /* name                 */
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
    { TAG_SONGEND,                  /* name                 */
      "",                           /* label                */
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
    { TAG_SONGSTART,                /* name                 */
      "",                           /* label                */
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
    { TAG_SPEEDADJUSTMENT,          /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_STATUS,                   /* name                 */
      "",                           /* label                */
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
    { TAG_TAGS,                     /* name                 */
      "",                           /* label                */
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
    { TAG_TITLE,                    /* name                 */
      "",                           /* label                */
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
    { TAG_TRACKNUMBER,              /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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
    { TAG_TRACKTOTAL,               /* name                 */
      "",                           /* label                */
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
    { TAG_VARIOUSARTISTS,           /* name                 */
      "",                           /* label                */
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
    { TAG_VOLUMEADJUSTPERC,         /* name                 */
      "",                           /* label                */
      "",                           /* short-label          */
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

void
tagdefInit (void)
{
  tagdefs [TAG_KEY_AFMODTIME].label = _("Audio File Date");
  tagdefs [TAG_KEY_ALBUM].label = _("Album");
  tagdefs [TAG_KEY_ALBUMARTIST].label = _("Album Artist");
  tagdefs [TAG_KEY_ARTIST].label = _("Artist");
  tagdefs [TAG_KEY_COMPOSER].label = _("Composer");
  tagdefs [TAG_KEY_CONDUCTOR].label = _("Conductor");
  tagdefs [TAG_KEY_DANCE].label = _("Dance");
  tagdefs [TAG_KEY_DANCELEVEL].label = _("Level");
  tagdefs [TAG_KEY_DANCERATING].label = _("Dance Rating");
  tagdefs [TAG_KEY_DANCERATING].shortLabel = _("Rating");
  tagdefs [TAG_KEY_DATE].label = _("Date");
  tagdefs [TAG_KEY_DBADDDATE].label = _("Date Added");
  tagdefs [TAG_KEY_DISCNUMBER].label = _("Disc");
  tagdefs [TAG_KEY_DISCTOTAL].label = _("Total Discs");
  tagdefs [TAG_KEY_DISPLAYIMG].label = _("Image");
  tagdefs [TAG_KEY_DURATION].label = _("Duration");
  tagdefs [TAG_KEY_DURATION_STR].label = _("Duration");
  tagdefs [TAG_KEY_GENRE].label = _("Genre");
  tagdefs [TAG_KEY_KEYWORD].label = _("Keyword");
  tagdefs [TAG_KEY_MQDISPLAY].label = _("Marquee Display");
  tagdefs [TAG_KEY_NOMAXPLAYTIME].label = _("No Maximum Play Time");
  tagdefs [TAG_KEY_NOMAXPLAYTIME].shortLabel = _("No Max");
  tagdefs [TAG_KEY_NOTES].label = _("Notes");
  tagdefs [TAG_KEY_SONGEND].label = _("Song End");
  tagdefs [TAG_KEY_SONGSTART].label = _("Song Start");
  tagdefs [TAG_KEY_SPEEDADJUSTMENT].label = _("Speed Adjustment");
  tagdefs [TAG_KEY_SPEEDADJUSTMENT].shortLabel = _("Spd. Adj.");
  tagdefs [TAG_KEY_STATUS].label = _("Status");
  tagdefs [TAG_KEY_TAGS].label = _("Tags");
  tagdefs [TAG_KEY_TITLE].label = _("Title");
  tagdefs [TAG_KEY_TRACKNUMBER].label = _("Track Number");
  tagdefs [TAG_KEY_TRACKNUMBER].shortLabel = _("Track");
  tagdefs [TAG_KEY_TRACKTOTAL].label = _("Total Tracks");
  tagdefs [TAG_KEY_VARIOUSARTISTS].label = _("Various Artists");
  tagdefs [TAG_KEY_VOLUMEADJUSTPERC].label = _("Adjustment");
  tagdefs [TAG_KEY_VOLUMEADJUSTPERC].shortLabel = _("Vol. Adj.");
}
