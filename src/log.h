#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fileutil.h"

typedef uint32_t   loglevel_t;

#define LOG_NONE        0x00000000
#define LOG_IMPORTANT   0x00000001  // 1
#define LOG_BASIC       0x00000002  // 2
#define LOG_MSGS        0x00000004  // 4
#define LOG_MAIN        0x00000008  // 8
#define LOG_LIST        0x00000010  // 16
#define LOG_SONGSEL     0x00000020  // 32
#define LOG_DANCESEL    0x00000040  // 64
#define LOG_VOLUME      0x00000080  // 128
#define LOG_SOCKET      0x00000100  // 256
#define LOG_DB          0x00000200  // 512
#define LOG_RAFILE      0x00000400  // 1024
#define LOG_PROC        0x00000800  // 2048
#define LOG_PLAYER      0x00001000  // 4096
#define LOG_DATAFILE    0x00002000  // 8192
#define LOG_PROCESS     0x00004000  // 16384
#define LOG_WEBSRV      0x00008000  // 32768
#define LOG_WEBCLIENT   0x00010000  // 65536
#define LOG_ALL         0xFFFFFFFF

typedef enum {
  LOG_ERR,
  LOG_SESS,
  LOG_DBG,
  LOG_MAX
} logidx_t;

typedef struct {
  filehandle_t  fhandle;
  int           opened;
  int           indent;
  ssize_t       level;
  const char    *processTag;
} bdjlog_t;

#define LOG_ERROR_NAME    "logerror"
#define LOG_SESSION_NAME  "logsession"
#define LOG_DEBUG_NAME    "logdbg"
#define LOG_EXTENSION     ".txt"

#define logProcBegin(lvl,tag)   rlogProcBegin (lvl, tag, __FILE__, __LINE__)
#define logProcEnd(lvl,tag,suffix)  rlogProcEnd (lvl, tag, suffix, __FILE__, __LINE__)
#define logError(msg)           rlogError (msg, errno, __FILE__, __LINE__)
#define logMsg(idx,lvl,fmt,...) rlogVarMsg (idx, lvl, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

bdjlog_t *  logOpen (const char *fn, const char *processtag);
bdjlog_t *  logOpenAppend (const char *fn, const char *processtag);
void        logClose (logidx_t idx);
bool        logCheck (logidx_t idx, loglevel_t level);
void        rlogProcBegin (loglevel_t level, const char *tag,
                const char *fn, int line);
void        rlogProcEnd (loglevel_t level, const char *tag,
                const char *suffix, const char *fn, int line);
void        rlogError (const char *msg, int err, const char *fn, int line);
void        rlogVarMsg (logidx_t, loglevel_t level,
                const char *fn, int line, const char *fmt, ...);
void        logSetLevel (logidx_t idx, loglevel_t level);
void        logStart (const char *processtag, loglevel_t level);
void        logStartAppend (const char *processnm,
                const char *processtag, loglevel_t level);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
