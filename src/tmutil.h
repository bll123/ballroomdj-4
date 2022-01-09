#ifndef INC_TMUTIL_H
#define INC_TMUTIL_H

#include <sys/time.h>

typedef struct {
  struct timeval    start;
} mstime_t;

void      mssleep (size_t);
time_t    mstime (void);
void      mstimestart (mstime_t *);
size_t    mstimeend (mstime_t *);
char      *tmutilDstamp (char *, size_t);
char      *tmutilTstamp (char *, size_t);

#endif /* INC_TMUTIL_H */
