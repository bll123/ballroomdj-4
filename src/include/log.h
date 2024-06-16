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
  LOG_NONE            = 0,
  LOG_IMPORTANT       = (1 << 0), // 1
  LOG_BASIC           = (1 << 1), // 2
  LOG_MSGS            = (1 << 2), // 4
  LOG_INFO            = (1 << 3), // 8
  LOG_ACTIONS         = (1 << 4), // 16
  LOG_LIST            = (1 << 5), // 32
  LOG_SONGSEL         = (1 << 6), // 64
  LOG_DANCESEL        = (1 << 7), // 128
  LOG_VOLUME          = (1 << 8), // 256
  LOG_SOCKET          = (1 << 9), // 512
  LOG_DB              = (1 << 10), // 1024
  LOG_RAFILE          = (1 << 11), // 2048
  LOG_PROC            = (1 << 12), // 4096
  LOG_PLAYER          = (1 << 13), // 8192
  LOG_DATAFILE        = (1 << 14), // 16384
  LOG_PROCESS         = (1 << 15), // 32768
  LOG_WEBSRV          = (1 << 16), // 65536
  LOG_WEBCLIENT       = (1 << 17), // 131072
  LOG_DBUPDATE        = (1 << 18), // 262144
  LOG_PROGSTATE       = (1 << 19), // 524288
  LOG_ITUNES          = (1 << 20), // 1048576
  LOG_AUDIO_ADJUST    = (1 << 21), // 2097152
  LOG_AUDIO_TAG       = (1 << 22), // 4194304
  LOG_AUDIO_ID        = (1 << 23), // 8388608
  LOG_AUDIOID_DUMP    = (1 << 24), // 16777216
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

#define logStartProgram(prog)   rlogStartProgram (prog, __FILE__, __LINE__, __func__)
#define logProcBegin()          rlogProcBegin (__FILE__, __LINE__, __func__)
#define logProcEnd(suffix)      rlogProcEnd (suffix, __FILE__, __LINE__, __func__)
#define logError(msg)           rlogError (msg, errno, __FILE__, __LINE__, __func__)
#define logMsg(idx,lvl,fmt,...) rlogVarMsg (idx, lvl, __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)

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
const char * logPlayerState (playerstate_t plstate);
const char * logStateDebugText (int state);
void logBasic (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
void logStderr (const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

/* needed by the #defines */
void rlogStartProgram (const char *prog, const char *fn, int line, const char *func);
void rlogProcBegin (const char *fn, int line, const char *func);
void rlogProcEnd (const char *suffix, const char *fn, int line, const char *func);
void rlogError (const char *msg, int err, const char *fn, int line, const char *func);
void rlogVarMsg (logidx_t, loglevel_t level, const char *fn, int line, const char *func, const char *fmt, ...)
    __attribute__ ((format (printf, 6, 7)));

#endif /* INC_BDJLOG_H */
