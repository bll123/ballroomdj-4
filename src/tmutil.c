#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "tmutil.h"
#include "bdjstring.h"

void
mssleep (size_t mt)
{
  size_t t = mt * 1000;
  usleep ((useconds_t) t);
}

time_t
mstime (void)
{
  struct timeval    curr;
  time_t            s, u, m, tot;

  gettimeofday (&curr, NULL);

  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
  tot = s * 1000 + m;
  return tot;
}

void
mstimestart (mstime_t *mt)
{
  gettimeofday (&mt->tm, NULL);
}

time_t
mstimeend (mstime_t *t)
{
  struct timeval    end;
  time_t            s, u, m;

  gettimeofday (&end, NULL);
  s = end.tv_sec - t->tm.tv_sec;
  u = end.tv_usec - t->tm.tv_usec;
  m = s * 1000 + u / 1000;
  return m;
}

void
mstimeset (mstime_t *mt, size_t addTime)
{
  gettimeofday (&mt->tm, NULL);
  mt->tm.tv_sec += addTime / 1000;
  mt->tm.tv_usec += (addTime - ((addTime / 1000) * 1000)) * 1000;
}

bool
mstimeCheck (mstime_t *mt)
{
  struct timeval    ttm;

  gettimeofday (&ttm, NULL);
  if (ttm.tv_sec > mt->tm.tv_sec) {
    return true;
  }
  if (ttm.tv_sec == mt->tm.tv_sec &&
      ttm.tv_usec >= mt->tm.tv_usec) {
    return true;
  }
  return false;
}

char *
tmutilDstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  time_t            s;
#if _lib_localtime_r
  struct tm         t;
#endif

  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%Y-%m-%d", tp);
  return buff;
}

char *
tmutilTstamp (char *buff, size_t max)
{
  struct timeval    curr;
  struct tm         *tp;
  char              tbuff [20];
  time_t            s, u, m;
#if _lib_localtime_r
  struct tm         t;
#endif


  gettimeofday (&curr, NULL);
  s = curr.tv_sec;
  u = curr.tv_usec;
  m = u / 1000;
#if _lib_localtime_r
  localtime_r (&s, &t);
  tp = &t;
#else
  tp = localtime (&s);
#endif
  strftime (buff, max, "%H:%M:%S", tp);
  snprintf (tbuff, sizeof (tbuff), ".%03ld", m);
  strlcat (buff, tbuff, max);
  return buff;
}

char *
tmutilToMS (ssize_t ms, char *buff, size_t max)
{
  ssize_t     m, s;

  m = ms / 1000 / 60;
  s = (ms - (m * 1000 * 60)) / 1000;
  snprintf (buff, max, "%zd:%02zd", m, s);
  return buff;
}




