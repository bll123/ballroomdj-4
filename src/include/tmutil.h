/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#if ! _typ_suseconds_t && ! defined (BDJ_TYPEDEF_USECONDS)
 typedef useconds_t suseconds_t;
 #define BDJ_TYPEDEF_USECONDS 1
#endif

typedef enum {
  TM_CLOCK_ISO,
  TM_CLOCK_LOCAL,
  TM_CLOCK_TIME_12,
  TM_CLOCK_TIME_24,
  TM_CLOCK_OFF,
  TM_CLOCK_MAX,
} bdjtmclock_t;

enum {
  /* this is just a very large number so that the timer won't pop */
  /* any time soon. use 48 hours */
  TM_TIMER_OFF = 172800000,
};

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
char      * tmutilDisp (char *buff, size_t max, int type);
char      *tmutilTstamp (char *, size_t);
char      *tmutilShortTstamp (char *, size_t);
char      *tmutilToMS (time_t ms, char *buff, size_t max);
char      *tmutilToMSD (time_t ms, char *buff, size_t max, int decimals);
char      * tmutilToDateHM (time_t ms, char *buff, size_t max);
long      tmutilStrToMS (const char *str);
long      tmutilStrToHM (const char *str);
time_t    tmutilStringToUTC (const char *str, const char *fmt);

/* linux does not define this by default */
char * strptime (const char *buf, const char *fmt, struct tm *tm);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

