#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>
#include <stdint.h>

#include "fileutil.h"

/*
 * 7 : important + basic + main
 * 15 : 7 + datafile
 * 23 : 7 + list
 * 31 : 7 + datafile + list
 * 263 : 7 + socket
 * 4103 : 7 + songsel
 */
typedef enum {
  LOG_NONE      = 0x0000,
  LOG_IMPORTANT = 0x0001,
  LOG_BASIC     = 0x0002,
  LOG_MAIN      = 0x0004,
  LOG_DATAFILE  = 0x0008,
  LOG_LIST      = 0x0010,
  LOG_PLAYER    = 0x0020,
  LOG_PROCESS   = 0x0040,
  LOG_VOLUME    = 0x0080,
  LOG_SOCKET    = 0x0100,
  LOG_DB        = 0x0200,
  LOG_RAFILE    = 0x0400,
  LOG_PROC      = 0x0800,
  LOG_SONGSEL   = 0x1000,
  LOG_ALL       = 0xFFFF,
} bdjloglvl_t;

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
  long          level;
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
void        logClose (logidx_t);
void        rlogProcBegin (bdjloglvl_t level, const char *tag,
                const char *fn, int line);
void        rlogProcEnd (bdjloglvl_t level, const char *tag,
                const char *suffix, const char *fn, int line);
void        rlogError (const char *msg, int err, const char *fn, int line);
void        rlogVarMsg (logidx_t, bdjloglvl_t level,
                const char *fn, int line, const char *fmt, ...);
void        logSetLevel (logidx_t idx, bdjloglvl_t level);
void        logStart (const char *processtag, bdjloglvl_t level);
void        logStartAppend (const char *processnm,
                const char *processtag, bdjloglvl_t level);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
