#ifndef INC_BDJLOG_H
#define INC_BDJLOG_H

#include <stdio.h>

extern FILE   *logfh;
extern int    loggingOn;

void startBDJLog (char *fn);
void bdjlogError (char *, int);
void bdjlogMsg (char *, char *, int, ...);
void bdjlog (char *);

#endif /* INC_BDJLOG_H */
