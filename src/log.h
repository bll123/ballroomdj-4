#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>

typedef struct {
  FILE          *fh;
  int           indent;
  const char    *processTag;
} bdjlog_t;

typedef enum {
  LOG_ERR,
  LOG_SESS,
  LOG_DBG,
  LOG_MAX
} logidx_t;

#define LOG_ERROR_NAME    "logerror"
#define LOG_SESSION_NAME  "logsession"
#define LOG_DEBUG_NAME    "logdbg"
#define LOG_EXTENSION     ".txt"

#if defined (BDJLOG_OFF)
# define logProcBegin(tag)
# define logProcEnd(tag,suffix)
# define logError(msg)
# define logMsg(idx,fmt,...)
#else
# define logProcBegin(tag)   rlogProcBegin (tag, __FILE__, __LINE__)
# define logProcEnd(tag,suffix)  rlogProcEnd (tag, suffix, __FILE__, __LINE__)
# define logError(msg)       rlogError (msg, errno, __FILE__, __LINE__)
# define logMsg(idx,fmt,...) rlogVarMsg (idx, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

bdjlog_t *  logOpen (const char *fn, const char *processtag);
bdjlog_t *  logOpenAppend (const char *fn, const char *processtag);
void        logClose (logidx_t);
void        rlogProcBegin (const char *tag, const char *fn, int line);
void        rlogProcEnd (const char *tag, const char *suffix, const char *fn, int line);
void        rlogError (const char *msg, int err, const char *fn, int line);
void        rlogVarMsg (logidx_t, const char *, int, const char *, ...);
void        logStderr (void);
void        logStart (const char *processtag);
void        logStartAppend (const char *processnm, const char *processtag);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
