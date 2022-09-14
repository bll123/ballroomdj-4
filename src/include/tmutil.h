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

void      mssleep (time_t);
time_t    mstime (void);
time_t    mstimestartofday (void);
void      mstimestart (mstime_t *tm);
time_t    mstimeend (mstime_t *tm);
void      mstimeset (mstime_t *tm, time_t addTime);
void      mstimesettm (mstime_t *mt, time_t addTime);
bool      mstimeCheck (mstime_t *tm);
char      *tmutilDstamp (char *, size_t);
char      * tmutilDisp (char *buff, size_t max);
char      *tmutilTstamp (char *, size_t);
char      *tmutilShortTstamp (char *, size_t);
char      *tmutilToMS (time_t ms, char *buff, size_t max);
char      *tmutilToMSD (time_t ms, char *buff, size_t max);
char      * tmutilToDateHM (time_t ms, char *buff, size_t max);
long      tmutilStrToMS (const char *str);
long      tmutilStrToHM (const char *str);

#endif /* INC_TMUTIL_H */
