#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"

int bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route);
void bdj4shutdown (bdjmsgroute_t route);

#endif /* INC_BDJ4INIT_H */
