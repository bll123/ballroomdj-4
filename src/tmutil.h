#ifndef INC_TMUTIL_H
#define INC_TMUTIL_H

#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>

#if ! _typ_suseconds_t && ! defined (BDJ_TYPEDEF_USECONDS)
 typedef useconds_t suseconds_t;
 #define BDJ_TYPEDEF_USECONDS 1
#endif

typedef struct {
  struct timeval    tm;
} mstime_t;

void      mssleep (size_t);
time_t    mstime (void);
time_t    mstimestartofday (void);
void      mstimestart (mstime_t *tm);
time_t    mstimeend (mstime_t *tm);
void      mstimeset (mstime_t *tm, ssize_t addTime);
bool      mstimeCheck (mstime_t *tm);
char      *tmutilDstamp (char *, size_t);
char      * tmutilDisp (char *buff, size_t max);
char      *tmutilTstamp (char *, size_t);
char      *tmutilToMS (ssize_t ms, char *buff, size_t max);
char      *tmutilToMSD (ssize_t ms, char *buff, size_t max);
char      * tmutilToDateHM (ssize_t ms, char *buff, size_t max);

#endif /* INC_TMUTIL_H */
