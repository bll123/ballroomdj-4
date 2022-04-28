#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"
#include "musicdb.h"

enum {
  BDJ4_INIT_NONE                = 0x0000,
  BDJ4_INIT_NO_DB_LOAD          = 0x0001,
  BDJ4_INIT_NO_DETACH           = 0x0002,
  BDJ4_INIT_NO_START            = 0x0004,
  BDJ4_INIT_NO_MARQUEE          = 0x0008,
  BDJ4_INIT_NO_DATAFILE_LOAD    = 0x0010,
  BDJ4_INIT_HIDE_MARQUEE        = 0x0020,
  BDJ4_DB_REBUILD               = 0x0040,
  BDJ4_DB_CHECK_NEW             = 0x0080,
  BDJ4_DB_PROGRESS              = 0x0100,
  BDJ4_DB_REORG                 = 0x0200,
  BDJ4_DB_UPD_FROM_TAGS         = 0x0400,
  BDJ4_DB_WRITE_TAGS            = 0x0800,
  BDJ4_CLI                      = 0x1000,
};

int bdj4startup (int argc, char *argv[], musicdb_t **musicdb,
    char *tag, bdjmsgroute_t route, int flags);
void bdj4shutdown (bdjmsgroute_t route, musicdb_t *musicdb);

#endif /* INC_BDJ4INIT_H */
