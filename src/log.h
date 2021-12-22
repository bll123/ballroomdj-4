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
  LOG_MAX
} logidx_t;

bdjlog_t *  logOpen (char *fn);
void        logClose (logidx_t);
void        logError (logidx_t, char *, int);
void        logMsg (logidx_t, char *);
void        logVarMsg (logidx_t, char *, char *, ...);

void        logStart (void);
void        logEnd (void);

#endif /* INC_BDJLOG_H */
