#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>

typedef struct {
  FILE          *fh;
  int           loggingOn;
  int           debugOn;
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

bdjlog_t *  logOpen (char *fn);
void        logClose (logidx_t);
void        logError (logidx_t, char *, int);
void        logMsg (logidx_t, char *);
void        logVarMsg (logidx_t, char *, char *, ...);
void        logStderr (void);
void        logStart (void);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
