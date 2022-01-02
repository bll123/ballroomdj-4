#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "tagdef.h"
#include "bdjstring.h"
#include "list.h"

tagdef_t tagdefs[MAX_TAG_KEY] = {
  [TAG_KEY_ADJUSTFLAGS] =
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
  [TAG_KEY_AFMODTIME] =
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
  [TAG_KEY_ALBUM] =
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
  [TAG_KEY_ALBUMARTIST] =
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
  [TAG_KEY_ARTIST] =
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
  [TAG_KEY_AUTOORGFLAG] =
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
  [TAG_KEY_BPM] =
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
  [TAG_KEY_COMPOSER] =
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
  [TAG_KEY_CONDUCTOR] =
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
  [TAG_KEY_DANCE] =
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
  [TAG_KEY_DANCELEVEL] =
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
  [TAG_KEY_DANCERATING] =
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
  [TAG_KEY_DATE] =
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
  [TAG_KEY_DBADDDATE] =
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
  [TAG_KEY_DISCNUMBER] =
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
  [TAG_KEY_DISCTOTAL] =
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
  [TAG_KEY_DISPLAYIMG] =
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
  [TAG_KEY_DURATION] =
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
  [TAG_KEY_DURATION_HMS] =
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
    0                             /* text search          */
  },
  [TAG_KEY_DURATION_STR] =
  { 9,                            /* default edit order   */
    16,                           /* edit index           */
    15,                           /* edit width           */
    ANCHOR_WEST,                  /* listing anchor       */
    0,                            /* listing weight       */
    ANCHOR_WEST,                  /* song list anchor     */
    0,                            /* song list weight     */
    AIDD_NO,                      /* audio id disp        */
    ET_DISABLED_ENTRY,            /* edit type            */
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
  [TAG_KEY_FILE] =
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
  [TAG_KEY_GENRE] =
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
  [TAG_KEY_KEYWORD] =
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
  [TAG_KEY_MQDISPLAY] =
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
  [TAG_KEY_MUSICBRAINZ_TRACKID] =
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
  [TAG_KEY_NOMAXPLAYTIME] =
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
  [TAG_KEY_NOTES] =
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
  [TAG_KEY_SAMESONG] =
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
  [TAG_KEY_SONGEND] =
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
  [TAG_KEY_SONGSTART] =
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
  [TAG_KEY_SPEEDADJUSTMENT] =
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
  [TAG_KEY_STATUS] =
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
  [TAG_KEY_TAGS] =
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
  [TAG_KEY_TITLE] =
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
  [TAG_KEY_TRACKNUMBER] =
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
  [TAG_KEY_TRACKTOTAL] =
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
  [TAG_KEY_VARIOUSARTISTS] =
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
  [TAG_KEY_UPDATEFLAG] =
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
  [TAG_KEY_UPDATETIME] =
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
  [TAG_KEY_VOLUMEADJUSTPERC] =
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
  [TAG_KEY_WRITETIME] =
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
  [TAG_KEY_RRN] =
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
  [TAG_KEY_DUR] =
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

