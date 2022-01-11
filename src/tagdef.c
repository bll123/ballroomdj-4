#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "tagdef.h"
#include "bdjstring.h"
#include "list.h"

tagdef_t tagdefs[MAX_TAG_KEY] = {
  [TAG_ADJUSTFLAGS] =
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    7,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 1,                            /* default edit order   */
    4,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 3,                            /* default edit order   */
    5,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    15,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    2,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    3,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    5,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 5,                            /* default edit order   */
    9,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    1,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 7,                            /* default edit order   */
    11,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 6,                            /* default edit order   */
    10,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_OPT,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 13,                           /* default edit order   */
    13,                           /* edit index           */
    5,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
    0,                            /* is bdj tag           */
    1,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    0                             /* text search          */
  },
  [TAG_DISPLAYIMG] =
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
    1,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    1,                            /* album edit           */
    1,                            /* all edit             */
    1,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_DURATION] =
  { 9,                            /* default edit order   */
    16,                           /* edit index           */
    15,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
    0,                            /* is bdj tag           */
    0,                            /* is norm tag          */
    0,                            /* album edit           */
    0,                            /* all edit             */
    0,                            /* editable             */
    0,                            /* listing display      */
    0,                            /* song list display    */
    1                             /* text search          */
  },
  [TAG_GENRE] =
  { 1,                            /* default edit order   */
    1,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    20,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    20,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_CHECKBUTTON,               /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 8,                            /* default edit order   */
    12,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    10,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    10,                           /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_SCALE,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    17,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_COMBOBOX,                  /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 8,                            /* default edit order   */
    12,                           /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 4,                            /* default edit order   */
    8,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_BOTH,                  /* listing anchor       */
    1,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    10,                           /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    14,                           /* edit index           */
    5,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_EAST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_ENTRY,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_YES,                     /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    6,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_CHECKBUTTON,               /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_SCALE,                     /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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
  { 0,                            /* default edit order   */
    0,                            /* edit index           */
    0,                            /* edit width           */
    ANCHOR_EAST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_NA,                        /* edit type            */
    TAGDEF_INITIALIZED,           /* initialized          */
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

