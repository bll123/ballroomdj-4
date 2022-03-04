#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"

enum {
  BDJ4_INIT_NONE        = 0x00,
  BDJ4_INIT_NO_DB_LOAD  = 0x01,
  BDJ4_INIT_NO_DETACH   = 0x02,
  BDJ4_INIT_NO_START    = 0x04,
  BDJ4_INIT_NO_MARQUEE  = 0x08,
};

int bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route,
    int flags);
void bdj4shutdown (bdjmsgroute_t route);

#endif /* INC_BDJ4INIT_H */
