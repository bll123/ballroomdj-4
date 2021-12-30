#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>

typedef struct {
  FILE          *fh;
  int           loggingOn;
} bdjlog_t;

typedef enum {
  LOG_ERR,
  LOG_SESS,
  LOG_DBG,
  LOG_MAX
} logidx_t;

#define LOG_ERROR_NAME    "data/%s/logerror%s%s"
#define LOG_SESSION_NAME  "data/%s/logsession%s%s"
#define LOG_DEBUG_NAME    "data/%s/logdbg%s%s"
#define LOG_EXTENSION     ".txt"

/* logProcBegin/End always use LOG_DBG */
#define logProcBegin(tag)   rlogProcBegin (tag, __FILE__, __LINE__)
#define logProcEnd(tag,suffix)  rlogProcEnd (tag, suffix, __FILE__, __LINE__)
#define logError(msg)       rlogError (msg, errno, __FILE__, __LINE__)
#define logMsg(idx,fmt,...) rlogVarMsg (idx, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

bdjlog_t *  logOpen (const char *fn);
void        logClose (logidx_t);
void        rlogProcBegin (const char *tag, const char *fn, int line);
void        rlogProcEnd (const char *tag, const char *suffix, const char *fn, int line);
void        rlogError (const char *msg, int err, const char *fn, int line);
void        rlogVarMsg (logidx_t, const char *, int, const char *, ...);
void        logStderr (void);
void        logStart (void);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
