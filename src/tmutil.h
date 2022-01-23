#ifndef INC_TMUTIL_H
#define INC_TMUTIL_H

#include <stdbool.h>
#include <sys/time.h>

typedef struct {
  struct timeval    tm;
} mstime_t;

void      mssleep (size_t);
time_t    mstime (void);
void      mstimestart (mstime_t *tm);
time_t    mstimeend (mstime_t *tm);
void      mstimeset (mstime_t *tm, size_t addTime);
bool      mstimeCheck (mstime_t *tm);
char      *tmutilDstamp (char *, size_t);
char      *tmutilTstamp (char *, size_t);
char      *tmutilToMS (ssize_t ms, char *buff, size_t max);

#endif /* INC_TMUTIL_H */
