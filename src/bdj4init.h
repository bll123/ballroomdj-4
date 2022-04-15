#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"

enum {
  BDJ4_INIT_NONE        = 0x00,
  BDJ4_INIT_NO_DB_LOAD  = 0x0001,
  BDJ4_INIT_NO_DETACH   = 0x0002,
  BDJ4_INIT_NO_START    = 0x0004,
  BDJ4_INIT_NO_MARQUEE  = 0x0008,
  BDJ4_INIT_NO_DATAFILE_LOAD  = 0x0010,
  BDJ4_DB_REBUILD       = 0x0020,
  BDJ4_DB_CHECK_NEW     = 0x0040,
};

int bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route,
    int flags);
void bdj4shutdown (bdjmsgroute_t route);

#endif /* INC_BDJ4INIT_H */
