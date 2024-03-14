/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJ4INIT_H
#define INC_BDJ4INIT_H

#include "bdjmsg.h"
#include "log.h"
#include "musicdb.h"

enum {
  BDJ4_INIT_ALL                 = 0,      // uses db, datafiles, locks
  BDJ4_INIT_NO_DB_LOAD          = (1 << 0),
  BDJ4_INIT_NO_DETACH           = (1 << 1),
  BDJ4_INIT_NO_START            = (1 << 2),
  BDJ4_INIT_NO_MARQUEE          = (1 << 3),
  BDJ4_INIT_NO_DATAFILE_LOAD    = (1 << 4),
  BDJ4_INIT_HIDE_MARQUEE        = (1 << 5),
  BDJ4_INIT_NO_LOCK             = (1 << 6),
  BDJ4_INIT_WAIT                = (1 << 7),
  BDJ4_ARG_DB_REBUILD           = (1 << 8),
  BDJ4_ARG_DB_CHECK_NEW         = (1 << 9),
  BDJ4_ARG_DB_COMPACT           = (1 << 10),
  BDJ4_ARG_DB_REORG             = (1 << 11),
  BDJ4_ARG_DB_UPD_FROM_TAGS     = (1 << 12),
  BDJ4_ARG_DB_UPD_FROM_ITUNES   = (1 << 13),
  BDJ4_ARG_DB_WRITE_TAGS        = (1 << 14),
  BDJ4_ARG_PROGRESS             = (1 << 15),
  BDJ4_ARG_CLI                  = (1 << 16),
  BDJ4_ARG_VERBOSE              = (1 << 17),
  BDJ4_ARG_QUIET                = (1 << 18),
  BDJ4_ARG_TS_RUNSECTION        = (1 << 19),
  BDJ4_ARG_TS_RUNTEST           = (1 << 20),
  BDJ4_ARG_TS_STARTTEST         = (1 << 21),
  BDJ4_ARG_INST_UNATTENDED      = (1 << 22),
  BDJ4_ARG_INST_REINSTALL       = (1 << 23),
  BDJ4_ARG_UPD_NEW              = (1 << 24),
  BDJ4_ARG_UPD_CONVERT          = (1 << 25),
  BDJ4_INIT_NO_LOG              = (1 << 26),
};

void bdj4initArgInit (void);
void bdj4initArgCleanup (void);
char * bdj4initArgFromWide (int idx, const char *arg);
void bdj4initFreeArg (void *targ);

loglevel_t bdj4startup (int argc, char *argv[], musicdb_t **musicdb,
    char *tag, bdjmsgroute_t route, uint32_t *flags);
musicdb_t * bdj4ReloadDatabase (musicdb_t *musicdb);
void bdj4shutdown (bdjmsgroute_t route, musicdb_t *musicdb);

#endif /* INC_BDJ4INIT_H */
