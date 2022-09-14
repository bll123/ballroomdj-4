#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"
#include "musicdb.h"

enum {
  BDJ4_INIT_NONE                = 0x00000000,
  BDJ4_INIT_NO_DB_LOAD          = 0x00000001,
  BDJ4_INIT_NO_DETACH           = 0x00000002,
  BDJ4_INIT_NO_START            = 0x00000004,
  BDJ4_INIT_NO_MARQUEE          = 0x00000008,
  BDJ4_INIT_NO_DATAFILE_LOAD    = 0x00000010,
  BDJ4_INIT_HIDE_MARQUEE        = 0x00000020,
  BDJ4_INIT_NO_LOCK             = 0x00000040,
  BDJ4_INIT_WAIT                = 0x00000080,
  BDJ4_DB_REBUILD               = 0x00000100,
  BDJ4_DB_CHECK_NEW             = 0x00000200,
  BDJ4_DB_PROGRESS              = 0x00000400,
  BDJ4_DB_REORG                 = 0x00000800,
  BDJ4_DB_UPD_FROM_TAGS         = 0x00001000,
  BDJ4_DB_WRITE_TAGS            = 0x00002000,
  BDJ4_DB_CLI                   = 0x00004000,
  BDJ4_TS_RUNSECTION            = 0x00008000,
  BDJ4_TS_RUNTEST               = 0x00010000,
};

int bdj4startup (int argc, char *argv[], musicdb_t **musicdb,
    char *tag, bdjmsgroute_t route, int flags);
musicdb_t * bdj4ReloadDatabase (musicdb_t *musicdb);
void bdj4shutdown (bdjmsgroute_t route, musicdb_t *musicdb);

#endif /* INC_BDJ4INIT_H */
