#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>
#include <stdint.h>

#include "fileutil.h"

typedef enum {
  LOG_LVL_NONE,
  LOG_LVL_1,
  LOG_LVL_2,
  LOG_LVL_3,
  LOG_LVL_4,
  LOG_LVL_5,
  LOG_LVL_6,
  LOG_LVL_7,
  LOG_LVL_8,
  LOG_LVL_9,
  LOG_LVL_ALL,
  LOG_LVL_MAX
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
  bdjloglvl_t   level;
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
