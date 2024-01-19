/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bdj4.h"
#include "player.h"

/*
 * log tags:
 *  alt  - alt-setup
 *  bpmc - bpm counter
 *  chk  - check_all
 *  cfui - configuration ui
 *  dbup - database update
 *  dmfd - dbg-mk-music-from-db
 *  help - helper
 *  info - bdj4info
 *  inst - installer
 *  lnch - launcher
 *  main - main
 *  mm   - mobile marquee
 *  mq   - marquee
 *  mui  - management ui
 *  play - player
 *  plui - player ui
 *  rc   - mobile remote control
 *  strt - starter
 *  tdbc - tdbcompare
 *  tdbs - tdbsetval
 *  tags - bdj4tags
 *  tset - tmusicsetup
 *  ts   - testsuite
 *  taes - aesed
 *  ttdc - ttagdbchk
 *  tvlc - vlcsinklist
 *  tvol - voltest
 *  updt - updater
 *
 */

typedef uint32_t   loglevel_t;

enum {
  LOG_NONE            = 0x00000000,
  LOG_IMPORTANT       = 0x00000001,  // 1
  LOG_BASIC           = 0x00000002,  // 2
  LOG_MSGS            = 0x00000004,  // 4
  LOG_INFO            = 0x00000008,  // 8
  LOG_ACTIONS         = 0x00000010,  // 16
  LOG_LIST            = 0x00000020,  // 32
  LOG_SONGSEL         = 0x00000040,  // 64
  LOG_DANCESEL        = 0x00000080,  // 128
  LOG_VOLUME          = 0x00000100,  // 256
  LOG_SOCKET          = 0x00000200,  // 512
  LOG_DB              = 0x00000400,  // 1024
  LOG_RAFILE          = 0x00000800,  // 2048
  LOG_PROC            = 0x00001000,  // 4096
  LOG_PLAYER          = 0x00002000,  // 8192
  LOG_DATAFILE        = 0x00004000,  // 16384
  LOG_PROCESS         = 0x00008000,  // 32768
  LOG_WEBSRV          = 0x00010000,  // 65536
  LOG_WEBCLIENT       = 0x00020000,  // 131072
  LOG_DBUPDATE        = 0x00040000,  // 262144
  LOG_PROGSTATE       = 0x00080000,  // 524288
  LOG_ITUNES          = 0x00100000,  // 1048576
  LOG_AUDIO_ADJUST    = 0x00200000,  // 2097152
  LOG_AUDIO_TAG       = 0x00400000,  // 4194304
  LOG_AUDIO_ID        = 0x00800000,  // 8388608
  LOG_AUDIOID_DUMP    = 0x01000000,  // 16777216
  /* insert new values here, push redir_inst down */
  LOG_REDIR_INST      = 0x80000000,
  LOG_ALL             = ~LOG_REDIR_INST,
};

typedef enum {
  LOG_ERR,
  LOG_SESS,
  LOG_DBG,
  LOG_INSTALL,
  LOG_GTK,
  LOG_MAX
} logidx_t;

typedef struct bdjlog bdjlog_t;

#define LOG_ERROR_NAME    "logerror"
#define LOG_SESSION_NAME  "logsession"
#define LOG_DEBUG_NAME    "logdbg"
#define LOG_INSTALL_NAME  "loginstall"
#define LOG_GTK_NAME      "loggtk"
#define LOG_EXTENSION     ".txt"

enum {
  LOG_MAX_BUFF    = 4096,
};

#define logProcBegin(lvl,tag)   rlogProcBegin (lvl, tag, __FILE__, __LINE__)
#define logProcEnd(lvl,tag,suffix)  rlogProcEnd (lvl, tag, suffix, __FILE__, __LINE__)
#define logError(msg)           rlogError (msg, errno, __FILE__, __LINE__)
#define logMsg(idx,lvl,fmt,...) rlogVarMsg (idx, lvl, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

bdjlog_t *  logOpen (const char *fn, const char *processtag);
bdjlog_t *  logOpenAppend (const char *fn, const char *processtag);
void logClose (logidx_t idx);
bool logCheck (logidx_t idx, loglevel_t level);
void logSetLevelAll (loglevel_t level);
void logSetLevel (logidx_t idx, loglevel_t level, const char *processtag);
void logStart (const char *processnm, const char *processtag, loglevel_t level);
void logStartAppend (const char *processnm, const char *processtag, loglevel_t level);
void logEnd (void);
void logBacktraceHandler (int sig);
void logBacktrace (void);
const char * logPlstateDebugText (playerstate_t plstate);
const char * logStateDebugText (int state);
void logBasic (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
void logStderr (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

/* needed by the #defines */
void rlogProcBegin (loglevel_t level, const char *tag, const char *fn, int line);
void rlogProcEnd (loglevel_t level, const char *tag, const char *suffix, const char *fn, int line);
void rlogError (const char *msg, int err, const char *fn, int line);
void rlogVarMsg (logidx_t, loglevel_t level, const char *fn, int line, const char *fmt, ...)
    __attribute__ ((format (printf, 5, 6)));

#endif /* INC_BDJLOG_H */
