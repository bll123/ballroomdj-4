#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"

enum {
  BDJ4_INIT_NONE,
  BDJ4_INIT_NO_DB_LOAD,
};

int bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route,
    int flags);
void bdj4shutdown (bdjmsgroute_t route);

#endif /* INC_BDJ4INIT_H */
